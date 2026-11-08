// MicroDriver StallGuard & Motion Telemetry Dashboard (injected via /0.js on microdriver.local)
(function () {
  if (window.__microdriverSgDashboardLoaded) return;
  window.__microdriverSgDashboardLoaded = true;

  const PHASE_NAMES = {
    0: "IDLE",
    1: "ACCEL",
    2: "CRUISE",
    3: "DECEL",
    4: "BRAKING",
    5: "SEEK_PROBE",
    6: "SEEK_FAST",
    7: "SEEK_SLOW",
    8: "HOMING",
  };

  const PHASE_COLORS = {
    0: "#64748b",
    1: "#10b981",
    2: "#3b82f6",
    3: "#f59e0b",
    4: "#ef4444",
    5: "#a855f7",
    6: "#ec4899",
    7: "#06b6d4",
    8: "#eab308",
  };

  const STOP_NAMES = {
    0: "NONE (Moving / Ready)",
    1: "TARGET_REACHED (Normal stop)",
    2: "USER_STOP (Manual stop)",
    3: "OBSTACLE_SG_UART (3x SG_RESULT <= 2*SGTHRS)",
    4: "SEEK_SG_UART (1st SG_RESULT <= 2*SGTHRS after 32 steps)",
    5: "SEEK_DIAG_PIN (Hardware DIAG pin rose during Home seek)",
    6: "HOMING_SG_UART (Auto-homing SG_RESULT <= 2*SGTHRS)",
    7: "DRIVER_RESET (TMC2209 supply/power reset)",
    8: "DRIVER_ERROR (TMC2209 DRV_ERR / UV_CP / OT)",
    9: "LOOP_TIMEOUT (Main loop blocked >250ms)",
  };

  const samples = [];
  const events = [];
  const MAX_SAMPLES = 4000;
  const MAX_EVENTS = 150;

  let paused = false;
  let xAxisMode = "time"; // "time" | "position"
  let timeWindowSec = 30; // 15 | 30 | 60 | 0 (all)
  let hoverIndex = -1;
  let collapsed = false;
  let sseConnected = false;
  let rafPending = false;

  const TRACE_RE =
    /SGTRACE,(\d+),(-?\d+),(-?\d+),([\d.-]+),(-?\d+),(\d+),(-?\d+),(-?\d+),([01]),([01])(?:,(-?\d+),([\d.-]+),([\d.-]+),(\d+),(\d+),(-?\d+),([01]),([01]),([\d.-]+),(\d+))?/;
  const EVT_RE =
    /SGEVT,(\d+),([A-Z0-9_]+),(-?\d+),(-?\d+),([\d.-]+),([\d.-]+),([\d.-]+),(-?\d+),(\d+),(\d+),(-?\d+),([01]),(\d+),([^\r\n\x1b"]+)/;

  function parseLogMessage(raw) {
    if (!raw || typeof raw !== "string") return;
    const clean = raw.replace(/\x1b\[[0-9;]*m/g, "");

    const tm = TRACE_RE.exec(clean);
    if (tm) {
      const s = {
        ms: +tm[1],
        position: +tm[2],
        direction: +tm[3],
        speed: +tm[4],
        result: +tm[5],
        software_sgthrs: +tm[6],
        driver_sgthrs: +tm[7],
        tcoolthrs: +tm[8],
        obstacle_armed: +tm[9],
        position_known: +tm[10],
        target_pos: tm[11] !== undefined ? +tm[11] : +tm[2],
        max_speed: tm[12] !== undefined ? +tm[12] : +tm[4],
        accel: tm[13] !== undefined ? +tm[13] : 0,
        phase: tm[14] !== undefined ? +tm[14] : 0,
        consec_stalls: tm[15] !== undefined ? +tm[15] : 0,
        steps_since_arm: tm[16] !== undefined ? +tm[16] : 0,
        diag_pin: tm[17] !== undefined ? +tm[17] : 0,
        obstacle_enabled: tm[18] !== undefined ? +tm[18] : 1,
        min_speed: tm[19] !== undefined ? +tm[19] : 0,
        stop_code: tm[20] !== undefined ? +tm[20] : 0,
      };
      if (!paused) {
        samples.push(s);
        if (samples.length > MAX_SAMPLES) samples.shift();
        scheduleRender();
      }
      return;
    }

    const em = EVT_RE.exec(clean);
    if (em) {
      const ev = {
        ms: +em[1],
        code: em[2],
        position: +em[3],
        target_pos: +em[4],
        speed: +em[5],
        max_speed: +em[6],
        accel: +em[7],
        result: +em[8],
        trigger_level: +em[9],
        consec_stalls: +em[10],
        steps_since_arm: +em[11],
        diag_pin: +em[12],
        phase: +em[13],
        reason: em[14].trim(),
      };
      if (!paused) {
        events.unshift(ev);
        if (events.length > MAX_EVENTS) events.pop();
        scheduleRender();
      }
    }
  }

  function attachToEventSource(es) {
    if (!es || es.__sgHooked) return;
    es.__sgHooked = true;
    sseConnected = true;
    es.addEventListener("open", () => {
      sseConnected = true;
      scheduleRender();
    });
    es.addEventListener("error", () => {
      scheduleRender();
    });
    es.addEventListener("log", (e) => {
      sseConnected = true;
      parseLogMessage(e.data);
    });
    es.addEventListener("ping", () => {
      sseConnected = true;
    });
    scheduleRender();
  }

  // Hook window.EventSource so we share the exact stream opened by <esp-app>
  const NativeEventSource = window.EventSource;
  let capturedEs = null;
  if (NativeEventSource) {
    function PatchedEventSource(url, dict) {
      const instance = new NativeEventSource(url, dict);
      if (String(url).includes("events")) {
        capturedEs = instance;
        attachToEventSource(instance);
      }
      return instance;
    }
    PatchedEventSource.prototype = NativeEventSource.prototype;
    PatchedEventSource.CONNECTING = NativeEventSource.CONNECTING;
    PatchedEventSource.OPEN = NativeEventSource.OPEN;
    PatchedEventSource.CLOSED = NativeEventSource.CLOSED;
    window.EventSource = PatchedEventSource;

    // Fallback if <esp-app> didn't load (e.g. offline without oi.esphome.io) or already opened EventSource
    setTimeout(() => {
      if (!capturedEs) {
        try {
          capturedEs = new NativeEventSource("/events");
          attachToEventSource(capturedEs);
        } catch (err) {
          console.warn("SG Dashboard fallback EventSource error:", err);
        }
      }
    }, 900);
  }

  function triggerButton(slug) {
    fetch("/button/" + slug + "/press", { method: "POST" }).catch((err) =>
      console.warn("Button press failed:", err)
    );
  }

  function buildVerdict(latestSample, latestStallEvt) {
    if (latestStallEvt) {
      const ph = PHASE_NAMES[latestStallEvt.phase] || latestStallEvt.phase;
      const isStall =
        latestStallEvt.code.includes("SG") ||
        latestStallEvt.code.includes("DIAG") ||
        latestStallEvt.code.includes("OBSTACLE") ||
        latestStallEvt.code.includes("RESET") ||
        latestStallEvt.code.includes("ERROR");

      let diagnosis = "";
      if (latestStallEvt.code === "SEEK_DIAG_PIN") {
        diagnosis =
          `Hardware DIAG pin (GPIO4) rose during ${ph} at pos=${latestStallEvt.position} steps (${latestStallEvt.steps_since_arm} steps into seek), ` +
          `speed=${latestStallEvt.speed.toFixed(0)}/${latestStallEvt.max_speed.toFixed(0)} steps/s, accel=${latestStallEvt.accel >= 0 ? "+" : ""}${latestStallEvt.accel.toFixed(0)} steps/s², ` +
          `SG_RESULT=${latestStallEvt.result} vs trigger <=${latestStallEvt.trigger_level}. ` +
          (latestStallEvt.steps_since_arm < 32
            ? `NOTE: Triggered before 32 steps because hardware DIAG is NOT gated by ENDSTOP_STALL_ARM_STEPS!`
            : latestStallEvt.accel > 0
            ? `NOTE: Triggered while accelerating (accel > 0); motor load during ramp-up pushed SG_RESULT below threshold.`
            : `Motor load reached hardware SGTHRS threshold.`);
      } else if (latestStallEvt.code === "SEEK_SG_UART") {
        diagnosis =
          `UART poll endstop_stalled_() triggered during ${ph} at pos=${latestStallEvt.position} steps (${latestStallEvt.steps_since_arm} steps >= 32 arm threshold), ` +
          `speed=${latestStallEvt.speed.toFixed(0)}/${latestStallEvt.max_speed.toFixed(0)} steps/s, accel=${latestStallEvt.accel >= 0 ? "+" : ""}${latestStallEvt.accel.toFixed(0)} steps/s², ` +
          `SG_RESULT=${latestStallEvt.result} <= trigger ${latestStallEvt.trigger_level}. ` +
          (latestStallEvt.accel !== 0
            ? `NOTE: Home seek stops on the 1st low UART sample even during acceleration/deceleration (${latestStallEvt.accel >= 0 ? "+" : ""}${latestStallEvt.accel.toFixed(0)} steps/s²)!`
            : `Single UART sample <= 2*SGTHRS stopped seek.`);
      } else if (latestStallEvt.code === "OBSTACLE_SG_UART") {
        diagnosis =
          `Obstacle detection stopped normal move during ${ph} at pos=${latestStallEvt.position} steps (${latestStallEvt.steps_since_arm} steps since cruise arm), ` +
          `speed=${latestStallEvt.speed.toFixed(0)}/${latestStallEvt.max_speed.toFixed(0)} steps/s, accel=${latestStallEvt.accel >= 0 ? "+" : ""}${latestStallEvt.accel.toFixed(0)} steps/s², ` +
          `SG_RESULT=${latestStallEvt.result} <= trigger ${latestStallEvt.trigger_level} for ${latestStallEvt.consec_stalls}/3 consecutive 10ms polls.`;
      } else {
        diagnosis =
          `${latestStallEvt.reason} — Phase=${ph}, pos=${latestStallEvt.position} (target=${latestStallEvt.target_pos}), ` +
          `speed=${latestStallEvt.speed.toFixed(0)} steps/s, accel=${latestStallEvt.accel >= 0 ? "+" : ""}${latestStallEvt.accel.toFixed(0)} steps/s², ` +
          `SG_RESULT=${latestStallEvt.result} (trigger <=${latestStallEvt.trigger_level}).`;
      }

      return {
        isAlert: isStall,
        title: `Last Event [${ (latestStallEvt.ms / 1000).toFixed(2) }s]: ${latestStallEvt.code}`,
        text: diagnosis,
      };
    }

    if (latestSample) {
      return {
        isAlert: false,
        title: "Live Telemetry Ready",
        text: "Run a movement (Home +, Home -, Move +/-1000, or Target Position) to record speed, acceleration, position, and StallGuard trigger conditions.",
      };
    }

    return {
      isAlert: false,
      title: "Waiting for Movement Telemetry...",
      text: "Connected to microdriver.local stream. Press any movement button below (e.g. Home +, Home -, +1000 Steps) to graph real-time motion & StallGuard diagnostics.",
    };
  }

  let rootEl,
    verdictEl,
    cardsEl,
    hoverInfoEl,
    evtBodyEl,
    statusBadgeEl,
    canvasSg,
    canvasSpd,
    canvasPos,
    bodyWrapEl;

  function createDashboardDOM() {
    if (document.getElementById("sg-analyzer-root")) return;

    const style = document.createElement("style");
    style.textContent = `
      #sg-analyzer-root {
        font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
        background: #0f172a;
        color: #e2e8f0;
        margin: 0;
        padding: 14px 18px;
        border-bottom: 2px solid #334155;
        box-sizing: border-box;
      }
      #sg-analyzer-root * { box-sizing: border-box; }
      .sg-hdr {
        display: flex;
        flex-wrap: wrap;
        align-items: center;
        justify-content: space-between;
        gap: 10px;
        margin-bottom: 10px;
      }
      .sg-title {
        font-size: 17px;
        font-weight: 700;
        color: #f8fafc;
        display: flex;
        align-items: center;
        gap: 10px;
      }
      .sg-badge {
        font-size: 12px;
        padding: 3px 8px;
        border-radius: 999px;
        font-weight: 600;
        background: #1e293b;
        border: 1px solid #475569;
      }
      .sg-toolbar {
        display: flex;
        flex-wrap: wrap;
        gap: 6px;
        align-items: center;
      }
      .sg-btn, .sg-sel {
        background: #1e293b;
        color: #e2e8f0;
        border: 1px solid #475569;
        border-radius: 6px;
        padding: 5px 10px;
        font-size: 12px;
        font-weight: 600;
        cursor: pointer;
      }
      .sg-btn:hover { background: #334155; }
      .sg-btn-act { background: #2563eb; border-color: #3b82f6; color: #fff; }
      .sg-btn-act:hover { background: #1d4ed8; }
      .sg-btn-stop { background: #dc2626; border-color: #ef4444; color: #fff; }
      .sg-btn-stop:hover { background: #b91c1c; }
      .sg-verdict {
        padding: 10px 12px;
        border-radius: 8px;
        margin-bottom: 10px;
        font-size: 13px;
        line-height: 1.45;
        background: #1e293b;
        border-left: 4px solid #3b82f6;
      }
      .sg-verdict.alert {
        background: #2a1215;
        border-left-color: #ef4444;
        color: #fecaca;
      }
      .sg-cards {
        display: grid;
        grid-template-columns: repeat(auto-fit, minmax(175px, 1fr));
        gap: 8px;
        margin-bottom: 10px;
      }
      .sg-card {
        background: #1e293b;
        border: 1px solid #334155;
        border-radius: 7px;
        padding: 8px 10px;
      }
      .sg-card-lbl {
        font-size: 11px;
        color: #94a3b8;
        text-transform: uppercase;
        letter-spacing: 0.04em;
        margin-bottom: 3px;
      }
      .sg-card-val {
        font-size: 15px;
        font-weight: 700;
        color: #f8fafc;
      }
      .sg-card-sub {
        font-size: 11px;
        color: #94a3b8;
        margin-top: 2px;
      }
      .sg-chart-box {
        background: #1e293b;
        border: 1px solid #334155;
        border-radius: 8px;
        padding: 8px 10px;
        margin-bottom: 8px;
      }
      .sg-chart-hdr {
        display: flex;
        justify-content: space-between;
        align-items: center;
        font-size: 12px;
        color: #cbd5e1;
        margin-bottom: 4px;
        flex-wrap: wrap;
        gap: 8px;
      }
      .sg-legend {
        display: flex;
        gap: 12px;
        font-size: 11px;
        color: #94a3b8;
        flex-wrap: wrap;
      }
      .sg-dot {
        display: inline-block;
        width: 9px;
        height: 9px;
        border-radius: 50%;
        margin-right: 4px;
        vertical-align: middle;
      }
      canvas.sg-cv {
        width: 100%;
        height: 155px;
        display: block;
        background: #0f172a;
        border-radius: 4px;
        cursor: crosshair;
      }
      .sg-hover-bar {
        font-size: 12px;
        font-family: ui-monospace, SFMono-Regular, Menlo, monospace;
        background: #1e293b;
        border: 1px solid #334155;
        border-radius: 6px;
        padding: 7px 10px;
        margin-bottom: 10px;
        color: #e2e8f0;
      }
      .sg-evts {
        background: #1e293b;
        border: 1px solid #334155;
        border-radius: 8px;
        padding: 8px 10px;
        max-height: 185px;
        overflow-y: auto;
      }
      .sg-evts table {
        width: 100%;
        border-collapse: collapse;
        font-size: 12px;
      }
      .sg-evts th, .sg-evts td {
        text-align: left;
        padding: 4px 6px;
        border-bottom: 1px solid #334155;
      }
      .sg-evts th { color: #94a3b8; font-weight: 600; position: sticky; top: 0; background: #1e293b; }
    `;
    document.head.appendChild(style);

    rootEl = document.createElement("div");
    rootEl.id = "sg-analyzer-root";
    rootEl.innerHTML = `
      <div class="sg-hdr">
        <div class="sg-title">
          <span>MicroDriver StallGuard &amp; Motion Analyzer</span>
          <span id="sg-status-badge" class="sg-badge">Connecting...</span>
        </div>
        <div class="sg-toolbar">
          <button class="sg-btn sg-btn-act" data-cmd="home_-">Home -</button>
          <button class="sg-btn sg-btn-act" data-cmd="home_+">Home +</button>
          <button class="sg-btn" data-cmd="move_1000_steps_backward">-1000 Steps</button>
          <button class="sg-btn" data-cmd="move_1000_steps_forward">+1000 Steps</button>
          <button class="sg-btn sg-btn-stop" data-cmd="stop_motor">STOP</button>
          <span style="color:#475569">|</span>
          <select id="sg-xaxis" class="sg-sel" title="X-Axis Mode">
            <option value="time">X-Axis: Time (s)</option>
            <option value="position">X-Axis: Position (steps)</option>
          </select>
          <select id="sg-win" class="sg-sel" title="Time Window">
            <option value="15">Window: 15s</option>
            <option value="30" selected>Window: 30s</option>
            <option value="60">Window: 60s</option>
            <option value="0">Window: All</option>
          </select>
          <button id="sg-pause" class="sg-btn">Pause</button>
          <button id="sg-clear" class="sg-btn">Clear</button>
          <button id="sg-csv" class="sg-btn">Export CSV</button>
          <button id="sg-collapse" class="sg-btn">Hide Graph</button>
        </div>
      </div>
      <div id="sg-body-wrap">
        <div id="sg-verdict" class="sg-verdict"></div>
        <div id="sg-cards" class="sg-cards"></div>
        <div id="sg-hover-info" class="sg-hover-bar">Hover over any graph to inspect exact timestamp, position, speed, acceleration, SG_RESULT, and StallGuard arming state.</div>
        <div class="sg-chart-box">
          <div class="sg-chart-hdr">
            <strong>1. StallGuard Result (SG_RESULT) vs Trigger Threshold (2 &times; SGTHRS)</strong>
            <div class="sg-legend">
              <span><i class="sg-dot" style="background:#38bdf8"></i>Forward SG_RESULT</span>
              <span><i class="sg-dot" style="background:#fb923c"></i>Backward SG_RESULT</span>
              <span><i class="sg-dot" style="background:#ef4444"></i>Trigger Level (2&times;SGTHRS)</span>
              <span><i class="sg-dot" style="background:rgba(168,85,247,0.35)"></i>Home Seek Armed</span>
              <span><i class="sg-dot" style="background:rgba(59,130,246,0.30)"></i>Obstacle Armed</span>
            </div>
          </div>
          <canvas id="sg-cv-sg" class="sg-cv"></canvas>
        </div>
        <div class="sg-chart-box">
          <div class="sg-chart-hdr">
            <strong>2. Speed (steps/s) &amp; Obstacle Speed Gate</strong>
            <div class="sg-legend">
              <span><i class="sg-dot" style="background:#10b981"></i>Current Speed (steps/s)</span>
              <span><i class="sg-dot" style="background:#94a3b8"></i>Max Speed Setting</span>
              <span><i class="sg-dot" style="background:#c084fc"></i>Arm / Min Speed Threshold</span>
            </div>
          </div>
          <canvas id="sg-cv-spd" class="sg-cv"></canvas>
        </div>
        <div class="sg-chart-box">
          <div class="sg-chart-hdr">
            <strong>3. Movement Position (steps), Acceleration (steps/s&sup2;) &amp; Motion Phase</strong>
            <div class="sg-legend">
              <span><i class="sg-dot" style="background:#60a5fa"></i>Current Position (steps)</span>
              <span><i class="sg-dot" style="background:#34d399"></i>+Acceleration (ramp up)</span>
              <span><i class="sg-dot" style="background:#f87171"></i>-Deceleration / Braking</span>
            </div>
          </div>
          <canvas id="sg-cv-pos" class="sg-cv"></canvas>
        </div>
        <div class="sg-evts">
          <table>
            <thead>
              <tr>
                <th>Time (s)</th>
                <th>Event</th>
                <th>Phase</th>
                <th>Pos / Target</th>
                <th>Speed / Max</th>
                <th>Accel</th>
                <th>SG_RESULT / Trig</th>
                <th>Consec</th>
                <th>Arm Steps</th>
                <th>DIAG</th>
                <th>Diagnostic Details</th>
              </tr>
            </thead>
            <tbody id="sg-evt-body">
              <tr><td colspan="11" style="color:#94a3b8">No movement or StallGuard events recorded yet.</td></tr>
            </tbody>
          </table>
        </div>
      </div>
    `;

    document.body.insertBefore(rootEl, document.body.firstChild);

    verdictEl = document.getElementById("sg-verdict");
    cardsEl = document.getElementById("sg-cards");
    hoverInfoEl = document.getElementById("sg-hover-info");
    evtBodyEl = document.getElementById("sg-evt-body");
    statusBadgeEl = document.getElementById("sg-status-badge");
    bodyWrapEl = document.getElementById("sg-body-wrap");
    canvasSg = document.getElementById("sg-cv-sg");
    canvasSpd = document.getElementById("sg-cv-spd");
    canvasPos = document.getElementById("sg-cv-pos");

    rootEl.querySelectorAll("[data-cmd]").forEach((btn) => {
      btn.addEventListener("click", () => triggerButton(btn.getAttribute("data-cmd")));
    });

    document.getElementById("sg-xaxis").addEventListener("change", (e) => {
      xAxisMode = e.target.value;
      scheduleRender();
    });
    document.getElementById("sg-win").addEventListener("change", (e) => {
      timeWindowSec = +e.target.value;
      scheduleRender();
    });
    const pauseBtn = document.getElementById("sg-pause");
    pauseBtn.addEventListener("click", () => {
      paused = !paused;
      pauseBtn.textContent = paused ? "Resume" : "Pause";
      scheduleRender();
    });
    document.getElementById("sg-clear").addEventListener("click", () => {
      samples.length = 0;
      events.length = 0;
      hoverIndex = -1;
      scheduleRender();
    });
    document.getElementById("sg-csv").addEventListener("click", exportCsv);
    const colBtn = document.getElementById("sg-collapse");
    colBtn.addEventListener("click", () => {
      collapsed = !collapsed;
      bodyWrapEl.style.display = collapsed ? "none" : "block";
      colBtn.textContent = collapsed ? "Show Graph" : "Hide Graph";
      if (!collapsed) scheduleRender();
    });

    [canvasSg, canvasSpd, canvasPos].forEach((cv) => {
      cv.addEventListener("mousemove", (e) => onCanvasHover(cv, e));
      cv.addEventListener("mouseleave", () => {
        hoverIndex = -1;
        scheduleRender();
      });
    });

    window.addEventListener("resize", scheduleRender);
    scheduleRender();
  }

  function getVisibleSamples() {
    if (!samples.length) return [];
    if (xAxisMode === "time" && timeWindowSec > 0) {
      const latestMs = samples[samples.length - 1].ms;
      const cutoff = latestMs - timeWindowSec * 1000;
      return samples.filter((s) => s.ms >= cutoff);
    }
    return samples;
  }

  function onCanvasHover(cv, e) {
    const vis = getVisibleSamples();
    if (!vis.length) return;
    const rect = cv.getBoundingClientRect();
    const relX = e.clientX - rect.left;
    const padL = 56,
      padR = 52;
    const plotW = Math.max(10, rect.width - padL - padR);
    const ratio = Math.max(0, Math.min(1, (relX - padL) / plotW));

    if (xAxisMode === "time") {
      const tMin = vis[0].ms;
      const tMax = Math.max(tMin + 1, vis[vis.length - 1].ms);
      const targetMs = tMin + ratio * (tMax - tMin);
      let bestIdx = 0,
        bestDist = Infinity;
      for (let i = 0; i < vis.length; i++) {
        const d = Math.abs(vis[i].ms - targetMs);
        if (d < bestDist) {
          bestDist = d;
          bestIdx = i;
        }
      }
      hoverIndex = bestIdx;
    } else {
      let pMin = Infinity,
        pMax = -Infinity;
      for (const s of vis) {
        if (s.position < pMin) pMin = s.position;
        if (s.position > pMax) pMax = s.position;
      }
      if (pMin === pMax) pMax = pMin + 1;
      const targetPos = pMin + ratio * (pMax - pMin);
      let bestIdx = 0,
        bestDist = Infinity;
      for (let i = 0; i < vis.length; i++) {
        const d = Math.abs(vis[i].position - targetPos);
        if (d < bestDist) {
          bestDist = d;
          bestIdx = i;
        }
      }
      hoverIndex = bestIdx;
    }
    scheduleRender();
  }

  function exportCsv() {
    if (!samples.length) return;
    const cols = [
      "ms",
      "position",
      "target_pos",
      "direction",
      "speed",
      "max_speed",
      "accel",
      "result",
      "software_sgthrs",
      "driver_sgthrs",
      "tcoolthrs",
      "phase",
      "obstacle_armed",
      "obstacle_enabled",
      "min_speed",
      "consec_stalls",
      "steps_since_arm",
      "diag_pin",
      "position_known",
      "stop_code",
    ];
    const lines = [cols.join(",")];
    for (const s of samples) {
      lines.push(cols.map((k) => s[k]).join(","));
    }
    const blob = new Blob([lines.join("\n")], { type: "text/csv" });
    const a = document.createElement("a");
    a.href = URL.createObjectURL(blob);
    a.download = "microdriver-stallguard-trace.csv";
    a.click();
  }

  function scheduleRender() {
    if (rafPending) return;
    rafPending = true;
    requestAnimationFrame(() => {
      rafPending = false;
      renderAll();
    });
  }

  function prepareCanvas(cv) {
    const dpr = window.devicePixelRatio || 1;
    const rect = cv.getBoundingClientRect();
    const w = Math.max(300, Math.floor(rect.width));
    const h = Math.max(120, Math.floor(rect.height));
    if (cv.width !== w * dpr || cv.height !== h * dpr) {
      cv.width = w * dpr;
      cv.height = h * dpr;
    }
    const ctx = cv.getContext("2d");
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    ctx.clearRect(0, 0, w, h);
    return { ctx, w, h, padL: 56, padR: 52, padT: 12, padB: 24 };
  }

  function renderAll() {
    if (!rootEl) return;
    const latest = samples.length ? samples[samples.length - 1] : null;
    const latestEvt = events.length ? events[0] : null;
    const phaseName = latest ? PHASE_NAMES[latest.phase] || latest.phase : "IDLE";

    statusBadgeEl.textContent = `${sseConnected ? "LIVE" : "CONNECTING"} · ${phaseName} · ${samples.length} samples`;
    statusBadgeEl.style.borderColor = latest && latest.phase !== 0 ? "#10b981" : "#475569";

    if (collapsed) return;

    const verdict = buildVerdict(latest, latestEvt);
    verdictEl.className = "sg-verdict" + (verdict.isAlert ? " alert" : "");
    verdictEl.innerHTML = `<strong>${verdict.title}:</strong> ${verdict.text}`;

    const s = latest || {
      ms: 0,
      position: 0,
      target_pos: 0,
      direction: 0,
      speed: 0,
      max_speed: 0,
      accel: 0,
      result: 0,
      software_sgthrs: 50,
      driver_sgthrs: 50,
      tcoolthrs: 0,
      phase: 0,
      obstacle_armed: 0,
      obstacle_enabled: 1,
      min_speed: 0,
      consec_stalls: 0,
      steps_since_arm: 0,
      diag_pin: 0,
      position_known: 0,
      stop_code: 0,
    };

    const trig = s.software_sgthrs * 2;
    const margin = s.result - trig;
    cardsEl.innerHTML = `
      <div class="sg-card">
        <div class="sg-card-lbl">Phase &amp; Stop Status</div>
        <div class="sg-card-val" style="color:${PHASE_COLORS[s.phase] || "#f8fafc"}">${PHASE_NAMES[s.phase] || s.phase}</div>
        <div class="sg-card-sub">${STOP_NAMES[s.stop_code] || s.stop_code}</div>
      </div>
      <div class="sg-card">
        <div class="sg-card-lbl">Position / Target</div>
        <div class="sg-card-val">${s.position} / ${s.target_pos} <span style="font-size:11px;font-weight:400">steps</span></div>
        <div class="sg-card-sub">Known: ${s.position_known ? "YES" : "NO"} · Dir: ${s.direction > 0 ? "FWD (+1)" : s.direction < 0 ? "BWD (-1)" : "IDLE (0)"}</div>
      </div>
      <div class="sg-card">
        <div class="sg-card-lbl">Speed / Max Speed</div>
        <div class="sg-card-val">${s.speed.toFixed(0)} / ${s.max_speed.toFixed(0)} <span style="font-size:11px;font-weight:400">steps/s</span></div>
        <div class="sg-card-sub">Gate/Min Speed: ${s.min_speed.toFixed(0)} steps/s</div>
      </div>
      <div class="sg-card">
        <div class="sg-card-lbl">Acceleration</div>
        <div class="sg-card-val" style="color:${s.accel > 0 ? "#34d399" : s.accel < 0 ? "#f87171" : "#e2e8f0"}">${s.accel > 0 ? "+" : ""}${s.accel.toFixed(0)} <span style="font-size:11px;font-weight:400">steps/s²</span></div>
        <div class="sg-card-sub">${s.accel > 0 ? "Ramping Up (Accelerating)" : s.accel < 0 ? "Ramping Down (Decel/Braking)" : "Constant Speed / Standstill"}</div>
      </div>
      <div class="sg-card">
        <div class="sg-card-lbl">SG_RESULT vs Trigger</div>
        <div class="sg-card-val" style="color:${s.result <= trig && s.direction !== 0 ? "#ef4444" : "#38bdf8"}">${s.result} <span style="font-size:12px;color:#94a3b8">vs &le;${trig}</span></div>
        <div class="sg-card-sub">Headroom: ${margin >= 0 ? "+" + margin : margin} (SW SGTHRS=${s.software_sgthrs})</div>
      </div>
      <div class="sg-card">
        <div class="sg-card-lbl">Stall Detector State</div>
        <div class="sg-card-val">Consec: ${s.consec_stalls}/3 · DIAG: ${s.diag_pin}</div>
        <div class="sg-card-sub">Obs Armed: ${s.obstacle_armed ? "YES" : "NO"} · Arm Steps: ${s.steps_since_arm} (&ge;32)</div>
      </div>
      <div class="sg-card">
        <div class="sg-card-lbl">Hardware Shadow Regs</div>
        <div class="sg-card-val">HW SGTHRS: ${s.driver_sgthrs}</div>
        <div class="sg-card-sub">TCOOLTHRS: ${s.tcoolthrs} · Obs Switch: ${s.obstacle_enabled ? "ON" : "OFF"}</div>
      </div>
    `;

    const vis = getVisibleSamples();
    if (hoverIndex >= 0 && hoverIndex < vis.length) {
      const h = vis[hoverIndex];
      hoverInfoEl.innerHTML =
        `<strong>Cursor [${(h.ms / 1000).toFixed(2)}s]</strong> · Phase=<strong>${PHASE_NAMES[h.phase]}</strong> · ` +
        `Pos=<strong>${h.position}</strong> (Target ${h.target_pos}) · Speed=<strong>${h.speed.toFixed(1)}</strong>/${h.max_speed.toFixed(0)} steps/s · ` +
        `Accel=<strong>${h.accel >= 0 ? "+" : ""}${h.accel.toFixed(0)}</strong> steps/s² · ` +
        `SG_RESULT=<strong>${h.result}</strong> (Trig &le;${h.software_sgthrs * 2}, HW_SGTHRS=${h.driver_sgthrs}, TCOOL=${h.tcoolthrs}) · ` +
        `Armed=${h.obstacle_armed} · StepsArm=${h.steps_since_arm} · Consec=${h.consec_stalls}/3 · DIAG=${h.diag_pin}`;
    }

    drawCharts(vis);
    renderEventsTable();
  }

  function drawCharts(vis) {
    const c1 = prepareCanvas(canvasSg);
    const c2 = prepareCanvas(canvasSpd);
    const c3 = prepareCanvas(canvasPos);

    if (!vis.length) {
      [c1, c2, c3].forEach(({ ctx, w, h }) => {
        ctx.fillStyle = "#64748b";
        ctx.font = "13px sans-serif";
        ctx.textAlign = "center";
        ctx.fillText("No motion samples yet — run a movement to see real-time telemetry", w / 2, h / 2);
      });
      return;
    }

    const plotW = c1.w - c1.padL - c1.padR;
    let xMin, xMax;
    if (xAxisMode === "time") {
      xMin = vis[0].ms;
      xMax = Math.max(xMin + 100, vis[vis.length - 1].ms);
    } else {
      xMin = Infinity;
      xMax = -Infinity;
      for (const p of vis) {
        if (p.position < xMin) xMin = p.position;
        if (p.position > xMax) xMax = p.position;
      }
      if (xMin === xMax) xMax = xMin + 1;
    }
    const xOf = (p) =>
      c1.padL +
      (((xAxisMode === "time" ? p.ms : p.position) - xMin) / (xMax - xMin)) * plotW;

    // --- CHART 1: SG_RESULT & Trigger Level ---
    {
      const { ctx, w, h, padL, padR, padT, padB } = c1;
      const plotH = h - padT - padB;
      let yMax = 260;
      for (const p of vis) {
        yMax = Math.max(yMax, p.result, p.software_sgthrs * 2 + 20, p.driver_sgthrs * 2 + 20);
      }
      const yOf = (v) => padT + plotH - (Math.max(0, v) / yMax) * plotH;

      // Grid
      ctx.strokeStyle = "#1e293b";
      ctx.fillStyle = "#94a3b8";
      ctx.font = "11px sans-serif";
      ctx.textAlign = "right";
      for (let i = 0; i <= 4; i++) {
        const val = (yMax * i) / 4;
        const yy = yOf(val);
        ctx.beginPath();
        ctx.moveTo(padL, yy);
        ctx.lineTo(w - padR, yy);
        ctx.stroke();
        ctx.fillText(val.toFixed(0), padL - 6, yy + 4);
      }

      // Armed shading bands
      for (let i = 0; i < vis.length; i++) {
        const p = vis[i];
        const isSeek = p.phase === 5 || p.phase === 6 || p.phase === 7;
        const isObsArmed = p.obstacle_armed === 1;
        if (!isSeek && !isObsArmed) continue;
        const x0 = xOf(p);
        const x1 = i + 1 < vis.length ? Math.max(x0 + 2, xOf(vis[i + 1])) : x0 + 3;
        ctx.fillStyle = isSeek ? "rgba(168,85,247,0.16)" : "rgba(59,130,246,0.14)";
        ctx.fillRect(x0, padT, x1 - x0, plotH);
      }

      // Software trigger line (2 * SGTHRS)
      ctx.strokeStyle = "#ef4444";
      ctx.setLineDash([5, 3]);
      ctx.lineWidth = 1.5;
      ctx.beginPath();
      for (let i = 0; i < vis.length; i++) {
        const xx = xOf(vis[i]);
        const yy = yOf(vis[i].software_sgthrs * 2);
        if (i === 0) ctx.moveTo(xx, yy);
        else ctx.lineTo(xx, yy);
      }
      ctx.stroke();
      ctx.setLineDash([]);

      // SG_RESULT line + points
      ctx.lineWidth = 2;
      for (let i = 1; i < vis.length; i++) {
        const p0 = vis[i - 1],
          p1 = vis[i];
        ctx.strokeStyle = p1.direction >= 0 ? "#38bdf8" : "#fb923c";
        ctx.beginPath();
        ctx.moveTo(xOf(p0), yOf(p0.result));
        ctx.lineTo(xOf(p1), yOf(p1.result));
        ctx.stroke();
      }
      for (let i = 0; i < vis.length; i++) {
        const p = vis[i];
        const xx = xOf(p),
          yy = yOf(p.result);
        const isLow = p.direction !== 0 && p.result <= p.software_sgthrs * 2;
        ctx.fillStyle = isLow ? "#ef4444" : p.direction >= 0 ? "#38bdf8" : "#fb923c";
        ctx.beginPath();
        ctx.arc(xx, yy, isLow ? 4.2 : 2.3, 0, Math.PI * 2);
        ctx.fill();
      }

      // Event markers on Chart 1
      if (xAxisMode === "time") {
        for (const ev of events) {
          if (ev.ms < xMin || ev.ms > xMax) continue;
          const xx = padL + ((ev.ms - xMin) / (xMax - xMin)) * plotW;
          const isStall =
            ev.code.includes("SG") || ev.code.includes("DIAG") || ev.code.includes("OBSTACLE");
          ctx.strokeStyle = isStall ? "#ef4444" : "#94a3b8";
          ctx.setLineDash([2, 2]);
          ctx.beginPath();
          ctx.moveTo(xx, padT);
          ctx.lineTo(xx, padT + plotH);
          ctx.stroke();
          ctx.setLineDash([]);
          ctx.fillStyle = isStall ? "#fca5a5" : "#cbd5e1";
          ctx.font = "10px sans-serif";
          ctx.textAlign = "left";
          ctx.fillText(ev.code, Math.min(w - padR - 65, xx + 3), padT + 10);
        }
      }

      drawXAxisLabels(ctx, w, h, padL, padR, padB, xMin, xMax);
      drawHoverLine(ctx, vis, xOf, padT, plotH);
    }

    // --- CHART 2: Speed & Thresholds ---
    {
      const { ctx, w, h, padL, padR, padT, padB } = c2;
      const plotH = h - padT - padB;
      let yMax = 200;
      for (const p of vis) {
        yMax = Math.max(yMax, p.speed * 1.1, p.max_speed * 1.1, p.min_speed * 1.1);
      }
      const yOf = (v) => padT + plotH - (Math.max(0, v) / yMax) * plotH;

      ctx.strokeStyle = "#1e293b";
      ctx.fillStyle = "#94a3b8";
      ctx.font = "11px sans-serif";
      ctx.textAlign = "right";
      for (let i = 0; i <= 4; i++) {
        const val = (yMax * i) / 4;
        const yy = yOf(val);
        ctx.beginPath();
        ctx.moveTo(padL, yy);
        ctx.lineTo(w - padR, yy);
        ctx.stroke();
        ctx.fillText(val.toFixed(0), padL - 6, yy + 4);
      }

      // Max speed dotted line
      ctx.strokeStyle = "#64748b";
      ctx.setLineDash([4, 4]);
      ctx.lineWidth = 1.2;
      ctx.beginPath();
      for (let i = 0; i < vis.length; i++) {
        const xx = xOf(vis[i]),
          yy = yOf(vis[i].max_speed);
        if (i === 0) ctx.moveTo(xx, yy);
        else ctx.lineTo(xx, yy);
      }
      ctx.stroke();

      // Min speed / arm gate line
      ctx.strokeStyle = "#c084fc";
      ctx.setLineDash([3, 3]);
      ctx.beginPath();
      for (let i = 0; i < vis.length; i++) {
        const xx = xOf(vis[i]),
          yy = yOf(vis[i].min_speed);
        if (i === 0) ctx.moveTo(xx, yy);
        else ctx.lineTo(xx, yy);
      }
      ctx.stroke();
      ctx.setLineDash([]);

      // Speed curve
      ctx.strokeStyle = "#10b981";
      ctx.lineWidth = 2;
      ctx.beginPath();
      for (let i = 0; i < vis.length; i++) {
        const xx = xOf(vis[i]),
          yy = yOf(vis[i].speed);
        if (i === 0) ctx.moveTo(xx, yy);
        else ctx.lineTo(xx, yy);
      }
      ctx.stroke();

      drawXAxisLabels(ctx, w, h, padL, padR, padB, xMin, xMax);
      drawHoverLine(ctx, vis, xOf, padT, plotH);
    }

    // --- CHART 3: Position (left axis), Acceleration (right axis) & Phase Ribbon ---
    {
      const { ctx, w, h, padL, padR, padT, padB } = c3;
      const ribbonH = 8;
      const plotH = h - padT - padB - ribbonH;

      let pMin = Infinity,
        pMax = -Infinity,
        aMax = 500;
      for (const p of vis) {
        pMin = Math.min(pMin, p.position);
        pMax = Math.max(pMax, p.position);
        aMax = Math.max(aMax, Math.abs(p.accel) * 1.15);
      }
      if (pMin === pMax) {
        pMin -= 10;
        pMax += 10;
      }
      const yPos = (v) => padT + plotH - ((v - pMin) / (pMax - pMin)) * plotH;
      const yAcc = (a) => padT + plotH / 2 - (a / aMax) * (plotH / 2);

      ctx.strokeStyle = "#1e293b";
      ctx.font = "11px sans-serif";
      for (let i = 0; i <= 4; i++) {
        const val = pMin + ((pMax - pMin) * i) / 4;
        const yy = yPos(val);
        ctx.beginPath();
        ctx.moveTo(padL, yy);
        ctx.lineTo(w - padR, yy);
        ctx.stroke();
        ctx.fillStyle = "#60a5fa";
        ctx.textAlign = "right";
        ctx.fillText(val.toFixed(0), padL - 6, yy + 4);
      }

      // Right axis labels for Acceleration (-aMax, 0, +aMax)
      ctx.textAlign = "left";
      ctx.fillStyle = "#34d399";
      ctx.fillText("+" + aMax.toFixed(0), w - padR + 5, padT + 9);
      ctx.fillStyle = "#94a3b8";
      ctx.fillText("0", w - padR + 5, yAcc(0) + 4);
      ctx.fillStyle = "#f87171";
      ctx.fillText("-" + aMax.toFixed(0), w - padR + 5, padT + plotH);

      // Acceleration shaded step bars
      const zeroY = yAcc(0);
      for (let i = 0; i < vis.length; i++) {
        const p = vis[i];
        if (p.accel === 0) continue;
        const x0 = xOf(p);
        const x1 = i + 1 < vis.length ? Math.max(x0 + 2, xOf(vis[i + 1])) : x0 + 3;
        const yy = yAcc(p.accel);
        ctx.fillStyle = p.accel > 0 ? "rgba(52,211,153,0.25)" : "rgba(248,113,113,0.25)";
        ctx.fillRect(x0, Math.min(zeroY, yy), x1 - x0, Math.abs(yy - zeroY));
      }

      // Position curve
      ctx.strokeStyle = "#60a5fa";
      ctx.lineWidth = 2;
      ctx.beginPath();
      for (let i = 0; i < vis.length; i++) {
        const xx = xOf(vis[i]),
          yy = yPos(vis[i].position);
        if (i === 0) ctx.moveTo(xx, yy);
        else ctx.lineTo(xx, yy);
      }
      ctx.stroke();

      // Phase color ribbon at bottom of Chart 3
      const ribY = padT + plotH + 2;
      for (let i = 0; i < vis.length; i++) {
        const p = vis[i];
        const x0 = xOf(p);
        const x1 = i + 1 < vis.length ? Math.max(x0 + 2, xOf(vis[i + 1])) : x0 + 3;
        ctx.fillStyle = PHASE_COLORS[p.phase] || "#64748b";
        ctx.fillRect(x0, ribY, x1 - x0, ribbonH - 2);
      }

      drawXAxisLabels(ctx, w, h, padL, padR, padB, xMin, xMax);
      drawHoverLine(ctx, vis, xOf, padT, plotH);
    }
  }

  function drawXAxisLabels(ctx, w, h, padL, padR, padB, xMin, xMax) {
    ctx.fillStyle = "#94a3b8";
    ctx.font = "11px sans-serif";
    ctx.textAlign = "left";
    const fmt = (v) => (xAxisMode === "time" ? (v / 1000).toFixed(1) + "s" : v.toFixed(0) + " steps");
    ctx.fillText(fmt(xMin), padL, h - 6);
    ctx.textAlign = "center";
    ctx.fillText(fmt((xMin + xMax) / 2), (padL + w - padR) / 2, h - 6);
    ctx.textAlign = "right";
    ctx.fillText(fmt(xMax), w - padR, h - 6);
  }

  function drawHoverLine(ctx, vis, xOf, padT, plotH) {
    if (hoverIndex < 0 || hoverIndex >= vis.length) return;
    const xx = xOf(vis[hoverIndex]);
    ctx.strokeStyle = "#f8fafc";
    ctx.lineWidth = 1;
    ctx.setLineDash([3, 3]);
    ctx.beginPath();
    ctx.moveTo(xx, padT);
    ctx.lineTo(xx, padT + plotH);
    ctx.stroke();
    ctx.setLineDash([]);
  }

  function renderEventsTable() {
    if (!events.length) return;
    evtBodyEl.innerHTML = events
      .slice(0, 35)
      .map((ev) => {
        const isStall =
          ev.code.includes("SG") ||
          ev.code.includes("DIAG") ||
          ev.code.includes("OBSTACLE") ||
          ev.code.includes("RESET") ||
          ev.code.includes("ERROR");
        const color = isStall ? "#fca5a5" : "#e2e8f0";
        return `<tr style="color:${color}">
          <td>${(ev.ms / 1000).toFixed(2)}s</td>
          <td><strong>${ev.code}</strong></td>
          <td>${PHASE_NAMES[ev.phase] || ev.phase}</td>
          <td>${ev.position} / ${ev.target_pos}</td>
          <td>${ev.speed.toFixed(0)} / ${ev.max_speed.toFixed(0)}</td>
          <td>${ev.accel >= 0 ? "+" : ""}${ev.accel.toFixed(0)}</td>
          <td>${ev.result} / &le;${ev.trigger_level}</td>
          <td>${ev.consec_stalls}/3</td>
          <td>${ev.steps_since_arm}</td>
          <td>${ev.diag_pin}</td>
          <td>${ev.reason}</td>
        </tr>`;
      })
      .join("");
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", createDashboardDOM);
  } else {
    createDashboardDOM();
  }
})();
