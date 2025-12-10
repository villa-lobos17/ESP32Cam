
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "esp_camera.h"
#include <esp_system.h>

extern "C" uint8_t temprature_sens_read();

// Replace with your WiFi credentials
const char *WIFI_SSID = "xxxxxxxxx";
const char *WIFI_PASS = "xxxxxxxxxx";
const char *HOSTNAME = "esp32-cam";

// AI Thinker ESP32-CAM pinout
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

WebServer server(80);

void printDiagnostics();
bool connectToWiFi(const char *ssid, const char *pass, uint8_t maxRetries = 25);
bool initCamera();
void handleRoot();
void handleStream();
void handleControl();
void handleStats();

float readChipTempC() {
  // Lectura básica del sensor interno; no es muy precisa pero sirve de referencia.
  return ((float)temprature_sens_read() - 32.0f) / 1.8f;
}

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1.0" />
  <title>ESP32 Cam Dashboard</title>
  <style>
    @import url('https://fonts.googleapis.com/css2?family=Space+Grotesk:wght@400;600&family=Share+Tech+Mono&display=swap');
    :root {
      --bg: #030712;
      --panel: rgba(7, 14, 30, 0.92);
      --accent: #2dd4ff;
      --accent-2: #7c3aed;
      --accent-3: #22c55e;
      --text: #e2e8f0;
      --muted: #94a3b8;
      --border: rgba(45, 212, 255, 0.25);
      --glow: 0 24px 80px rgba(45, 212, 255, 0.35);
    }
    * { box-sizing: border-box; }
    body {
      margin: 0;
      font-family: 'Space Grotesk', system-ui, sans-serif;
      background: radial-gradient(circle at 30% 20%, rgba(45,212,255,0.15), transparent 35%),
                  radial-gradient(circle at 70% 10%, rgba(124,58,237,0.18), transparent 30%),
                  radial-gradient(circle at 50% 80%, rgba(34,197,94,0.12), transparent 40%),
                  linear-gradient(135deg, #040711 0%, #050c1a 50%, #0b1628 100%);
      color: var(--text);
      min-height: 100vh;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 18px;
    }
    .shell {
      width: min(1200px, 100%);
      background: linear-gradient(150deg, rgba(255,255,255,0.05), rgba(255,255,255,0.01));
      border: 1px solid var(--border);
      border-radius: 22px;
      padding: 22px;
      box-shadow: var(--glow);
      backdrop-filter: blur(18px);
      position: relative;
      overflow: hidden;
    }
    .shell:before {
      content: "";
      position: absolute;
      inset: 0;
      background: linear-gradient(90deg, rgba(45,212,255,0.08), rgba(124,58,237,0.05));
      opacity: 0.6;
      pointer-events: none;
    }
    .hud-rings {
      position: absolute;
      inset: 8% 6%;
      background:
        radial-gradient(circle at 50% 50%, rgba(45,212,255,0.2), transparent 32%),
        radial-gradient(circle at 50% 50%, rgba(124,58,237,0.15), transparent 42%),
        repeating-radial-gradient(circle at 50% 50%, rgba(45,212,255,0.08), rgba(45,212,255,0.08) 2px, transparent 3px, transparent 8px);
      mix-blend-mode: screen;
      filter: blur(10px);
      pointer-events: none;
    }
    header {
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 16px;
      flex-wrap: wrap;
      position: relative;
      z-index: 1;
    }
    .title {
      font-size: 18px;
      letter-spacing: 0.16em;
      display: flex;
      align-items: center;
      gap: 12px;
      font-family: 'Share Tech Mono', monospace;
      text-transform: uppercase;
    }
    .status {
      display: flex;
      align-items: center;
      gap: 8px;
      padding: 10px 14px;
      border-radius: 12px;
      background: rgba(45,212,255,0.08);
      border: 1px solid var(--border);
      box-shadow: 0 0 12px rgba(45,212,255,0.45);
    }
    .status .dot {
      width: 12px;
      height: 12px;
      border-radius: 50%;
      background: var(--accent);
      box-shadow: 0 0 12px rgba(45,212,255,0.7);
    }
    .grid {
      display: grid;
      grid-template-columns: 2fr 1.1fr;
      gap: 14px;
      margin-top: 12px;
    }
    .card {
      background: var(--panel);
      border: 1px solid var(--border);
      border-radius: 18px;
      padding: 14px;
      overflow: hidden;
      box-shadow: 0 12px 30px rgba(0,0,0,0.3);
      position: relative;
    }
    .card h3 {
      margin: 0 0 10px;
      font-size: 13px;
      letter-spacing: 0.14em;
      text-transform: uppercase;
      color: var(--muted);
      display: flex;
      align-items: center;
      gap: 8px;
    }
    .segment {
      border-top: 1px solid rgba(255,255,255,0.06);
      margin-top: 10px;
      padding-top: 10px;
    }
    #streamBox { position: relative; }
    #stream, #canvasFx {
      width: 100%;
      border-radius: 14px;
      border: 1px solid var(--border);
      background: radial-gradient(circle at 30% 30%, rgba(45,212,255,0.1), #030712);
      min-height: 340px;
      object-fit: cover;
      display: block;
    }
    #canvasFx { position: absolute; inset: 0; opacity: 0; transition: opacity 0.2s ease; pointer-events: none; }
    .actions {
      display: flex;
      gap: 10px;
      flex-wrap: wrap;
      margin-top: 12px;
    }
    button, .toggle {
      cursor: pointer;
      border: 1px solid var(--border);
      background: linear-gradient(135deg, rgba(45,212,255,0.22), rgba(124,58,237,0.14));
      color: var(--text);
      padding: 10px 14px;
      border-radius: 12px;
      font-size: 13px;
      letter-spacing: 0.04em;
      transition: transform 0.15s ease, border-color 0.15s ease, box-shadow 0.15s;
    }
    button:hover, .toggle:hover { transform: translateY(-1px); border-color: rgba(45,212,255,0.5); box-shadow: 0 0 12px rgba(45,212,255,0.35); }
    button:active, .toggle:active { transform: translateY(0); }
    .stats {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(170px, 1fr));
      gap: 10px;
      margin-top: 4px;
    }
    .stat {
      padding: 12px;
      border-radius: 12px;
      border: 1px solid var(--border);
      background: rgba(255,255,255,0.03);
      box-shadow: inset 0 1px 0 rgba(255,255,255,0.04);
    }
    .stat .label { color: var(--muted); font-size: 11px; letter-spacing: 0.16em; text-transform: uppercase; }
    .stat .value { font-size: 18px; margin-top: 6px; font-weight: 600; }
    .meter {
      margin-top: 8px;
      width: 100%;
      height: 10px;
      border-radius: 999px;
      background: rgba(255,255,255,0.06);
      overflow: hidden;
      border: 1px solid var(--border);
    }
    .meter .fill {
      height: 100%;
      background: linear-gradient(90deg, var(--accent), var(--accent-2), var(--accent-3));
      width: 30%;
      box-shadow: 0 0 10px rgba(45,212,255,0.5);
      transition: width 0.25s ease;
    }
    .controls { display: grid; gap: 10px; grid-template-columns: repeat(auto-fit, minmax(220px, 1fr)); }
    .control {
      background: rgba(255,255,255,0.03);
      border: 1px solid var(--border);
      border-radius: 12px;
      padding: 10px 12px;
      box-shadow: inset 0 1px 0 rgba(255,255,255,0.05);
    }
    .control label { display: flex; justify-content: space-between; color: var(--muted); font-size: 11px; letter-spacing: 0.12em; text-transform: uppercase; }
    .control .value { color: var(--text); font-weight: 600; }
    input[type=range], select {
      width: 100%;
      accent-color: var(--accent);
      background: transparent;
      margin-top: 6px;
    }
    select {
      border-radius: 10px;
      border: 1px solid var(--border);
      padding: 8px 10px;
      color: var(--text);
      background: rgba(255,255,255,0.04);
    }
    .toggles { display: grid; grid-template-columns: repeat(auto-fit, minmax(160px,1fr)); gap: 8px; margin-top: 6px; }
    .toggle {
      display: flex;
      align-items: center;
      justify-content: space-between;
      background: rgba(255,255,255,0.04);
    }
    .toggle span { color: var(--muted); letter-spacing: 0.08em; font-size: 12px; text-transform: uppercase; }
    .pill { padding: 6px 10px; border-radius: 999px; background: rgba(45,212,255,0.12); font-size: 12px; }
    footer { margin-top: 14px; color: var(--muted); font-size: 12px; letter-spacing: 0.05em; }
    @media (max-width: 900px) { .grid { grid-template-columns: 1fr; } }
  </style>
</head>
<body>
  <div class="shell">
    <div class="hud-rings"></div>
    <header>
      <div class="title">OSLO CAM OPS</div>
      <div class="status">
        <div class="dot"></div>
        <div id="ipLabel">Link Active</div>
      </div>
    </header>
    <div class="grid">
      <div class="card">
        <h3>VISOR PRINCIPAL</h3>
        <div id="streamBox">
          <img id="stream" src="" alt="Stream" />
          <canvas id="canvasFx"></canvas>
        </div>
        <div class="actions">
          <button id="startBtn">Start</button>
          <button id="stopBtn">Stop</button>
          <button id="aiBtn">AI Enhance</button>
          <button id="refreshBtn">Reiniciar cam</button>
        </div>
      </div>
      <div class="card">
        <h3>TELEMETRÍA</h3>
        <div class="stats">
          <div class="stat">
            <div class="label">IP</div>
            <div class="value" id="ip">{IP}</div>
          </div>
          <div class="stat">
            <div class="label">Chip</div>
            <div class="value">{CHIP}</div>
          </div>
          <div class="stat">
            <div class="label">CPU</div>
            <div class="value">{CPU} MHz</div>
          </div>
          <div class="stat">
            <div class="label">Temp chip</div>
            <div class="value" id="tempVal">{TEMP} °C</div>
          </div>
          <div class="stat">
            <div class="label">Heap libre</div>
            <div class="value">{HEAP} KB</div>
            <div class="meter"><div class="fill" id="heapBar"></div></div>
          </div>
          <div class="stat">
            <div class="label">Heap mínimo</div>
            <div class="value">{HEAP_MIN} KB</div>
          </div>
          <div class="stat">
            <div class="label">PSRAM libre</div>
            <div class="value" id="psramVal">{PSRAM} KB</div>
            <div class="meter"><div class="fill" id="psramBar"></div></div>
          </div>
        </div>
      </div>
    </div>
    <div class="card" style="margin-top:12px;">
      <h3>AJUSTES DE IMAGEN</h3>
      <div class="controls">
        <div class="control">
          <label>Resolución <span class="value" id="resLabel"></span></label>
          <select id="resolution">
            <option value="UXGA">UXGA 1600x1200</option>
            <option value="SXGA">SXGA 1280x1024</option>
            <option value="SVGA">SVGA 800x600</option>
            <option value="VGA">VGA 640x480</option>
          </select>
        </div>
        <div class="control">
          <label>Calidad JPEG <span class="value" id="qLabel"></span></label>
          <input id="quality" type="range" min="5" max="30" step="1" />
        </div>
        <div class="control">
          <label>Brillo <span class="value" id="bLabel"></span></label>
          <input id="brightness" type="range" min="-2" max="2" step="1" />
        </div>
        <div class="control">
          <label>Contraste <span class="value" id="cLabel"></span></label>
          <input id="contrast" type="range" min="-2" max="2" step="1" />
        </div>
        <div class="control">
          <label>Saturación <span class="value" id="sLabel"></span></label>
          <input id="saturation" type="range" min="-2" max="2" step="1" />
        </div>
        <div class="control">
          <label>Exposición (AEC) <span class="value" id="eLabel"></span></label>
          <input id="exposure" type="range" min="0" max="1200" step="10" />
        </div>
      </div>
      <div class="segment"></div>
      <div class="toggles">
        <div class="toggle" id="awbT"><span>AWB</span><div class="pill" id="awbVal"></div></div>
        <div class="toggle" id="agcT"><span>AGC</span><div class="pill" id="agcVal"></div></div>
        <div class="toggle" id="aecT"><span>AEC</span><div class="pill" id="aecVal"></div></div>
        <div class="toggle" id="hmT"><span>H Mirror</span><div class="pill" id="hmVal"></div></div>
        <div class="toggle" id="vfT"><span>V Flip</span><div class="pill" id="vfVal"></div></div>
        <div class="toggle" id="rotT"><span>Rotar 180°</span><div class="pill" id="rotVal"></div></div>
      </div>
    </div>
    <footer>HUD: stream MJPEG + realce en cliente. Mantén alimentación y señal WiFi fuertes para máxima calidad.</footer>
  </div>
  <script>
    const streamEl = document.getElementById('stream');
    const canvasFx = document.getElementById('canvasFx');
    const aiBtn = document.getElementById('aiBtn');
    const refreshBtn = document.getElementById('refreshBtn');
    const ipLabel = document.getElementById('ipLabel');
    const q = document.getElementById('quality');
    const r = document.getElementById('resolution');
    const b = document.getElementById('brightness');
    const c = document.getElementById('contrast');
    const s = document.getElementById('saturation');
    const e = document.getElementById('exposure');
    const qLabel = document.getElementById('qLabel');
    const resLabel = document.getElementById('resLabel');
    const bLabel = document.getElementById('bLabel');
    const cLabel = document.getElementById('cLabel');
    const sLabel = document.getElementById('sLabel');
    const eLabel = document.getElementById('eLabel');
    const heapBar = document.getElementById('heapBar');
    const psramBar = document.getElementById('psramBar');
    const psramVal = document.getElementById('psramVal');
    const tempVal = document.getElementById('tempVal');
    const awbT = document.getElementById('awbT');
    const agcT = document.getElementById('agcT');
    const aecT = document.getElementById('aecT');
    const hmT = document.getElementById('hmT');
    const vfT = document.getElementById('vfT');
    const rotT = document.getElementById('rotT');
    const awbVal = document.getElementById('awbVal');
    const agcVal = document.getElementById('agcVal');
    const aecVal = document.getElementById('aecVal');
    const hmVal = document.getElementById('hmVal');
    const vfVal = document.getElementById('vfVal');
    const rotVal = document.getElementById('rotVal');

    const resMap = {
      'UXGA': 'UXGA',
      'SXGA': 'SXGA',
      'SVGA': 'SVGA',
      'VGA': 'VGA'
    };

    function setToggleState(el, pill, on) {
      el.dataset.state = on ? '1' : '0';
      pill.textContent = on ? 'ON' : 'OFF';
      pill.style.background = on ? 'rgba(45,212,255,0.18)' : 'rgba(255,255,255,0.04)';
    }

    function sendCtrl(varName, val) {
      fetch(`/control?var=${varName}&val=${val}`).catch(() => {});
    }

    async function refreshStats() {
      try {
        const res = await fetch('/stats');
        const data = await res.json();
        const heapTotal = 282748; // aproximado del log
        const heapPct = Math.min(100, Math.max(0, (data.heap_free / heapTotal) * 100));
        const psTotal = data.psram_total || 1;
        const psPct = Math.min(100, Math.max(0, (data.psram_free / psTotal) * 100));
        heapBar.style.width = `${heapPct}%`;
        psramBar.style.width = `${psPct}%`;
        psramVal.textContent = Math.floor(data.psram_free / 1024) + ' KB';
        if (data.temp_c !== undefined) {
          tempVal.textContent = data.temp_c.toFixed(1) + ' °C';
        }
      } catch (e) {
        // ignore
      }
    }

    let aiOn = false;
    let lastFrame = null;
    const ctx = canvasFx.getContext('2d');

    function aiRender() {
      if (!aiOn) {
        requestAnimationFrame(aiRender);
        return;
      }
      if (streamEl.naturalWidth && streamEl.naturalHeight) {
        const w = streamEl.naturalWidth;
        const h = streamEl.naturalHeight;
        canvasFx.width = w;
        canvasFx.height = h;
        ctx.drawImage(streamEl, 0, 0, w, h);
        let frame = ctx.getImageData(0, 0, w, h);
        const data = frame.data;
        if (lastFrame && lastFrame.data.length == data.length) {
          // simple temporal denoise: blend con frame anterior
          for (let i = 0; i < data.length; i += 4) {
            data[i]   = data[i] * 0.65 + lastFrame.data[i] * 0.35;
            data[i+1] = data[i+1] * 0.65 + lastFrame.data[i+1] * 0.35;
            data[i+2] = data[i+2] * 0.65 + lastFrame.data[i+2] * 0.35;
          }
        }
        // micro-sharpen: simple contrast bump
        for (let i = 0; i < data.length; i += 4) {
          data[i]   = Math.min(255, data[i] * 1.03 + 5);
          data[i+1] = Math.min(255, data[i+1] * 1.03 + 5);
          data[i+2] = Math.min(255, data[i+2] * 1.03 + 5);
        }
        ctx.putImageData(frame, 0, 0);
        lastFrame = frame;
      }
      requestAnimationFrame(aiRender);
    }

    function setInitials() {
      const init = {
        quality: {QUALITY},
        frame: "UXGA",
        brightness: {BRIGHTNESS},
        contrast: {CONTRAST},
        saturation: {SATURATION},
        exposure: {AECVAL},
        awb: {AWB},
        agc: {AGC},
        aec: {AEC},
        hmirror: {HFLIP},
        vflip: {VFLIP},
        rotate: {ROTATE180}
      };
      q.value = init.quality;
      qLabel.textContent = init.quality;
      r.value = init.frame;
      r.value = init.frame;
      resLabel.textContent = resMap[init.frame] || init.frame;
      b.value = init.brightness;
      c.value = init.contrast;
      s.value = init.saturation;
      e.value = init.exposure;
      bLabel.textContent = b.value;
      cLabel.textContent = c.value;
      sLabel.textContent = s.value;
      eLabel.textContent = e.value;
      setToggleState(awbT, awbVal, !!init.awb);
      setToggleState(agcT, agcVal, !!init.agc);
      setToggleState(aecT, aecVal, !!init.aec);
      setToggleState(hmT, hmVal, !!init.hmirror);
      setToggleState(vfT, vfVal, !!init.vflip);
      setToggleState(rotT, rotVal, !!init.rotate);
    }

    document.getElementById('startBtn').onclick = () => {
      streamEl.src = streamUrl;
      ipLabel.textContent = 'Streaming...';
    };
    document.getElementById('stopBtn').onclick = () => {
      streamEl.src = '';
      ipLabel.textContent = 'Idle';
    };
    refreshBtn.onclick = () => { location.reload(); };
    aiBtn.onclick = () => {
      aiOn = !aiOn;
      canvasFx.style.opacity = aiOn ? '1' : '0';
      aiBtn.textContent = aiOn ? 'AI Enhance ON' : 'AI Enhance';
      if (!aiOn) lastFrame = null;
    };
    // Auto-start
    const streamUrl = `/stream`;
    streamEl.src = streamUrl;

    q.oninput = (e) => { qLabel.textContent = e.target.value; sendCtrl('quality', e.target.value); };
    r.onchange = (e) => { resLabel.textContent = resMap[e.target.value] || e.target.value; sendCtrl('framesize', e.target.value); };
    b.oninput = (e) => { bLabel.textContent = e.target.value; sendCtrl('brightness', e.target.value); };
    c.oninput = (e) => { cLabel.textContent = e.target.value; sendCtrl('contrast', e.target.value); };
    s.oninput = (e) => { sLabel.textContent = e.target.value; sendCtrl('saturation', e.target.value); };
    e.oninput = (ev) => { eLabel.textContent = ev.target.value; sendCtrl('aec_value', ev.target.value); };

    function toggleHandler(el, pill, varName) {
      el.onclick = () => {
        const next = el.dataset.state === '1' ? 0 : 1;
        setToggleState(el, pill, !!next);
        sendCtrl(varName, next);
      };
    }

    toggleHandler(awbT, awbVal, 'awb');
    toggleHandler(agcT, agcVal, 'agc');
    toggleHandler(aecT, aecVal, 'aec');
    toggleHandler(hmT, hmVal, 'hmirror');
    toggleHandler(vfT, vfVal, 'vflip');
    rotT.onclick = () => {
      const next = rotT.dataset.state === '1' ? 0 : 1;
      setToggleState(rotT, rotVal, !!next);
      // rotar 180 = hmirror + vflip al mismo valor
      sendCtrl('rotate180', next);
      setToggleState(hmT, hmVal, !!next);
      setToggleState(vfT, vfVal, !!next);
    };

    setInitials();
    refreshStats();
    setInterval(refreshStats, 3000);
    requestAnimationFrame(aiRender);
  </script>
</body>
</html>
)rawliteral";

// Información extendida del chip, WiFi y sensor.
void printDiagnostics() {
  Serial.println("===== INFORMACIÓN DEL CHIP =====");
  Serial.printf("Modelo: %s\n", ESP.getChipModel());
  Serial.printf("Revisión silicio: %d\n", ESP.getChipRevision());
  Serial.printf("Cores: %d\n", ESP.getChipCores());
  Serial.printf("Freq CPU: %d MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("SDK: %s\n", ESP.getSdkVersion());
  Serial.printf("Flash: %lu bytes\n", (unsigned long)ESP.getFlashChipSize());
  Serial.printf("PSRAM: %lu bytes (libre: %lu)\n",
                (unsigned long)ESP.getPsramSize(),
                (unsigned long)ESP.getFreePsram());
  Serial.printf("Heap total: %lu, libre: %lu, min libre: %lu\n",
                (unsigned long)ESP.getHeapSize(),
                (unsigned long)ESP.getFreeHeap(),
                (unsigned long)ESP.getMinFreeHeap());
  float tempC = readChipTempC();
  if (tempC > -50 && tempC < 150) {
    Serial.printf("Temperatura chip: %.2f C\n", tempC);
  }

  esp_reset_reason_t reason = esp_reset_reason();
  Serial.printf("Último reset: %d\n", static_cast<int>(reason));

  sensor_t * s = esp_camera_sensor_get();
  if (s) {
    Serial.println("===== INFORMACIÓN DEL SENSOR =====");
    Serial.printf("ID: %d\n", s->id);
    Serial.printf("Framesize: %d\n", s->status.framesize);
    Serial.printf("Brightness: %d, Contrast: %d, Saturation: %d\n",
                  s->status.brightness, s->status.contrast, s->status.saturation);
    Serial.printf("Ajustes: AWB %d, AGC %d, AEC %d\n",
                  s->status.awb, s->status.agc, s->status.aec);
  }

  Serial.println("===== INFORMACIÓN DE WIFI =====");
  Serial.printf("IP local: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("MAC: %s\n", WiFi.macAddress().c_str());
  Serial.printf("Hostname: %s\n", WiFi.getHostname());
}

// Fallback de stream por WebServer cuando el servidor dedicado en 81 no arranca
void handleStream() {
  WiFiClient client = server.client();
  String response = "HTTP/1.1 200 OK\r\n"
                    "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
                    "Access-Control-Allow-Origin: *\r\n\r\n";
  client.print(response);

  while (client.connected()) {
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Frame buffer nulo");
      break;
    }

    client.printf("--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", fb->len);
    client.write(fb->buf, fb->len);
    client.print("\r\n");
    esp_camera_fb_return(fb);

    if (!client.connected()) {
      break;
    }
    delay(5);
  }
}

bool connectToWiFi(const char *ssid, const char *pass, uint8_t maxRetries) {
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOSTNAME);
  WiFi.setSleep(false); // evita ahorro de energía que puede cortar el stream
  WiFi.begin(ssid, pass);

  Serial.printf("Conectando a %s", ssid);
  uint8_t attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < maxRetries) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Conectado. IP: ");
    Serial.println(WiFi.localIP());
    return true;
  }

  Serial.println("No se pudo conectar a WiFi.");
  return false;
}

bool initCameraWithParams(framesize_t fsize, int quality, int fbCount) {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = fsize;
    config.jpeg_quality = quality;
    config.fb_count = fbCount;
    config.fb_location = CAMERA_FB_IN_PSRAM;
  } else {
    config.frame_size = (fsize > FRAMESIZE_SVGA) ? FRAMESIZE_SVGA : fsize;
    config.jpeg_quality = quality + 2;
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }
  config.grab_mode = CAMERA_GRAB_LATEST;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Error iniciando cámara: 0x%x\n", err);
    return false;
  }

  // Ajuste adicional para mejor nitidez
  sensor_t * s = esp_camera_sensor_get();
  if (s) {
    s->set_brightness(s, 1);
    s->set_contrast(s, 1);
    s->set_gainceiling(s, GAINCEILING_2X);
    s->set_lenc(s, 1);  // corrección de lente
    s->set_bpc(s, 1);   // corrije píxeles malos
    s->set_wpc(s, 1);   // corrije píxeles blancos
  }
  return true;
}

bool initCamera() {
  // Intentos escalonados para evitar fallos de malloc en OV3660.
  struct CamTry { framesize_t fs; int q; int fb; };
  CamTry tries[] = {
    {FRAMESIZE_SXGA, 12, 2}, // alta calidad estable
    {FRAMESIZE_XGA,  12, 1},
    {FRAMESIZE_SVGA, 14, 1}
  };

  for (auto t : tries) {
    Serial.printf("Intentando cámara fs=%d quality=%d fb=%d\n", (int)t.fs, t.q, t.fb);
    if (initCameraWithParams(t.fs, t.q, t.fb)) {
      Serial.println("Cámara inicializada");
      return true;
    }
    esp_camera_deinit();
    delay(200);
  }
  return false;
}

String buildIndexPage() {
  String page = FPSTR(INDEX_HTML);
  page.replace("{IP}", WiFi.localIP().toString());
  page.replace("{CHIP}", ESP.getChipModel());
  page.replace("{CPU}", String(ESP.getCpuFreqMHz()));
  page.replace("{PSRAM}", String(ESP.getFreePsram() / 1024));
  page.replace("{HEAP}", String(ESP.getFreeHeap() / 1024));
  page.replace("{HEAP_MIN}", String(ESP.getMinFreeHeap() / 1024));
  float tempC = readChipTempC();
  page.replace("{TEMP}", (tempC > -50 && tempC < 150) ? String(tempC, 1) : String("N/A"));
  sensor_t * s = esp_camera_sensor_get();
  if (s) {
    page.replace("{QUALITY}", String(s->status.quality));
    page.replace("{FRAME}", String((int)s->status.framesize));
    // Determina nombre amigable para el select (independiente de códigos internos)
    String fname = "UXGA";
    int f = (int)s->status.framesize;
    if (f >= FRAMESIZE_UXGA) fname = "UXGA";
    else if (f >= FRAMESIZE_SXGA) fname = "SXGA";
    else if (f >= FRAMESIZE_SVGA) fname = "SVGA";
    else fname = "VGA";
    page.replace("{FRAME_NAME}", fname);
    page.replace("{BRIGHTNESS}", String(s->status.brightness));
    page.replace("{CONTRAST}", String(s->status.contrast));
    page.replace("{SATURATION}", String(s->status.saturation));
    page.replace("{AECVAL}", String(s->status.aec_value));
    page.replace("{AWB}", String(s->status.awb));
    page.replace("{AGC}", String(s->status.agc));
    page.replace("{AEC}", String(s->status.aec));
    page.replace("{HFLIP}", String(s->status.hmirror));
    page.replace("{VFLIP}", String(s->status.vflip));
    page.replace("{ROTATE180}", String((s->status.hmirror && s->status.vflip) ? 1 : 0));
  } else {
    page.replace("{QUALITY}", "10");
    page.replace("{FRAME}", String((int)FRAMESIZE_UXGA));
    page.replace("{FRAME_NAME}", "UXGA");
    page.replace("{BRIGHTNESS}", "0");
    page.replace("{CONTRAST}", "0");
    page.replace("{SATURATION}", "0");
    page.replace("{AECVAL}", "300");
    page.replace("{AWB}", "1");
    page.replace("{AGC}", "1");
    page.replace("{AEC}", "1");
    page.replace("{HFLIP}", "0");
    page.replace("{VFLIP}", "0");
    page.replace("{ROTATE180}", "0");
  }
  return page;
}

void handleRoot() {
  server.send(200, "text/html", buildIndexPage());
}

void handleControl() {
  if (!server.hasArg("var") || !server.hasArg("val")) {
    server.send(400, "text/plain", "Faltan argumentos");
    return;
  }
  sensor_t * s = esp_camera_sensor_get();
  if (!s) {
    server.send(500, "text/plain", "Sensor no disponible");
    return;
  }

  String var = server.arg("var");
  int val = server.arg("val").toInt();
  bool ok = true;

  if (var == "framesize") {
    // Primero intenta con nombre textual, luego con código numérico.
    String v = server.arg("val");
    v.toUpperCase();
    if (v == "UXGA") ok = (s->set_framesize(s, FRAMESIZE_UXGA) == 0);
    else if (v == "SXGA") ok = (s->set_framesize(s, FRAMESIZE_SXGA) == 0);
    else if (v == "SVGA") ok = (s->set_framesize(s, FRAMESIZE_SVGA) == 0);
    else if (v == "VGA") ok = (s->set_framesize(s, FRAMESIZE_VGA) == 0);
    else if (val >= 0 && val <= (int)FRAMESIZE_UXGA) ok = (s->set_framesize(s, (framesize_t)val) == 0);
    else ok = false;
  } else if (var == "quality") {
    ok = (s->set_quality(s, val) == 0);
  } else if (var == "brightness") {
    ok = (s->set_brightness(s, val) == 0);
  } else if (var == "contrast") {
    ok = (s->set_contrast(s, val) == 0);
  } else if (var == "saturation") {
    ok = (s->set_saturation(s, val) == 0);
  } else if (var == "aec_value") {
    ok = (s->set_aec_value(s, val) == 0);
  } else if (var == "awb") {
    ok = (s->set_whitebal(s, val) == 0);
  } else if (var == "agc") {
    ok = (s->set_gain_ctrl(s, val) == 0);
  } else if (var == "aec") {
    ok = (s->set_exposure_ctrl(s, val) == 0);
  } else if (var == "hmirror") {
    ok = (s->set_hmirror(s, val) == 0);
  } else if (var == "vflip") {
    ok = (s->set_vflip(s, val) == 0);
  } else if (var == "rotate180") {
    ok = (s->set_hmirror(s, val) == 0) && (s->set_vflip(s, val) == 0);
  } else {
    ok = false;
  }

  if (ok) {
    server.send(200, "text/plain", "ok");
  } else {
    server.send(400, "text/plain", "parametro invalido");
  }
}

void handleStats() {
  String out = "{";
  out += "\"heap_free\":" + String(ESP.getFreeHeap());
  out += ",\"heap_min\":" + String(ESP.getMinFreeHeap());
  out += ",\"psram_free\":" + String(ESP.getFreePsram());
  out += ",\"psram_total\":" + String(ESP.getPsramSize());
  float t = readChipTempC();
  if (t > -50 && t < 150) {
    out += ",\"temp_c\":" + String(t, 2);
  }
  out += "}";
  server.send(200, "application/json", out);
}

void setup() {
  Serial.begin(115200);
  delay(200);

  if (!connectToWiFi(WIFI_SSID, WIFI_PASS)) {
    while (true) {
      delay(1000);
    }
  }

  if (!initCamera()) {
    Serial.println("Fallo crítico al iniciar cámara.");
    while (true) {
      delay(1000);
    }
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/stream", HTTP_GET, handleStream);
  server.on("/control", HTTP_GET, handleControl);
  server.on("/stats", HTTP_GET, handleStats);
  server.onNotFound([]() {
    server.send(404, "text/plain", "Not found");
  });
  server.begin();

  printDiagnostics();
  Serial.println("Servidor listo. Abre http://" + WiFi.localIP().toString());
}

void loop() {
  server.handleClient();
  if (WiFi.status() != WL_CONNECTED) {
    connectToWiFi(WIFI_SSID, WIFI_PASS);
  }
}
