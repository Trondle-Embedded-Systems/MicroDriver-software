#include "tmc2209_api_registers.h"
#include "tmc2209_api.h"
#include "esphome/core/helpers.h"
#include "esphome/core/hal.h"

namespace esphome {
namespace tmc2209 {

uint8_t TMC2209API::crc8_(uint8_t *data, uint32_t bytes) {
  uint8_t result = 0;
  uint8_t *table;

  while (bytes--)
    result = tmc_crc_table_poly7_reflected[result ^ *data++];

  // Flip the result around
  // swap odd and even bits
  result = ((result >> 1) & 0x55) | ((result & 0x55) << 1);
  // swap consecutive pairs
  result = ((result >> 2) & 0x33) | ((result & 0x33) << 2);
  // swap nibbles ...
  result = ((result >> 4) & 0x0F) | ((result & 0x0F) << 4);

  return result;
}

void TMC2209API::set_dirty_bit_(uint8_t index, bool value) {
  if (index >= REGISTER_COUNT)
    return;

  uint8_t *tmp = &this->dirty_bits_[index / 8];
  uint8_t shift = (index % 8);
  uint8_t mask = 1 << shift;
  *tmp = (((*tmp) & (~(mask))) | (((value) << (shift)) & (mask)));
}

bool TMC2209API::get_dirty_bit_(uint8_t index) {
  if (index >= REGISTER_COUNT)
    return false;

  uint8_t *tmp = &this->dirty_bits_[index / 8];
  uint8_t shift = (index % 8);
  return ((*tmp) >> shift) & 1;
}

bool TMC2209API::cache_(CacheOperation operation, uint8_t address, uint32_t *value) {
  if (operation == CACHE_READ) {
    if (IS_READABLE(register_access_[address]))
      return false;

    // Grab the value from the cache
    *value = this->shadow_register_[address];
    return true;
  } else if (operation == CACHE_WRITE || operation == CACHE_FILL_DEFAULT) {
    // Fill the cache

    // Write to the shadow register.
    this->shadow_register_[address] = *value;
    // For write operations, mark the register dirty
    if (operation == CACHE_WRITE) {
      this->set_dirty_bit_(address, true);
    }

    return true;
  }
  return false;
}

// Sync byte (lower nibble 0b0101; upper nibble is "don't care" per datasheet 4.1).
static const uint8_t SYNC_BYTE = 0x05;
// Read replies are addressed to the master using node address 0xFF (datasheet 4.1.2).
static const uint8_t MASTER_ADDRESS = 0xFF;
// Transient glitches on the shared single wire must not be treated as hard failures.
// The datasheet defines clean error recovery; retry a few times like Klipper's tmc_uart.
static const uint8_t MAX_UART_ATTEMPTS = 3;
// Upper bound for an 8-byte reply to arrive. At 500 kBaud it is ~0.2 ms; even at the
// minimum 9600 Baud it is ~8.5 ms. Keeping it short lets a missing/unwired driver fail
// fast instead of stalling setup on the UART's much longer internal read timeout.
static const uint32_t REPLY_TIMEOUT_MS = 25;

void TMC2209API::flush_rx_() {
  // Drop any stale or previously echoed bytes so each datagram starts on a clean bus.
  uint8_t scrap;
  while (this->parent_->available())
    this->parent_->read_byte(&scrap);
}

void TMC2209API::send_datagram_(const uint8_t *data, uint8_t len) {
  this->flush_rx_();
  this->parent_->write_array(data, len);
  this->parent_->flush();  // wait until fully transmitted
  // On the single wire our own transmission is echoed straight back into the
  // receiver; discard it so it cannot be mistaken for a reply. flush() only
  // waits for TX: the echo typically still sits in the UART hardware FIFO at
  // this point (the esp-idf driver surfaces it after the RX idle timeout, ~10
  // symbol times), so draining "whatever is available" races the echo and
  // leaves stale bytes that corrupt the NEXT transaction (e.g. the IFCNT
  // write-verify read). Wait for the full echo and consume exactly len bytes.
  uint8_t scrap;
  if (this->wait_available_(len)) {
    for (uint8_t i = 0; i < len; i++)
      this->parent_->read_byte(&scrap);
  } else {
    this->flush_rx_();  // echo incomplete (bus glitch): drop whatever did arrive
  }
}

bool TMC2209API::wait_available_(uint8_t count) {
  const uint32_t start = millis();
  while (this->parent_->available() < (int) count) {
    if (millis() - start > REPLY_TIMEOUT_MS)
      return false;
  }
  return true;
}

bool TMC2209API::read_register_once_(uint8_t address, int32_t *value_out) {
  std::array<uint8_t, 8> buffer = {0};
  buffer.at(0) = SYNC_BYTE;
  buffer.at(1) = this->address_;
  buffer.at(2) = address;  // read access leaves bit 7 cleared
  buffer.at(3) = this->crc8_(buffer.data(), 3);

  this->flush_rx_();
  this->parent_->write_array(buffer.data(), 4);
  this->parent_->flush();

  // On the shared single wire the 4-byte request is echoed straight back, followed
  // by the 8-byte reply. Wait for all 12 bytes via available() with a short timeout:
  // a silent/un-echoing bus then fails in a few ms instead of blocking on the UART's
  // ~100 ms internal read timeout (which would also log an error on every byte).
  if (!this->wait_available_(12))
    return false;

  std::array<uint8_t, 4> echo = {0};
  this->parent_->read_array(echo.data(), 4);    // discard the request echo
  this->parent_->read_array(buffer.data(), 8);  // the actual reply

  if (buffer.at(0) != SYNC_BYTE)        // Byte 0: sync
    return false;
  if (buffer.at(1) != MASTER_ADDRESS)   // Byte 1: reply is addressed to the master
    return false;
  if (buffer.at(2) != address)          // Byte 2: echoed register address
    return false;
  if (buffer.at(7) != this->crc8_(buffer.data(), 7))  // Byte 7: CRC
    return false;

  *value_out = encode_uint32(buffer.at(3), buffer.at(4), buffer.at(5), buffer.at(6));
  return true;
}

void TMC2209API::write_register(uint8_t address, int32_t value) {
  ESP_LOGVV(TAG, "writing address 0x%x with value 0x%x (%d)", address, value, value);

  std::array<uint8_t, 8> datagram = {0};
  datagram.at(0) = SYNC_BYTE;
  datagram.at(1) = this->address_;
  datagram.at(2) = address | WRITE_BIT;  // bit 7 set marks a write access
  datagram.at(3) = (value >> 24) & 0xFF;
  datagram.at(4) = (value >> 16) & 0xFF;
  datagram.at(5) = (value >> 8) & 0xFF;
  datagram.at(6) = (value) &0xFF;
  datagram.at(7) = this->crc8_(datagram.data(), 7);

  // Keep the shadow cache in sync so reads of write-only registers return the
  // intended value regardless of bus acknowledgement.
  this->cache_(CACHE_WRITE, address, (uint32_t *) &value);

  if (!this->bus_enabled_)
    return;  // driver previously failed to answer: don't touch the bus

  // Verify each write through the IFCNT datagram counter, which the driver
  // increments on every accepted write (datasheet 4.1.1). Same principle as
  // Klipper's tmc_uart: read IFCNT, write, confirm it advanced by exactly one.
  for (uint8_t attempt = 0; attempt < MAX_UART_ATTEMPTS; attempt++) {
    int32_t before = 0;
    const bool have_counter = this->read_register_once_(IFCNT, &before);

    this->send_datagram_(datagram.data(), 8);

    if (!have_counter)
      return;  // driver not answering reads: send best-effort once, don't hammer the bus

    int32_t after = 0;
    if (this->read_register_once_(IFCNT, &after) && ((uint8_t) after == (uint8_t) (before + 1)))
      return;  // write acknowledged
  }

  ESP_LOGW(TAG, "write to register 0x%02X not acknowledged via IFCNT", address & ADDRESS_MASK);
}

int32_t TMC2209API::read_register(uint8_t address) {
  ESP_LOGVV(TAG, "reading address 0x%x", address);
  uint32_t value;

  // Read from cache for registers with write-only access
  if (this->cache_(CACHE_READ, address, &value))
    return value;

  if (!this->bus_enabled_)
    return 0;  // driver previously failed to answer: don't touch the bus

  address = address & ADDRESS_MASK;

  int32_t result = 0;
  for (uint8_t attempt = 0; attempt < MAX_UART_ATTEMPTS; attempt++) {
    if (this->read_register_once_(address, &result))
      return result;
  }

  ESP_LOGW(TAG, "read of register 0x%02X failed after %d attempts", address, MAX_UART_ATTEMPTS);
  return 0;
}

void TMC2209API::replay_dirty_registers() {
  for (uint8_t address = 0; address < REGISTER_COUNT; address++) {
    if (!this->get_dirty_bit_(address))
      continue;
    const uint8_t access = register_access_[address];
    // Flag registers (GSTAT) are write-to-clear: there is no state to restore.
    if (!IS_WRITABLE(access) || IS_FLAG(access))
      continue;
    this->write_register(address, this->shadow_register_[address]);
  }
}

bool TMC2209API::read_register_checked(uint8_t address, int32_t *value_out) {
  uint32_t value;
  if (this->cache_(CACHE_READ, address, &value)) {
    *value_out = value;
    return true;
  }

  if (!this->bus_enabled_)
    return false;

  address = address & ADDRESS_MASK;
  for (uint8_t attempt = 0; attempt < MAX_UART_ATTEMPTS; attempt++) {
    if (this->read_register_once_(address, value_out))
      return true;
  }
  return false;
}

uint32_t TMC2209API::update_field(uint32_t data, RegisterField field, uint32_t value) {
  return (data & (~field.mask)) | ((value << field.shift) & field.mask);
}

void TMC2209API::write_field(RegisterField field, uint32_t value) {
  uint32_t reg_value = this->read_register(field.address);
  reg_value = this->update_field(reg_value, field, value);
  this->write_register(field.address, reg_value);
}

uint32_t TMC2209API::extract_field(uint32_t data, RegisterField field) {
  uint32_t value = (data & field.mask) >> field.shift;

  if (field.is_signed) {
    uint32_t base_mask = field.mask >> field.shift;
    uint32_t sign_mask = base_mask & (~base_mask >> 1);
    value = (value ^ sign_mask) - sign_mask;
  }

  return value;
}

uint32_t TMC2209API::read_field(RegisterField field) {
  uint32_t value = this->read_register(field.address);
  return this->extract_field(value, field);
}

}  // namespace tmc2209
}  // namespace esphome
