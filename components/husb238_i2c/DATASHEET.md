husb238 datasheet form adafruit :


Adafruit HUSB238 Library
Main Page
+Classes
+Files

Search
Public Member Functions | List of all members
Adafruit_HUSB238 Class Reference
HUSB238 driver. More...

#include <Adafruit_HUSB238.h>

Public Member Functions
 	Adafruit_HUSB238 ()
 	Instantiates a new HUSB238 class. More...
 
bool 	begin (uint8_t addr=HUSB238_I2CADDR_DEFAULT, TwoWire *theWire=&Wire)
 	Sets up the I2C connection and tests that the sensor was found. More...
 
bool 	isAttached ()
 	Reads the attachment status from the HUSB238 device. More...
 
bool 	getCCdirection ()
 	Reads the CC direction from the HUSB238 device. More...
 
HUSB238_ResponseCodes 	getPDResponse ()
 	Reads the PD response from the HUSB238 device. More...
 
bool 	get5VContractV ()
 	Reads the 5V contract voltage status from the HUSB238 device. More...
 
HUSB238_5VCurrentContract 	get5VContractA ()
 	Reads the 5V contract current from the HUSB238 device. More...
 
bool 	isVoltageDetected (HUSB238_PDSelection pd)
 	Checks if a specific voltage is detected. More...
 
HUSB238_CurrentSetting 	currentDetected (HUSB238_PDSelection pd)
 	Reads the detected current based on the PD output selection. More...
 
HUSB238_VoltageSetting 	getPDSrcVoltage ()
 	Reads the source voltage from the HUSB238 device. More...
 
HUSB238_CurrentSetting 	getPDSrcCurrent ()
 	Reads the source current from the HUSB238 device. More...
 
HUSB238_PDSelection 	getSelectedPD ()
 	Gets the selected PDO. More...
 
void 	selectPD (HUSB238_PDSelection pd)
 	Selects a PD output. More...
 
void 	reset ()
 	Resets the HUSB238 device. More...
 
void 	requestPD ()
 	Requests Power Delivery (PD) from the HUSB238 device. More...
 
void 	getSourceCapabilities ()
 	Retrieves the source capabilities of the HUSB238 device. More...
 
Detailed Description
HUSB238 driver.

Constructor & Destructor Documentation
◆ Adafruit_HUSB238()
Adafruit_HUSB238::Adafruit_HUSB238	(		)	
Instantiates a new HUSB238 class.

This constructor initializes a new instance of the HUSB238 class.

Member Function Documentation
◆ begin()
bool Adafruit_HUSB238::begin	(	uint8_t 	addr = HUSB238_I2CADDR_DEFAULT,
TwoWire * 	theWire = &Wire 
)		
Sets up the I2C connection and tests that the sensor was found.

Parameters
addr	The 7-bit I2C address of the HUSB238.
theWire	Pointer to an I2C device we'll use to communicate; default is Wire.
Returns
true if sensor was found, otherwise false.
This function initializes the I2C communication with the HUSB238 device. It uses the provided I2C address and Wire interface. The function returns true if the device is successfully initialized.

◆ isAttached()
bool Adafruit_HUSB238::isAttached	(		)	
Reads the attachment status from the HUSB238 device.

Returns
The attachment status as a boolean value.
This function reads the 6th bit of the HUSB238_PD_STATUS1 register to get the attachment status. It returns true if the attachment status bit is set.

◆ getCCdirection()
bool Adafruit_HUSB238::getCCdirection	(		)	
Reads the CC direction from the HUSB238 device.

Returns
The CC status as a boolean value - false is CC1 connected, true is CC2 connected
This function reads the 7th bit of the HUSB238_PD_STATUS1 register

◆ getPDResponse()
HUSB238_ResponseCodes Adafruit_HUSB238::getPDResponse	(		)	
Reads the PD response from the HUSB238 device.

Returns
The PD response as an HUSB238_ResponseCodes enum value.
This function reads bits 3-5 of the HUSB238_PD_STATUS1 register to get the PD response. It returns the response as an HUSB238_ResponseCodes enum value.

◆ get5VContractV()
bool Adafruit_HUSB238::get5VContractV	(		)	
Reads the 5V contract voltage status from the HUSB238 device.

Returns
The 5V contract voltage status as a boolean value.
This function reads the 2nd bit of the HUSB238_PD_STATUS1 register to get the 5V contract voltage status. It returns true if the 5V contract voltage bit is set.

◆ get5VContractA()
HUSB238_5VCurrentContract Adafruit_HUSB238::get5VContractA	(		)	
Reads the 5V contract current from the HUSB238 device.

Returns
The 5V contract current as an HUSB238_5VCurrentContract enum value.
This function reads the bottom two bits (0-1) of the HUSB238_PD_STATUS1 register to get the 5V contract current. It returns the current as an HUSB238_5VCurrentContract enum value.

◆ isVoltageDetected()
bool Adafruit_HUSB238::isVoltageDetected	(	HUSB238_PDSelection 	pd	)	
Checks if a specific voltage is detected.

Parameters
pd	The PD output selection as an HUSB238_PDSelection enum value.
Returns
true if the voltage is detected, otherwise false.
This function checks if a specific voltage is detected based on the PD selection. It reads the 7th bit of the corresponding register to determine the status.

◆ currentDetected()
HUSB238_CurrentSetting Adafruit_HUSB238::currentDetected	(	HUSB238_PDSelection 	pd	)	
Reads the detected current based on the PD output selection.

Parameters
pd	The PD selection as an HUSB238_PDSelection enum value.
Returns
The detected current as an HUSB238_CurrentSetting enum value.
This function reads the bottom four bits (0-3) of the corresponding register based on the PD selection to get the detected current. It returns the current as an HUSB238_CurrentSetting enum value.

◆ getPDSrcVoltage()
HUSB238_VoltageSetting Adafruit_HUSB238::getPDSrcVoltage	(		)	
Reads the source voltage from the HUSB238 device.

Returns
The source voltage as an HUSB238_VoltageSetting enum value.
This function reads bits 4-7 of the HUSB238_PD_STATUS0 register to get the source voltage. It returns the voltage as an HUSB238_VoltageSetting enum value.

◆ getPDSrcCurrent()
HUSB238_CurrentSetting Adafruit_HUSB238::getPDSrcCurrent	(		)	
Reads the source current from the HUSB238 device.

Returns
The source current as an HUSB238_CurrentSetting enum value.
This function reads the bottom four bits (0-3) of the HUSB238_PD_STATUS0 register to get the source current. It returns the current as an HUSB238_CurrentSetting enum value.

◆ getSelectedPD()
HUSB238_PDSelection Adafruit_HUSB238::getSelectedPD	(		)	
Gets the selected PDO.

Returns
The selected PD as an HUSB238_PDSelection enum value.
This function reads bits 4-7 of the SRC_PDO register to get the selected PDO. It returns the PDO as an HUSB238_PDSelection enum value.

◆ selectPD()
void Adafruit_HUSB238::selectPD	(	HUSB238_PDSelection 	pd	)	
Selects a PD output.

Parameters
pd	The PD selection as an HUSB238_PDOelection enum value.
This function writes to bits 4-7 of the SRC_PDO register to select a PD.

◆ reset()
void Adafruit_HUSB238::reset	(		)	
Resets the HUSB238 device.

This function writes to the GO_COMMAND register to initiate a hard reset. Specifically, it writes 0b10000 to the bottom 5 bits of the GO_COMMAND register.

See also
GO_COMMAND register in HUSB238 Register Information (Page 7)
◆ requestPD()
void Adafruit_HUSB238::requestPD	(		)	
Requests Power Delivery (PD) from the HUSB238 device.

This function writes to the GO_COMMAND register to request a PD contract. Specifically, it writes 0b00001 to bits 0-1 of the GO_COMMAND register.

See also
GO_COMMAND register in HUSB238 Register Information (Page 7)
◆ getSourceCapabilities()
void Adafruit_HUSB238::getSourceCapabilities	(		)	
Retrieves the source capabilities of the HUSB238 device.

This function writes to the GO_COMMAND register to send out a Get_SRC_Cap command. Specifically, it writes 0b00100 to the bottom 5 bits of the GO_COMMAND register.

See also
GO_COMMAND register in HUSB238 Register Information (Page 7)
The documentation for this class was generated from the following files:
Adafruit_HUSB238.h
Adafruit_HUSB238.cpp
Generated by   doxygen 1.8.13


datasheet from hyntek :

USB Type-C Power Delivery Sink
Controller
Hynetek Semiconductor Co., Ltd. HUSB238
©2021 Hynetek Semiconductor Co., Ltd All rights reserved.
Rev. 2.0
www.hynetek.com
FEATURES
USB-IF certified
 TID: 3666
Standalone USB Power Delivery (PD) sink controller
Legacy charging sink
 Apple divider 3 detection
 BC1.2 SDP, CDP and DCP detection
Dead battery function
SOP’ communication function
3V to 25V operation range
30V voltage rating on VIN and GATE pins
25V voltage rating on CC1 and CC2 pins
I
2C access for monitoring and advanced settings
Integrated PMOS driver
VBUS over-voltage protection (OVP) and undervoltage protection (UVP)
Over-temperature protection (OTP) with
programmable thresholds
Low power consumption
APPLICATIONS
PD sink devices
USB-C cables
Wireless charger
GENERAL DESCRIPTION
The HUSB238 is a highly integrated USB Power
Delivery (PD) controller as sink role for up to 100W
power rating.
The HUSB238 is compatible with PD3.0 and Type-C
V1.4, and it can also support Apple Divider 3, BC1.2
SDP, CDP and DCP while the source is attached.
The HUSB238 can be used in electronic devices that
have legacy barrel connectors or USB micro-B
connectors for power such as IoT (Internet of Things)
devices, wireless charger, drones, smart speakers,
power tools, and other rechargeable devices.
The HUSB238 is available in 3mm x 3mm DFN-10L
and 3.9mm x 4mm SOT33-6L package options.
TYPICAL APPLICATION CIRCUIT
VIN
CC1
CC2
D+
DHUSB238
GND
VSET
ISET
SCL
SDA
GATE
GND
10Ω
1μF
49.9kΩ
5.1kΩ
VBUS
CC1
CC2
D+
D22.6kΩ 10kΩ
POWER
SYSTEM
SOC
USB-C RECEPTACLE
Figure 1. Typical Application Circuit
HUSB238 Data Sheet
©2021 Hynetek Semiconductor Co., Ltd All rights reserved. Page 2 of 15
TABLE OF CONTENTS
Features ............................................................................................................................................................................1
Applications .......................................................................................................................................................................1
General Description ..........................................................................................................................................................1
Typical Application Circuit .................................................................................................................................................1
Table of Contents ..............................................................................................................................................................2
Revision History ................................................................................................................................................................2
Pin Configuration and Function Descriptions....................................................................................................................3
Specifications ....................................................................................................................................................................4
Absolute Maximum Ratings ..............................................................................................................................................6
Thermal Resistance .......................................................................................................................................................6
ESD Caution ..................................................................................................................................................................6
Functional Block Diagram .................................................................................................................................................7
Theory of Operation ..........................................................................................................................................................8
Overview ........................................................................................................................................................................8
VIN Pin...........................................................................................................................................................................8
GATE Pin .......................................................................................................................................................................8
CC1 and CC2 Pins.........................................................................................................................................................8
VSET Pin........................................................................................................................................................................8
ISET Pin .........................................................................................................................................................................9
RDO Determination........................................................................................................................................................9
Operation With I2C Interface .............................................................................................................................................9
Legacy Charger Detection .............................................................................................................................................9
Dead Battery Function ................................................................................................................................................ 10
SOP’ Function ............................................................................................................................................................. 10
Over-Voltage Protection.............................................................................................................................................. 10
Under-Voltage Protection............................................................................................................................................ 10
Over-Temperature Protection ..................................................................................................................................... 10
Typical Application Circuits..............................................................................................................................................11
Package Outline Dimensions ......................................................................................................................................... 12
Ordering Guide............................................................................................................................................................... 14
Important Notice............................................................................................................................................................. 15
REVISION HISTORY
Version Date Descriptions
Rev. 1.0 12/2020 Initial version
Rev. 2.0 01/2021 Added SOT33-6L package information
Data Sheet HUSB238
©2021 Hynetek Semiconductor Co., Ltd All rights reserved. Page 3 of 15
PIN CONFIGURATION AND FUNCTION DESCRIPTIONS
VSET
ISET
CC1
VIN
CC2
1
2
3
4
6
5 GND
1
2
3
4
5
10
9
8
7
6
HUSB238
(DFN-10)
VIN
D+
DCC1
CC2
GATE
ISET
VSET
SCL
SDA
HUSB238
(SOT33-6L)
11
GND
Figure 2. Pin Configuration (Top View)
Table 1. Pin Function Descriptions
Pin No.
Pin Name Type1 Description
DFN-10L SOT33-6L
1 1 VIN P Power supply input. Connect this pin to VBUS of USB Type-C connector
and bias this pin via a 1μF ceramic capacitor.
2 − D+ DIO Positive line of USB 2.0 data line for Apple Divider 3 and BC1.2.
3 − D- DIO Negative line of USB 2.0 data line for Apple Divider 3 and BC1.2.
4 3 CC1 AIO Configuration line 1 used to negotiate a voltage/current with the attached
adapter.
5 4 CC2 AIO Configuration line 2 used to negotiate a voltage/current with the attached
adapter.
6 − SDA DIO I
2C communication data signal.
7 − SCL DIO I
2C communication clock signal.
8 2 VSET AI Connect a resistor to indicate the maximum voltage needed by the
system from the attached power adapter.
9 6 ISET AI Connect a resistor to indicate the maximum current needed by the
system from the attached power adapter.
10 − GATE OD Open drain gate driver output. Connect this signal to the gate of an
external PMOS through a series resistor. Leave this pin open if not used.
11 5 GND P Ground reference. All signals are referred to this pin.
1 Legend:
A = Analog Pin
P = Power Pin
D = Digital Pin
I = Input Pin
O = Output Pin
OD = Open Drain Pin
HUSB238 Data Sheet
©2021 Hynetek Semiconductor Co., Ltd All rights reserved. Page 4 of 15
SPECIFICATIONS
VIN = 5V, TA = 25°C, unless otherwise noted.
Table 2.
Parameter Symbol Test Conditions/Comments Min Typ Max Unit
POWER SUPPLY
Supply Voltage VIN 3 25 V
Supply Voltage UVLO Threshold VIN_UVLO_RISE Rising edge threshold 3.1 V
VIN_UVLO_FALL Falling edge threshold 2.9 V
Supply Current ISC_OPR VIN = 5V, CC is attached, normal
operation
3.1 mA
CC1 AND CC2 PINS
Pull-down Voltage in Dead Battery VDBL 200μA source current 0.45 1.5 V
VDBH 360μA source current 0.85 2.45 V
Pull-down resistor RD 4.6 5.1 5.6 kΩ
Voltage Threshold to Detect a DFP VTH_DEF Default current mode 0.15 0.2 0.25 V
VTH_1P5 1.5A current mode 0.61 0.66 0.70 V
VTH_3P0 3.0A current mode 1.16 1.23 1.31 V
TX Output Impedance1 RTX PD TX mode 33 48 75 Ω
Voltage Swing 1.125 V
D+/D- PINS
D- Source Voltage for 0.6V VDM_SRC 0.5 0.6 0.7 V
D+ Source Voltage for 0.6V VDP_SRC 0.5 0.6 0.7 V
Data Detect Voltage VDAT_REF 300 325 350 mV
D- Sink Current IDM_SINK 50 100 150 μA
D+ Sink Current IDP_SINK 50 100 150 μA
D+/D- Comparator Threshold for
2.7V Detection
VTH_2P7_HI High threshold for Apple divider 3 2.85 2.95 3.05 V
VTH_2P7_LO Low threshold for Apple divider 3 2.25 2.35 2.45 V
VSET AND ISET PINS
Source Current IVSET On VSET pin 95 100 105 μA
IISET On ISET pin 95 100 105 μA
Detect Debounce Time1
tDB_VSET For VSET pin 2 ms
tDB_ISET For ISET pin 2 ms
I
2C PARAMETERS
Supply Range 2.5 5.5 V
Low Level Input Voltage Apply for SDA, SCL pins 0.4 V
High Level Input Voltage Apply for SDA, SCL pins 1.4 V
Low Level Output Voltage Apply for SDA pin, 2mA load current 0.4 V
GATE PIN
Maximum Sink Current1 10 mA
Pull Low Impedance 100 300 Ω
PROTECTIONS
Over-Voltage Protection Threshold VVIN_OV Refer to VIN 115 120 125 %
OVP Debounce Time1
tDB_OV 50 μs
Under-Voltage Protection Threshold VVIN_UV Refer to VIN -2 V
UVP Debounce Time1
tDB_UV 1 ms
Over-Temperature Protection
Threshold1
Rising Threshold 150 ᴼC
Falling Threshold 130 ᴼC
1 Guaranteed by design
Data Sheet HUSB238
©2021 Hynetek Semiconductor Co., Ltd All rights reserved. Page 5 of 15
Parameter Symbol Test Conditions/Comments Min Typ Max Unit
OTP Debounce Time1
tDB_OT 100 ms
VIN Discharge Resistor RDIS 500 Ω
HUSB238 Data Sheet
©2021 Hynetek Semiconductor Co., Ltd All rights reserved. Page 6 of 15
ABSOLUTE MAXIMUM RATINGS
Table 3.
Parameter Rating
VIN, GATE −0.3V to +30V
CC1, CC2 −0.3V to +25V
D+, D− −0.3V to +12V
VSET, ISET, SDA, SCL −0.3V to +6V
Operating Temperature Range (Junction) −40°C to +125°C
Soldering Conditions JEDEC J-STD-020
Electrostatic Discharge (ESD)
Human Body Mode (HBM) ±6000V
Stresses at or above those listed under Absolute Maximum Ratings may cause permanent damage to the product.
This is a stress rating only; functional operation of the product at these or any other conditions above those indicated
in the operational section of this specification is not implied. Operation beyond the maximum operating conditions for
extended periods may affect product reliability.
THERMAL RESISTANCE
Thermal performance is directly linked to printed circuit board (PCB) design and operating environment. Close
attention to PCB thermal design is required.
θJA is the natural convection junction to ambient thermal resistance measured in a one cubic foot sealed enclosure.
θJC is the junction to case thermal resistance.
Table 4. Thermal Resistance
Package Type θJA θJC Unit
DFN-10L 75 54 °C/W
SOT33-6L 89 27 °C/W
ESD CAUTION
Electrostatic Discharge Sensitive Device.
Charged devices and circuit boards can discharge without detection. Although this product features patented
or proprietary protection circuitry, damage may occur on devices subjected to high energy ESD. Therefore,
proper ESD precautions should be taken to avoid performance degradation or loss of functionality.
Data Sheet HUSB238
©2021 Hynetek Semiconductor Co., Ltd All rights reserved. Page 7 of 15
FUNCTIONAL BLOCK DIAGRAM
APPLE MODE
BC1.2
DETECTION
USB PD
PHYSICAL
LAYER
USB PD
PROTOCAL
LAYER
PORT
CONTROLLER
I2C ENGINE
POR
CIRCUIT VOLTAGE
REGULATOR
OTP
VREG
IVSET
IISET
OV & UV
PROTECTION
GATE
DRIVER
RDIS
SDA
SCL
VSET
ISET
VIN GATE
GND
D+
DCC1
CC2
Figure 3. HUSB238 Functional Block Diagram
HUSB238 Data Sheet
©2021 Hynetek Semiconductor Co., Ltd All rights reserved. Page 8 of 15
THEORY OF OPERATION
OVERVIEW
The HUSB238 is a highly integrated USB Power Delivery (PD) controller as sink role. It’s compatible with PD3.0 and
Type-C V1.4. It can also support Apple Divider 3, BC1.2 SDP, DCP and CDP while source is attached. When
HUSB238 is connected to power source, it applies Rd to both CC lines, trying to establish USB Type-C connection.
After the USB Type-C connection is established, it monitors the CC lines to get source capabilities pack from USB PD
source. If there is valid source capabilities pack before time out, the HUSB238 policy engine requests a power supply
with voltage no greater than the programmed request voltage. If there is no valid source capabilities pack after time
out, the HUSB238 switches to Apple divider 3 or BC1.2 mode trying to determine corresponding charging protocol.
VIN PIN
VIN pin is the power supply input of the HUSB238, which is derived from the output of the PD source. Connect a 1µF
decoupling MLCC between VIN pin and GND pin as closer as possible.
The VIN pin is also connected to an internal MOSFET and 500Ohm discharging resistor, which is used as a bleeder to
help discharge the output capacitor to vSafe5V upon the hard reset, over-voltage fault, over-temperature fault or
detachment of a connected device.
GATE PIN
The GATE pin is open–drain output which allows to drive an external PMOS load switch directly. The GATE pin can be
programmed to turn on after POR or after the explicit contract. The default option is to turn on after POR. Please
contact local Hynetek sales for a device with options other than the default option.
CC1 AND CC2 PINS
CC1 and CC2 are the Configuration Channel pins used for connection and attachment detection, plug orientation
determination and system configuration management across USB Type-C cable. CC1 and CC2 pins can support as
high as 25V voltage, which is used for protection when CC1 or CC2 is shorted to VBUS pin on the connector.
Through the Type-C detection, one of the CC pin is connected to the internal BMC block to achieve PD
communication.
VSET PIN
A fixed 100μA current source is applied on VSET pin. Connect a resister between VSET and GND to indicate the
VSET_VOLTAGE value as shown in Table 5.
Table 5. VSET_VOLTAGE Setting
RVSET (kΩ) VSET_VOLTAGE (V)
0 5
6.04 9
10 12
14 15
17.8 18
Open 20
The RDO voltage of the HUSB238 is determined by the lower value between VSET_VOLTAGE and
SNK_PDO2_VOLTAGE. SNK_PDO2_VOLTAGE is programmable by internal fuse options and the default value is
20V. The requested voltage value can be changed dynamically with the resistance value change.
Data Sheet HUSB238
©2021 Hynetek Semiconductor Co., Ltd All rights reserved. Page 9 of 15
ISET PIN
A fixed 100μA current source is applied on ISET pin. Connect a resister between ISET and GND to indicate the
ISET_CURRENT value as shown in Table 6.
Table 6. ISET_CURRENT Setting
RISET (kΩ) ISET_CURERNT (A)
0 1.25
4.53 1.5
7.5 1.75
10.5 2
13.7 2.25
16.5 2.5
19.6 2.75
22.6 3
Open 3.25
The RDO current of the HUSB238 is determined by the lower value between ISET_CURRENT and
SNK_PDO2_CURRENT. SNK_PDO2_CURRENT is programmable by internal fuse options and the default value is
3.25A. The requested current value can be changed dynamically with the resistance value change.
RDO DETERMINATION
There are two ways to determine the RDO that the HUSB238 requests from the PD source. Set by the VSET and
ISET pins or set by the internal factory programmed fuse options. The HUSB238 compares the two values and uses
the lower value as its RDO.
For example, if the VSET and ISET is configured as 9V / 3A. The internal factory fuse option is 12V / 2A. Then the
RDO that HUSB238 requests from the PD source is 9V / 2A.
After the RDO is determined, the HUSB238 loops through the PD source PDOs from highest voltage first to find the
first PDO that satisfies the following conditions:
1. SOURCE_PDO_VOLTAGE ≤ RDO_VOLTAGE
2. SOURCE_PDO_CURRENT ≥ RDO_CURRENT
If both the conditions above are satisfied, then HUSB238 sends a request for this source PDO with operating current
set to the RDO current value.
If either one of the condition is not satisfied, the HUSB238 continues to compare with the second highest voltage
source PDO or requests 5V source PDO directly, depending on the internal fuse options. The default fuse option is to
continue to compare with the second highest voltage source PDO. Please contact local Hynetek sales for a device
with options other than the default option.
OPERATION WITH I2C INTERFACE
The HUSB238 is I2C communication capable with system MCU or processor through SDA and SCL pins. The
HUSB238 works as I2C slave role and the I2C address is 0x08.
After POR (Power On Reset), the HUSB238 receives the source capability information from the PD source adapter
and the HUSB238 saves the source capability information in registers. The system MCU can visit the HUSB238
registers through the I
2C bus and select a proper PDO to request from external PD source.
The I2C has the highest priority. If using I2C to select a source PDO, it over writes the internal RDO which is created
by VSET, ISET pins and internal factory fuse option, and the HUSB238 requests the I2C selected source PDO once
the I2C commands are written.
LEGACY CHARGER DETECTION
After the power on reset, if the HUSB238 does not establish PD contract with source adapter, the HUSB238 will waits
for 1.5 seconds and then switches to Apple Divider 3 and BC1.2 detections sequentially. For BC1.2 detection, the
HUSB238 detects SDP (Standard Data Port), CDP (Charging Data Port) and DCP (Dedicated Charging Port)
sequentially.
HUSB238 Data Sheet
©2021 Hynetek Semiconductor Co., Ltd All rights reserved. Page 10 of 15
DEAD BATTERY FUNCTION
The HUSB238 works as PD sink role which requires RD resistor to be presented on the CC pins even in the unpowered state for successful Type-C detection by source adapter.
The dead battery function in the HUSB238 supports default USB, 1.5A and 3.0A source broadcast RP current.
SOP’ FUNCTION
For the USB-C to legacy PC plug applications where high current and high voltage are required, the system needs
current more than 3A in most of the cases. If the SOP’ function is enabled, the HUSB238 is capable to reply SOP’
command, such as Discover Identity, sent by the PD source adapter.
The SOP’ function emulates the E-Marker function in the cable so that the system is capable of obtaining more than
3A charging current from PD source adapter.
OVER-VOLTAGE PROTECTION
The HUSB238 senses the voltage on VIN pin for overvoltage protection. The over-voltage threshold is 1.2 times of
max requested voltage. The OV debounce time is 50μs.
When OV happens, the HUSB238 turns off the external PMOS and enters into discharge mode where the internal
discharge circuit on VIN pin is turned on. The internal 5.1k Rd resistor is also disconnected during discharge mode.
After the discharge timeout, the HUSB238 enters into unattached mode and waits for re-connection with PD source
adapter.
UNDER-VOLTAGE PROTECTION
The HUSB238 senses the voltage on VIN pin for optional under-voltage protection. The under-voltage threshold is the
requested voltage minus 2V. For example, if the requested PDO voltage is 12V, then the UV threshold is 10V. The UV
debounce time is 1ms.
When UV happens, the HUSB238 turns off the external PMOS and enters into discharge mode where the internal
discharge circuit on VIN pin is turned on. The internal 5.1k Rd resistor is also disconnected during discharge mode.
After the discharge timeout, the HUSB238 enters into unattached mode and waits for re-connection with PD source
adapter.
The UVP function of the HUSB238 can be enabled or disabled by internal fuse options. The UVP function is disabled
for default option. Please contact local Hynetek sales for a device with options other than the default option.
OVER-TEMPERATURE PROTECTION
The HUSB238 integrates over-temperature protection function. It monitors the internal junction temperature. When the
junction temperature reaches the over temperature rising threshold, the HUSB238 requests 5V source PDO directly,
regardless of the previous established PDO, to reduce the total system power. When the junction temperature falls
below the over temperature falling threshold, the HUSB238 re-negotiates the previous PDO and tries to recover the
normal charging operation.
The over temperature thresholds are programmable by internal factory fuses. Please contact local Hynetek sales for a
device with options other than the default option.
Data Sheet HUSB238
©2021 Hynetek Semiconductor Co., Ltd All rights reserved. Page 11 of 15
TYPICAL APPLICATION CIRCUITS
VIN
CC1
CC2
D+
DHUSB238
DFN-10
GND
VSET
ISET
SCL
SDA
GATE
GND
1μF
49.9kΩ
5.1kΩ
VBUS
CC1
CC2
D+
DDC-DC Reg
Or Charger
SOC USB-C RECEPTACLE
+
BATTERY
Figure 4. USB-C Device Application
VIN
CC1
CC2
HUSB238
SOT33-6
GND
VSET
ISET
GND
1μF
VBUS
CC1
CC2 USB-C CONNECTOR
VIN
LEGACY CHARGING
CONNECTOR
GND
1kΩ
Figure 5. USB-C Cable Application
HUSB238 Data Sheet
©2021 Hynetek Semiconductor Co., Ltd All rights reserved. Page 12 of 15
PACKAGE OUTLINE DIMENSIONS
Figure 6. DFN-10L Package, 3 mm × 3 mm
Data Sheet HUSB238
©2021 Hynetek Semiconductor Co., Ltd All rights reserved. Page 13 of 15
Figure 7. SOT33-6L Package, 3.9 mm x 4.0 mm
HUSB238 Data Sheet
©2021 Hynetek Semiconductor Co., Ltd All rights reserved. Page 14 of 15
ORDERING GUIDE
Model Package SOP’ OTP Threshold RDO Mismatch Action Package Option
HUSB238_001DD DFN-10L YES 90°C / 75°C Request 5V Tape & Reel, 4k
HUSB238_002DD DFN-10L NO 150°C / 130°C Next PDO Tape & Reel, 4k
HUSB238_002SH SOT33-6L NO 150°C / 130°C Next PDO Tape & Reel, 7.5k
HUSB238_003DD DFN-10L YES 90°C / 75°C Next PDO Tape & Reel, 4k
HUSB238_003SH SOT33-6L YES 90°C / 75°C Next PDO Tape & Reel, 7.5k
HUSB238_004DD DFN-10L NO 150°C / 130°C Request 5V Tape & Reel, 4k
Data Sheet HUSB238
©2021 Hynetek Semiconductor Co., Ltd All rights reserved. Page 15 of 15
