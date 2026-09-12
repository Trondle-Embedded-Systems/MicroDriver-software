# MicroDriver door control

Use `microdriver_door.yaml` with this checkout's `components/` directory. The door
configuration now uses local components so firmware builds actually include these
fixes. Copy both to your ESPHome configuration directory if you build elsewhere.
Provide your own `secrets.yaml`; the validation build uses dummy credentials and
must not be installed as your normal firmware.

## Why the old behavior was unreliable

Every command previously targeted +1,000,000,000 or -1,000,000,000 steps. The motor
never approached its software target, so it could not decelerate at the physical
endpoint. StallGuard was the only normal stop condition, and every stall was
reported as OPEN or CLOSED, including obstructions and startup false detections.
Stop also left the delayed movement script alive, allowing a stopped door to
restart after the direction-change delay.

The new configuration moves between taught step positions. It ramps up and down,
disables the motor on arrival, and treats detected contact as a fault. Normal
Open/Close commands are refused until travel has been taught. Boot never moves
the door. A Stop during the 250 ms start delay cancels that pending movement.

## Teach the travel

Keep the path clear and supervise commissioning. StallGuard is load detection,
not a person-detection sensor; it cannot reliably detect every obstruction.

1. Press **Stop Door** and place the released door at its desired closed position
   by hand. Do not force the mechanism beyond its stop.
2. Press **Teach Closed Position**. This defines zero and clears the old stroke.
3. Press **Jog Open 100 Steps** once and verify that positive motion opens the
   door. If direction is wrong, stop and invert `dir_pin` in YAML before proceeding.
4. Continue jogging toward the desired open position. Each press moves at most
   100 steps at 200 steps/s. **Stop Door** can interrupt a jog for finer placement;
   **Jog Close 100 Steps** corrects an overshoot. Stop short of mechanical contact.
   Do not move the door by hand during this measurement.
5. Press **Teach Open Position**. The controller records the current step count
   as the open endpoint. The commissioning range is 0 to 30,000 steps.
6. Test **Close Door**, then **Open Door**. Both should slow before the taught
   endpoint and release the motor on arrival.

Travel is deliberately not restored after a reboot: step counts cannot determine
where a released door actually is after power loss. Repeat teaching after a
reboot, driver fault, stall, timeout, or any manual displacement. Pressing Stop
without a fault retains the reference, provided the door does not coast or move.

## Commands and faults

- A new command cancels the previous move, stops immediately, waits 250 ms, and
  starts a fresh acceleration ramp. Reversal is an immediate stop followed by a
  ramp; it is not a mechanically controlled braking maneuver.
- Normal travel has a 30 second timeout, adjustable with `door_max_run_time`.
- StallGuard contact, a driver reset/error, charge-pump undervoltage, or
  overtemperature stops movement and invalidates the taught positions. A fault
  never publishes an endpoint as successfully reached.
- The timer stops STEP generation if the main loop has not serviced it for
  250 ms. When the main loop resumes, the driver is disabled and a fault is
  reported. This timer guard stops pulses; it cannot remove coil current while
  the CPU/main loop is unresponsive.
- The cover remains `assumed_state`: its last reported endpoint is not a live
  position measurement after a Stop or fault.

## Tuning and remaining hardware limits

The defaults are 8 microsteps, interpolation enabled, StealthChop forced on,
400 steps/s normal speed, 400 steps/s^2 acceleration/deceleration, 0.5 A run
current, and StallGuard threshold 50. Tune current to the actual motor rating
and mechanism; these defaults are not evidence of an appropriate end-stop force.

The speed control permits 200 to 1000 steps/s. StallGuard is speed- and
load-dependent, particularly during acceleration and the final slowdown. A false
stall stops the door and requires re-teaching; reduce sensitivity only after
checking the mechanism and current. Do not use StallGuard as a safety interlock.

The motor is released at rest. Back-driving, coasting, missed steps, and manual
movement can invalidate step-based position without being detected. Reliable
absolute endpoint verification needs physical end-stop sensing or suitable
position feedback. This software does not turn StallGuard into such a sensor.

Check the ESP32-S3 module marking: GPIO34/GPIO35 are connected to this board's
DIR/STEP nets, but are reserved by octal PSRAM on S3R8/S3R8V variants. Do not change
these pins arbitrarily; verify that the fitted module supports the board wiring.

## Acceptance checks on the mechanism

1. Boot and verify no movement or holding torque.
2. Verify normal Open/Close refuses motion before teaching.
3. Teach the stroke and run supervised open/close cycles at the intended speed.
4. Send Open then Stop within 250 ms; verify there is no delayed restart.
5. Send Stop during travel; verify movement stops and torque is released.
6. Reverse during travel; verify the stop/pause/new ramp and final position.
7. In a controlled unloaded setup, trigger StallGuard; verify fault handling
   rather than a false successful endpoint. Normal commands must then be refused.
8. Temporarily shorten `door_max_run_time` below the movement duration; verify
   timeout, release, and refusal of subsequent normal moves until re-teaching.
9. Reboot and confirm the reference is invalidated and the door stays still.

A firmware build and simulated tests cannot establish actual noise, available
torque, supply stability, or collision detection. Verify these on the hardware.
