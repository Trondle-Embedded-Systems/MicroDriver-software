# Recording StallGuard along the door travel

These diagnostics target `microdriver_betea_1.yaml` and the TMC2209 component in
this checkout. They do not move the motor or change the sensitivity. The YAML
normally fetches the component from GitHub `main`; a build must use the updated
component in this checkout as well as the updated YAML. For a local build, copy
`components/` alongside the YAML and change the external component source to
`type: local` with `path: components`. For a Home Assistant build using the Git
source, make the component changes available at the chosen Git ref first and
copy the updated YAML to the ESPHome configuration. Do not flash a build made
with dummy secrets.

1. Install the matching firmware using your usual ESPHome workflow. Open
   `http://microdriver.local` in a browser (no sign-in password is required).
   **StallGuard Recording** is enabled by default on boot, and the top of the
   `microdriver.local` web page displays the live **MicroDriver StallGuard &
   Motion Analyzer** with 3 synchronized charts (`SG_RESULT` vs trigger level,
   Speed vs gates, Position + Acceleration + Motion Phase), a plain-English
   trigger diagnosis banner, and an event table (`SGEVT`).
2. You can also export the captured trace directly to CSV from the
   `microdriver.local` web interface (`Export CSV` button), or save ESPHome logs
   to a text file. Run supervised
   opening and closing movements at the intended speed, with the door path clear.
   Keep normal stop controls and obstacle detection available; this recording is
   not a safety system. Stop recording when finished.
3. On a computer with Python 3, run:

   ```sh
   python3 tools/stallguard_graph.py path/to/esphome-log.txt --output stallguard-trace.html
   ```

   Open the resulting HTML file in a browser. It plots the raw samples against
   the step counter, colors directions separately, shows the software trigger
   level, and displays details on hover. A CSV with the same base name contains
   every sample. The diagnostic entities **StallGuard Result**, **StallGuard
   Software Trigger Level**, and **StallGuard Sample Position** can also be
   graphed over time in Home Assistant history while recording is enabled.

Each `SGTRACE` record includes uptime in milliseconds, position in steps,
direction (+1/-1), estimated commanded speed in steps/s, raw `SG_RESULT`,
software sensitivity, the driver's shadow `SGTHRS` and `TCOOLTHRS`, whether
normal-move obstacle detection is armed, and whether position is known. A failed
UART read is logged and omitted instead of being mistaken for zero. The position
is sampled immediately after the UART read, not at the exact measurement time.
The driver registers for threshold and TCOOLTHRS are write-only; the trace uses
the firmware's shadow values, not a hardware readback. After a reset or failed
write these may not match the chip.

Lower `SG_RESULT` generally indicates greater load. The software compares it to
**twice** the sensitivity setting, but a point below the line does not by itself
mean a stop should occur: normal moves require the detector to be armed and
multiple consecutive low readings. Homing has different rules and may stop on
DIAG or the first valid low reading. The hardware DIAG threshold uses the driver
shadow threshold and is additionally speed-gated by TCOOLTHRS. The configured
sensitivity may differ from that hardware threshold outside homing. Readings at
standstill, low speed, acceleration and deceleration may be unreliable; compare
like speeds and directions, and repeat runs before adjusting sensitivity.

The 200 ms trace is much slower than the protection checks and may miss a short
dip or the sample that stopped motion. It introduces extra UART reads and log
traffic, which may affect timing; turn recording off after diagnosis. Position
is a commanded step count and may be wrong after missed steps or manual movement;
`position_known=0` marks an unreferenced counter. Do not infer a safe obstruction
force or choose a final threshold from this graph alone.
