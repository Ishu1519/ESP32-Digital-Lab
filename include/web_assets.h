#pragma once

// Self-contained embedded Web Dashboard for ESP32 Digital Lab
// Designed with ultra-sleek dark glassmorphism, responsive CSS grid, and high-performance Canvas charts.
// Size: ~18 KB (uncompressed) - well under the 200 KB target!

static const char INDEX_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP32 Digital Lab</title>
<style>
:root {
  --bg-primary: #0a0e17;
  --bg-card: rgba(18, 26, 43, 0.75);
  --bg-card-border: rgba(64, 120, 255, 0.15);
  --accent-cyan: #00f2fe;
  --accent-blue: #4facfe;
  --accent-emerald: #10b981;
  --accent-amber: #f59e0b;
  --accent-rose: #f43f5e;
  --text-primary: #f8fafc;
  --text-secondary: #94a3b8;
  --text-muted: #64748b;
  --font-mono: 'JetBrains Mono', 'Consolas', 'Courier New', monospace;
  --font-sans: 'Inter', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
}

* { box-sizing: border-box; margin: 0; padding: 0; }
body {
  background: var(--bg-primary);
  background-image: radial-gradient(circle at 15% 15%, rgba(79, 172, 254, 0.08) 0%, transparent 40%),
                    radial-gradient(circle at 85% 85%, rgba(0, 242, 254, 0.05) 0%, transparent 40%);
  color: var(--text-primary);
  font-family: var(--font-sans);
  min-height: 100vh;
  padding: 16px;
  line-height: 1.5;
}

/* Header */
header {
  display: flex;
  flex-wrap: wrap;
  justify-content: space-between;
  align-items: center;
  gap: 16px;
  padding: 16px 24px;
  background: var(--bg-card);
  backdrop-filter: blur(12px);
  border: 1px solid var(--bg-card-border);
  border-radius: 16px;
  margin-bottom: 20px;
  box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
}

.logo-group {
  display: flex;
  align-items: center;
  gap: 14px;
}

.logo-icon {
  width: 42px;
  height: 42px;
  border-radius: 10px;
  background: linear-gradient(135deg, var(--accent-blue), var(--accent-cyan));
  display: flex;
  align-items: center;
  justify-content: center;
  font-weight: 900;
  font-size: 20px;
  color: #050b14;
  box-shadow: 0 0 20px rgba(0, 242, 254, 0.35);
}

.title-box h1 {
  font-size: 20px;
  font-weight: 700;
  letter-spacing: -0.5px;
  background: linear-gradient(90deg, #fff, #94a3b8);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
}

.title-box .subtitle {
  font-size: 11px;
  color: var(--text-muted);
  font-family: var(--font-mono);
}

.status-badges {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 10px;
}

.badge {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  padding: 5px 12px;
  border-radius: 20px;
  font-size: 12px;
  font-family: var(--font-mono);
  background: rgba(255, 255, 255, 0.04);
  border: 1px solid rgba(255, 255, 255, 0.08);
}

.badge-dot {
  width: 8px;
  height: 8px;
  border-radius: 50%;
  background: var(--text-muted);
}

.badge-dot.live {
  background: var(--accent-emerald);
  box-shadow: 0 0 10px var(--accent-emerald);
  animation: pulse 2s infinite;
}

@keyframes pulse {
  0% { opacity: 1; transform: scale(1); }
  50% { opacity: 0.5; transform: scale(0.9); }
  100% { opacity: 1; transform: scale(1); }
}

/* Mode Navigation Tabs */
.nav-tabs {
  display: flex;
  gap: 8px;
  overflow-x: auto;
  padding-bottom: 8px;
  margin-bottom: 20px;
}

.tab-btn {
  background: var(--bg-card);
  border: 1px solid var(--bg-card-border);
  color: var(--text-secondary);
  padding: 10px 18px;
  border-radius: 12px;
  cursor: pointer;
  font-size: 13px;
  font-weight: 600;
  transition: all 0.2s ease;
  white-space: nowrap;
  display: flex;
  align-items: center;
  gap: 8px;
}

.tab-btn:hover {
  color: var(--text-primary);
  border-color: rgba(79, 172, 254, 0.4);
}

.tab-btn.active {
  background: linear-gradient(135deg, rgba(79, 172, 254, 0.2), rgba(0, 242, 254, 0.1));
  color: var(--accent-cyan);
  border-color: var(--accent-cyan);
  box-shadow: 0 0 16px rgba(0, 242, 254, 0.15);
}

.tab-btn.disabled {
  opacity: 0.45;
  cursor: not-allowed;
}

.tag-badge {
  font-size: 9px;
  padding: 2px 6px;
  border-radius: 6px;
  background: rgba(255, 255, 255, 0.1);
  text-transform: uppercase;
}

/* Grid Layout */
.dashboard-grid {
  display: grid;
  grid-template-columns: 1fr 340px;
  gap: 20px;
}

@media (max-width: 960px) {
  .dashboard-grid {
    grid-template-columns: 1fr;
  }
}

/* Card */
.card {
  background: var(--bg-card);
  backdrop-filter: blur(12px);
  border: 1px solid var(--bg-card-border);
  border-radius: 16px;
  padding: 20px;
  box-shadow: 0 8px 32px rgba(0, 0, 0, 0.2);
  margin-bottom: 20px;
}

.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 16px;
}

.card-title {
  font-size: 14px;
  font-weight: 600;
  text-transform: uppercase;
  letter-spacing: 0.8px;
  color: var(--text-secondary);
  display: flex;
  align-items: center;
  gap: 8px;
}

/* Giant Main Frequency Readout */
.hero-display {
  background: rgba(0, 0, 0, 0.4);
  border: 1px solid rgba(0, 242, 254, 0.25);
  border-radius: 14px;
  padding: 28px 24px;
  text-align: center;
  margin-bottom: 20px;
  position: relative;
  overflow: hidden;
  box-shadow: inset 0 0 25px rgba(0, 242, 254, 0.05);
}

.hero-display::before {
  content: '';
  position: absolute;
  top: 0; left: 0; right: 0; height: 2px;
  background: linear-gradient(90deg, transparent, var(--accent-cyan), transparent);
}

.freq-main {
  font-family: var(--font-mono);
  font-size: 48px;
  font-weight: 800;
  color: var(--text-primary);
  letter-spacing: -1px;
  text-shadow: 0 0 20px rgba(0, 242, 254, 0.3);
}

.freq-unit {
  font-size: 24px;
  color: var(--accent-cyan);
  margin-left: 8px;
  font-weight: 600;
}

.freq-sub {
  font-family: var(--font-mono);
  font-size: 13px;
  color: var(--text-muted);
  margin-top: 6px;
}

/* Secondary Metrics Metric Row */
.metrics-row {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(130px, 1fr));
  gap: 12px;
  margin-bottom: 20px;
}

.metric-pill {
  background: rgba(255, 255, 255, 0.025);
  border: 1px solid rgba(255, 255, 255, 0.06);
  border-radius: 12px;
  padding: 12px;
}

.metric-pill .label {
  font-size: 11px;
  text-transform: uppercase;
  letter-spacing: 0.5px;
  color: var(--text-muted);
  margin-bottom: 4px;
}

.metric-pill .val {
  font-family: var(--font-mono);
  font-size: 16px;
  font-weight: 700;
  color: var(--text-primary);
}

/* Charts / Canvas */
.chart-container {
  width: 100%;
  height: 180px;
  position: relative;
  background: rgba(0, 0, 0, 0.25);
  border-radius: 12px;
  border: 1px solid rgba(255, 255, 255, 0.05);
  overflow: hidden;
  margin-top: 10px;
}

canvas {
  width: 100%;
  height: 100%;
  display: block;
}

/* Controls */
.control-group {
  margin-bottom: 16px;
}

.control-group label {
  display: block;
  font-size: 12px;
  color: var(--text-secondary);
  margin-bottom: 6px;
}

select, input, button {
  width: 100%;
  padding: 10px 14px;
  background: rgba(0, 0, 0, 0.35);
  border: 1px solid var(--bg-card-border);
  border-radius: 10px;
  color: var(--text-primary);
  font-family: var(--font-sans);
  font-size: 13px;
  outline: none;
  transition: all 0.2s ease;
}

select:focus, input:focus {
  border-color: var(--accent-cyan);
  box-shadow: 0 0 10px rgba(0, 242, 254, 0.2);
}

button.btn-primary {
  background: linear-gradient(135deg, var(--accent-blue), var(--accent-cyan));
  color: #050b14;
  font-weight: 700;
  cursor: pointer;
  border: none;
  box-shadow: 0 4px 15px rgba(0, 242, 254, 0.25);
}

button.btn-primary:hover {
  opacity: 0.95;
  transform: translateY(-1px);
}

button.btn-secondary {
  background: rgba(255, 255, 255, 0.06);
  border: 1px solid rgba(255, 255, 255, 0.12);
  color: var(--text-secondary);
  font-weight: 600;
  cursor: pointer;
}

button.btn-secondary:hover {
  background: rgba(255, 255, 255, 0.1);
  color: var(--text-primary);
}

/* Preset Buttons Grid */
.preset-grid {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 8px;
  margin-top: 8px;
}

.btn-preset {
  padding: 7px 4px;
  font-size: 11px;
  font-family: var(--font-mono);
  background: rgba(255, 255, 255, 0.04);
  border: 1px solid rgba(255, 255, 255, 0.08);
  border-radius: 8px;
  color: var(--text-secondary);
  cursor: pointer;
}

.btn-preset:hover {
  background: rgba(79, 172, 254, 0.15);
  color: var(--accent-cyan);
  border-color: rgba(79, 172, 254, 0.4);
}

/* Range Slider */
input[type=range] {
  -webkit-appearance: none;
  background: rgba(255, 255, 255, 0.1);
  height: 6px;
  border-radius: 3px;
  padding: 0;
  border: none;
}

input[type=range]::-webkit-slider-thumb {
  -webkit-appearance: none;
  height: 16px;
  width: 16px;
  border-radius: 50%;
  background: var(--accent-cyan);
  cursor: pointer;
  box-shadow: 0 0 10px var(--accent-cyan);
}

/* Stats table */
.stat-row {
  display: flex;
  justify-content: space-between;
  padding: 6px 0;
  border-bottom: 1px solid rgba(255, 255, 255, 0.04);
  font-size: 12px;
  font-family: var(--font-mono);
}

.stat-row:last-child { border-bottom: none; }
.stat-label { color: var(--text-muted); }
.stat-val { color: var(--text-primary); font-weight: 600; }
</style>
</head>
<body>

<header>
  <div class="logo-group">
    <div class="logo-icon">&#x223F;</div>
    <div class="title-box">
      <h1>ESP32 Digital Lab</h1>
      <div class="subtitle">ORIGINAL ESP32-D0WD-V3 &bull; ALL-IN-ONE INSTRUMENT</div>
    </div>
  </div>
  
  <div class="status-badges">
    <div class="badge"><div class="badge-dot" id="ws-dot"></div> <span id="ws-status">Connecting...</span></div>
    <div class="badge">HEAP: <span id="heap-val" style="color:var(--accent-cyan)">-- KB</span></div>
    <div class="badge">UPTIME: <span id="uptime-val">-- s</span></div>
  </div>
</header>

<div class="nav-tabs">
  <button class="tab-btn active" id="tab-freq"><span style="color:var(--accent-cyan)">&#x2382;</span> Frequency Counter</button>
  <button class="tab-btn disabled" title="Milestone 2"><span style="color:var(--text-muted)">&#x2637;</span> Logic Analyzer <span class="tag-badge">Phase 3</span></button>
  <button class="tab-btn disabled" title="Milestone 3"><span style="color:var(--text-muted)">&#x223F;</span> Oscilloscope <span class="tag-badge">Phase 7</span></button>
  <button class="tab-btn disabled" title="Milestone 4"><span style="color:var(--text-muted)">&#x2300;</span> DMM <span class="tag-badge">Phase 8</span></button>
  <button class="tab-btn disabled" title="Milestone 5"><span style="color:var(--text-muted)">&#x223C;</span> Spectrum (FFT) <span class="tag-badge">Phase 11</span></button>
</div>

<div class="dashboard-grid">
  <!-- Left Main Column: Live Measurement & Waveform Display -->
  <div class="main-column">
    <div class="card">
      <div class="card-header">
        <div class="card-title">Live Pulse Counter &bull; GPIO 18 (PCNT)</div>
        <div class="badge" style="background:rgba(16,185,129,0.1); border-color:rgba(16,185,129,0.3); color:var(--accent-emerald)" id="sig-badge">SIGNAL DETECTED</div>
      </div>

      <div class="hero-display">
        <div class="freq-main"><span id="main-freq-val">0.000</span><span class="freq-unit" id="main-freq-unit">Hz</span></div>
        <div class="freq-sub">Raw Exact: <span id="exact-freq-val" style="color:var(--accent-cyan); font-weight:700">0</span> Hz &bull; Gate Window: <span id="gate-disp">500 ms</span></div>
      </div>

      <div class="metrics-row">
        <div class="metric-pill">
          <div class="label">Period</div>
          <div class="val" id="val-period">--</div>
        </div>
        <div class="metric-pill">
          <div class="label">Duty Cycle</div>
          <div class="val" id="val-duty">-- %</div>
        </div>
        <div class="metric-pill">
          <div class="label">Pulse Width</div>
          <div class="val" id="val-pulse">--</div>
        </div>
        <div class="metric-pill">
          <div class="label">Pulses In Gate</div>
          <div class="val" id="val-pulses">--</div>
        </div>
      </div>

      <div class="card-header" style="margin-top:20px; margin-bottom:6px">
        <div class="card-title" style="font-size:12px">Reconstructed Pulse Waveform</div>
      </div>
      <div class="chart-container" style="height:110px">
        <canvas id="canvas-wave"></canvas>
      </div>

      <div class="card-header" style="margin-top:20px; margin-bottom:6px">
        <div class="card-title" style="font-size:12px">Frequency Trend History</div>
      </div>
      <div class="chart-container" style="height:150px">
        <canvas id="canvas-trend"></canvas>
      </div>
    </div>
  </div>

  <!-- Right Sidebar: Hardware Controls, Loopback Reference Gen, Stats -->
  <div class="sidebar-column">
    <!-- Test Reference Signal Generator -->
    <div class="card">
      <div class="card-header">
        <div class="card-title" style="color:var(--accent-cyan)">Ref Signal Generator</div>
        <div class="badge" style="font-size:10px">GPIO 19</div>
      </div>
      <p style="font-size:11px; color:var(--text-muted); margin-bottom:12px;">
        Connect <strong>GPIO 19</strong> (LEDC out) to <strong>GPIO 18</strong> (PCNT in) with a jumper for loopback verification.
      </p>

      <div class="control-group">
        <label>Quick Presets</label>
        <div class="preset-grid">
          <button class="btn-preset" onclick="setRefGen(1000, 50)">1 kHz</button>
          <button class="btn-preset" onclick="setRefGen(10000, 50)">10 kHz</button>
          <button class="btn-preset" onclick="setRefGen(100000, 50)">100 kHz</button>
          <button class="btn-preset" onclick="setRefGen(1000000, 50)">1 MHz</button>
          <button class="btn-preset" onclick="setRefGen(5000000, 50)">5 MHz</button>
          <button class="btn-preset" onclick="setRefGen(10000000, 50)">10 MHz</button>
        </div>
      </div>

      <div class="control-group">
        <label>Custom Frequency (Hz)</label>
        <input type="number" id="gen-freq-input" value="10000" min="1" max="40000000">
      </div>

      <div class="control-group">
        <div style="display:flex; justify-content:space-between; margin-bottom:4px">
          <label>Duty Cycle</label>
          <span id="gen-duty-disp" style="font-size:11px; font-family:var(--font-mono); color:var(--accent-cyan)">50%</span>
        </div>
        <input type="range" id="gen-duty-slider" min="1" max="99" value="50" oninput="document.getElementById('gen-duty-disp').innerText = this.value + '%'">
      </div>

      <div style="display:flex; gap:8px">
        <button class="btn-primary" onclick="applyRefGen()">Apply Generator</button>
      </div>
    </div>

    <!-- Gate Timing & Statistics -->
    <div class="card">
      <div class="card-header">
        <div class="card-title">Measurement Statistics</div>
        <button class="btn-secondary" style="width:auto; padding:4px 10px; font-size:11px" onclick="resetStats()">Reset</button>
      </div>

      <div class="control-group">
        <label>Gate Sampling Interval</label>
        <select id="gate-select" onchange="setGateTime(this.value)">
          <option value="100">100 ms (Fast 10 Hz)</option>
          <option value="200">200 ms (5 Hz)</option>
          <option value="500" selected>500 ms (Balanced 2 Hz)</option>
          <option value="1000">1000 ms (1 Hz - Highest Precision)</option>
        </select>
      </div>

      <div style="margin-top:12px">
        <div class="stat-row"><span class="stat-label">Samples:</span><span class="stat-val" id="stat-samples">0</span></div>
        <div class="stat-row"><span class="stat-label">Min:</span><span class="stat-val" id="stat-min">--</span></div>
        <div class="stat-row"><span class="stat-label">Max:</span><span class="stat-val" id="stat-max">--</span></div>
        <div class="stat-row"><span class="stat-label">Mean:</span><span class="stat-val" id="stat-mean">--</span></div>
        <div class="stat-row"><span class="stat-label">Std Dev (&sigma;):</span><span class="stat-val" id="stat-std">--</span></div>
      </div>
    </div>
  </div>
</div>

<script>
let ws = null;
let trendHistory = [];
const MAX_HISTORY = 60;

function formatFreq(hz) {
  if (hz >= 1e9) return { val: (hz / 1e9).toFixed(6), unit: 'GHz' };
  if (hz >= 1e6) return { val: (hz / 1e6).toFixed(6), unit: 'MHz' };
  if (hz >= 1e3) return { val: (hz / 1e3).toFixed(4), unit: 'kHz' };
  return { val: hz.toFixed(2), unit: 'Hz' };
}

function formatPeriod(us) {
  if (us <= 0) return '--';
  if (us < 1.0) return (us * 1000).toFixed(1) + ' ns';
  if (us < 1000) return us.toFixed(2) + ' \u03BCs';
  if (us < 1000000) return (us / 1000).toFixed(3) + ' ms';
  return (us / 1000000).toFixed(3) + ' s';
}

function connectWebSocket() {
  const wsUrl = `ws://${window.location.hostname || '192.168.4.1'}/ws`;
  const dot = document.getElementById('ws-dot');
  const status = document.getElementById('ws-status');

  dot.className = 'badge-dot';
  status.innerText = 'Connecting...';

  ws = new WebSocket(wsUrl);

  ws.onopen = () => {
    dot.className = 'badge-dot live';
    status.innerText = 'ONLINE (Live WS)';
  };

  ws.onclose = () => {
    dot.className = 'badge-dot';
    status.innerText = 'DISCONNECTED';
    setTimeout(connectWebSocket, 2000);
  };

  ws.onerror = () => {
    ws.close();
  };

  ws.onmessage = (evt) => {
    try {
      const msg = JSON.parse(evt.data);
      if (msg.type === 'telemetry') {
        handleTelemetry(msg);
      }
    } catch(e) {
      console.error(e);
    }
  };
}

function handleTelemetry(pkt) {
  if (pkt.system) {
    document.getElementById('heap-val').innerText = (pkt.system.free_heap / 1024).toFixed(1) + ' KB';
    document.getElementById('uptime-val').innerText = pkt.system.uptime_s + ' s';
  }

  if (pkt.instrument && pkt.instrument.data) {
    const d = pkt.instrument.data;
    const f = d.freq_hz || 0;
    const fmt = formatFreq(f);

    document.getElementById('main-freq-val').innerText = fmt.val;
    document.getElementById('main-freq-unit').innerText = fmt.unit;
    document.getElementById('exact-freq-val').innerText = f.toLocaleString('en-US', {maximumFractionDigits: 2});
    document.getElementById('gate-disp').innerText = d.gate_time_ms + ' ms';

    document.getElementById('val-period').innerText = formatPeriod(d.period_us);
    document.getElementById('val-duty').innerText = (d.duty_pct || 0).toFixed(1) + ' %';
    document.getElementById('val-pulse').innerText = formatPeriod(d.pulse_width_us);
    document.getElementById('val-pulses').innerText = (d.total_pulses || 0).toLocaleString();

    const sigBadge = document.getElementById('sig-badge');
    if (d.signal_detected) {
      sigBadge.innerText = 'SIGNAL LOCKED';
      sigBadge.style.color = 'var(--accent-emerald)';
      sigBadge.style.background = 'rgba(16,185,129,0.1)';
      sigBadge.style.borderColor = 'rgba(16,185,129,0.3)';
    } else {
      sigBadge.innerText = 'NO SIGNAL';
      sigBadge.style.color = 'var(--accent-rose)';
      sigBadge.style.background = 'rgba(244,63,94,0.1)';
      sigBadge.style.borderColor = 'rgba(244,63,94,0.3)';
    }

    if (d.stats) {
      document.getElementById('stat-samples').innerText = d.stats.samples || 0;
      document.getElementById('stat-min').innerText = d.stats.min_hz ? formatFreq(d.stats.min_hz).val + ' ' + formatFreq(d.stats.min_hz).unit : '--';
      document.getElementById('stat-max').innerText = d.stats.max_hz ? formatFreq(d.stats.max_hz).val + ' ' + formatFreq(d.stats.max_hz).unit : '--';
      document.getElementById('stat-mean').innerText = d.stats.mean_hz ? formatFreq(d.stats.mean_hz).val + ' ' + formatFreq(d.stats.mean_hz).unit : '--';
      document.getElementById('stat-std').innerText = d.stats.std_dev_hz ? d.stats.std_dev_hz.toFixed(2) + ' Hz' : '--';
    }

    // Append to trend history
    trendHistory.push(f);
    if (trendHistory.length > MAX_HISTORY) trendHistory.shift();

    drawWaveform(d.duty_pct || 50, d.signal_detected);
    drawTrend();
  }
}

function drawWaveform(dutyPct, signalDetected) {
  const canvas = document.getElementById('canvas-wave');
  const ctx = canvas.getContext('2d');
  const w = canvas.width = canvas.parentElement.clientWidth;
  const h = canvas.height = canvas.parentElement.clientHeight;

  ctx.clearRect(0, 0, w, h);

  // Background Grid
  ctx.strokeStyle = 'rgba(255, 255, 255, 0.05)';
  ctx.lineWidth = 1;
  for (let x = 0; x < w; x += 30) {
    ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, h); ctx.stroke();
  }
  for (let y = 0; y < h; y += 25) {
    ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(w, y); ctx.stroke();
  }

  if (!signalDetected) {
    ctx.strokeStyle = 'rgba(244, 63, 94, 0.5)';
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.moveTo(0, h * 0.75);
    ctx.lineTo(w, h * 0.75);
    ctx.stroke();
    return;
  }

  // Draw 4 reconstructed cycles
  const cycles = 4;
  const cycleWidth = w / cycles;
  const dutyFraction = Math.max(0.02, Math.min(0.98, dutyPct / 100.0));
  const highY = h * 0.25;
  const lowY = h * 0.75;

  ctx.strokeStyle = '#00f2fe';
  ctx.lineWidth = 2.5;
  ctx.shadowColor = '#00f2fe';
  ctx.shadowBlur = 8;
  ctx.beginPath();

  for (let i = 0; i < cycles; i++) {
    const startX = i * cycleWidth;
    const splitX = startX + (cycleWidth * dutyFraction);
    const endX = (i + 1) * cycleWidth;

    if (i === 0) ctx.moveTo(startX, lowY);
    ctx.lineTo(startX, highY);
    ctx.lineTo(splitX, highY);
    ctx.lineTo(splitX, lowY);
    ctx.lineTo(endX, lowY);
  }
  ctx.stroke();
  ctx.shadowBlur = 0;
}

function drawTrend() {
  const canvas = document.getElementById('canvas-trend');
  const ctx = canvas.getContext('2d');
  const w = canvas.width = canvas.parentElement.clientWidth;
  const h = canvas.height = canvas.parentElement.clientHeight;

  ctx.clearRect(0, 0, w, h);

  if (trendHistory.length < 2) return;

  const min = Math.min(...trendHistory);
  const max = Math.max(...trendHistory);
  const range = (max - min) > 0 ? (max - min) : 1;

  ctx.strokeStyle = '#4facfe';
  ctx.lineWidth = 2;
  ctx.beginPath();

  const step = w / (MAX_HISTORY - 1);
  const startOffset = (MAX_HISTORY - trendHistory.length) * step;

  trendHistory.forEach((val, idx) => {
    const x = startOffset + (idx * step);
    const norm = (val - min) / range;
    const y = h - 20 - (norm * (h - 40));
    if (idx === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  });
  ctx.stroke();
}

function sendCommand(cmdObj) {
  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify(cmdObj));
  }
}

function setGateTime(gateMs) {
  sendCommand({ action: 'set_gate_time', gate_time_ms: parseInt(gateMs) });
}

function resetStats() {
  sendCommand({ action: 'reset_stats' });
  trendHistory = [];
}

function setRefGen(freqHz, dutyPct) {
  document.getElementById('gen-freq-input').value = freqHz;
  document.getElementById('gen-duty-slider').value = dutyPct;
  document.getElementById('gen-duty-disp').innerText = dutyPct + '%';
  applyRefGen();
}

function applyRefGen() {
  const freq = parseInt(document.getElementById('gen-freq-input').value);
  const duty = parseInt(document.getElementById('gen-duty-slider').value);
  sendCommand({ action: 'set_ref_gen', freq_hz: freq, duty_pct: duty, enabled: true });
}

window.addEventListener('load', () => {
  connectWebSocket();
});
</script>
</body>
</html>
)rawliteral";
