#include "NetworkManager.h"
#include "RadioEngine.h"
#include "AudioManager.h"
#include "Display.h"
#include "SettingsManager.h"
#include "Hardware.h"
#include "DigitalEngine.h"
#include "EncoderActions.h"
#include <WiFi.h>
#include "arduino_secrets.h"

NetworkManager network;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <meta charset="UTF-8">
  <title>BITFABRIK TRX Control</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    :root { --bg: #121417; --card: #1c1f26; --accent: #00aaff; --text: #e0e6ed; --dim: #94a3b8; --danger: #e53e3e; }
    body { font-family: 'Segoe UI', Tahoma, sans-serif; background: var(--bg); color: var(--text); margin: 0; padding: 10px; }
    .header { padding: 20px; text-align: center; border-bottom: 1px solid #2d3748; margin-bottom: 20px; }
    .header h1 { margin: 0; font-weight: 300; letter-spacing: 2px; color: var(--accent); }
    .dashboard { display: grid; grid-template-columns: repeat(auto-fit, minmax(360px, 1fr)); gap: 20px; max-width: 1200px; margin: 0 auto; }
    .card { background: var(--card); border-radius: 8px; padding: 20px; box-shadow: 0 4px 6px rgba(0,0,0,0.3); border: 1px solid #2d3748; }
    .card h3 { margin-top: 0; font-size: 0.9em; text-transform: uppercase; color: var(--dim); letter-spacing: 1px; border-bottom: 1px solid #2d3748; padding-bottom: 10px; margin-bottom:15px; }
    .freq-display { font-family: 'Courier New', monospace; font-size: 3.5em; color: #ff6600; text-shadow: 0 0 15px rgba(255,102,0,0.3); margin: 0; text-align: center; flex: 1; min-width: 200px;}
    .freq-sub { font-size: 0.6em; color: rgba(255,102,0,0.7); }
    .monitor-layout { display: flex; align-items: center; justify-content: space-around; flex-wrap: wrap; gap: 10px; margin-bottom: 15px; }
    .control-row { display: flex; align-items: center; gap: 10px; margin-top: 15px; flex-wrap: wrap; justify-content: center; }
    .small-input { background: #121417; border: 1px solid #4a5568; color: white; padding: 8px; border-radius: 4px; width: 120px; text-align: center; }
    .step-btn-group { display: flex; gap: 5px; flex-wrap: wrap; justify-content: center; }
    .btn-step { background: #2d3748; border: 1px solid #4a5568; color: var(--text); padding: 5px 10px; border-radius: 4px; cursor: pointer; font-size: 0.8em; }
    .btn-step.active { background: var(--accent); border-color: var(--accent); }
    .btn-nav { background: #2d3748; border: 1px solid #4a5568; color: var(--text); padding: 8px 15px; border-radius: 4px; cursor: pointer; font-weight: bold; }
    .status-line { display: flex; justify-content: space-between; font-size: 1.1em; color: var(--dim); align-items: center; }
    .swr-bar-bg { background: #2d3748; height: 12px; border-radius: 4px; overflow: hidden; position: relative; margin: 10px 0;}
    .swr-bar-fill { background: linear-gradient(90deg, #00ff88, #ffcc00, #ff4444); height: 12px; width: 0%; transition: width 0.3s; }
    .btn-grid { display: grid; gap: 10px; margin-top: 10px; }
    .btn { background: #2d3748; border: 1px solid #4a5568; color: var(--text); padding: 12px; border-radius: 4px; cursor: pointer; transition: all 0.2s; }
    .btn.active { background: var(--accent); border-color: var(--accent); color: white; }
    .btn-send { background: var(--danger); border: none; font-weight: bold; color: white; }
    .terminal { background: #000; border-radius: 4px; height: 180px; overflow-y: scroll; padding: 15px; font-family: 'Consolas', monospace; color: #00ff44; border: 1px solid #2d3748; }
    .carrier-led { width: 12px; height: 12px; border-radius: 50%; background: #2d3748; display: inline-block; margin-right: 8px; }
    .carrier-led.on { background: var(--danger); box-shadow: 0 0 8px var(--danger); }
    .slider { -webkit-appearance: none; width: 100%; height: 8px; border-radius: 4px; background: #2d3748; outline: none; margin-top: 15px; }
    .slider::-webkit-slider-thumb { -webkit-appearance: none; appearance: none; width: 20px; height: 20px; border-radius: 50%; background: var(--accent); cursor: pointer; border: 2px solid white; }
    .pwr-slider::-webkit-slider-thumb { background: var(--danger); }
    .cpu-bar-container { display: flex; gap: 10px; margin-top: 5px; }
    .cpu-bg { background: #2d3748; height: 4px; border-radius: 2px; overflow: hidden; flex: 1; }
    .cpu-fill { height: 100%; width: 0%; transition: width 0.5s; }
    .wheel { width: 150px; height: 150px; border-radius: 50%; background: radial-gradient(circle, #2d3748 0, #1c1f26 105px, #000 150px); border: 8px solid #4a5568; position: relative; touch-action: none; cursor: default; }
    .wheel-marker { position: absolute; top: 15px; left: 50%; width: 10px; height: 30px; background: var(--accent); border-radius: 5px; transform: translateX(-50%); }
    .email-container { display: flex; flex-direction: column; gap: 8px; }
    .email-input { background: #121417; border: 1px solid #4a5568; color: white; padding: 8px; border-radius: 4px; font-size: 0.9em; }

    /* Mode-Aware Styling */
    .status-banner { display: none; background: var(--danger); color: white; padding: 15px; text-align: center; font-weight: bold; border-radius: 8px; margin: 0 auto 20px; max-width: 1200px; box-shadow: 0 4px 15px rgba(229, 62, 62, 0.4); }
    .banner-btn { background: white; color: var(--danger); border: none; padding: 5px 15px; border-radius: 4px; cursor: pointer; font-weight: bold; margin-left: 15px; transition: all 0.2s; }
    .banner-btn:hover { background: #f0f0f0; transform: scale(1.05); }
    body.mode-GEN .status-banner { display: flex; align-items: center; justify-content: center; }
    body.mode-GEN .radio-only { opacity: 0.4; pointer-events: none; filter: grayscale(0.8); }

    @media (max-width: 600px) { .freq-display { font-size: 2.5em; } .dashboard { grid-template-columns: 1fr; } }
  </style>
</head><body>
  <div class="header"><h1>BITFABRIK TRX-1 <span id="ws_conn" style="font-size:0.3em; vertical-align:middle; color:#4a5568">●</span></h1></div>

  <div class="status-banner" id="mode_banner">
    <span>⚠️ SIGNAL GENERATOR ACTIVE - PA OUTPUT DISABLED</span>
    <button onclick="setMode('RADIO')" class="banner-btn">EXIT TO RADIO</button>
  </div>

  <div class="dashboard">
    <!-- Card 1: Live Station (Central Control) -->
    <div class="card">
      <h3>{{H_LIVE_MONITOR}}</h3>
      <div class="monitor-layout">
        <div class="freq-display" id="freq_display">
          <span id="freq_main">0.000</span><span class="freq-sub" id="freq_sub">.000</span>
          <div style="font-size:0.3em; color:var(--dim); margin-top:5px;"><span id="band_name">--</span> | <span id="mode_name">--</span></div>
        </div>
        <div id="tuning_wheel" class="wheel"><div class="wheel-marker"></div></div>
      </div>
      <div class="btn-grid radio-only" id="band-group" style="grid-template-columns: repeat(3, 1fr);">
        <button class="btn" id="b10" onclick="setBand(0)">10m</button><button class="btn" id="b15" onclick="setBand(1)">15m</button><button class="btn" id="b20" onclick="setBand(2)">20m</button>
        <button class="btn" id="b40" onclick="setBand(3)">40m</button><button class="btn" id="b80" onclick="setBand(4)">80m</button><button class="btn" id="b160" onclick="setBand(5)">160m</button>
      </div>
    </div>

    <!-- Card 2: Signal & Health -->
    <div class="card">
      <h3>{{H_SIGNAL_HEALTH}}</h3>
      <div class="status-line"><span>SWR<span id="swr_val">: 1.0</span></span><span style="color:var(--danger)">PWR: <span id="pwr_val">0.0W</span></span></div>
      <div class="swr-bar-bg"><div class="swr-bar-fill" id="swr_bar"></div></div>

      <div style="margin-top:25px; border-top: 1px solid #2d3748; padding-top:15px;">
        <div style="margin-bottom:10px; font-size:0.8em; color:var(--dim)">System Load (CPU)</div>
        <div class="cpu-bar-container">
          <div style="flex:1">
            <div style="font-size:0.65em; margin-bottom:2px">Core 0 (Net) <span id="cpu0_val">0%</span></div>
            <div class="cpu-bg"><div id="cpu0_bar" class="cpu-fill" style="background:#00aaff"></div></div>
          </div>
          <div style="flex:1">
            <div style="font-size:0.65em; margin-bottom:2px">Core 1 (TRX) <span id="cpu1_val">0%</span></div>
            <div class="cpu-bg"><div id="cpu1_bar" class="cpu-fill" style="background:#00ff88"></div></div>
          </div>
        </div>
      </div>
    </div>

    <!-- Card 3: Tuning Controls -->
    <div class="card">
      <h3>{{H_TUNING_CONTROLS}}</h3>
      <div class="control-row radio-only" style="margin-top:0">
        <div class="step-btn-group">
          <button class="btn-step" id="vfo_a_lbl" onclick="selectVfo(0)">VFO A</button>
          <button class="btn-step" id="vfo_b_lbl" onclick="selectVfo(1)">VFO B</button>
          <button class="btn-step" onclick="vfoCopy()">A=B</button>
        </div>
      </div>
      <div class="control-row">
        <button class="btn-nav" onclick="stepFreq(-1)">&#9660;</button>
        <input type="number" id="freq_num" class="small-input" step="10" onchange="setFreq(this.value)">
        <button class="btn-nav" onclick="stepFreq(1)">&#9650;</button>
      </div>
      <div class="control-row">
        <div class="step-btn-group">
          <button class="btn-step" id="s10" onclick="setStep(10)">10</button>
          <button class="btn-step" id="s100" onclick="setStep(100)">100</button>
          <button class="btn-step" id="s1000" onclick="setStep(1000)">1k</button>
          <button class="btn-step" id="s10000" onclick="setStep(10000)">10k</button>
        </div>
      </div>
      <div class="radio-only" style="margin-top:15px; border-top: 1px solid #2d3748; padding-top:15px;">
        <div class="status-line">
          <span style="font-size:0.8em;">Receiver Incremental Tuning</span>
          <div class="step-btn-group">
            <button class="btn-step" id="rit_toggle" onclick="toggleRit()">{{L_RIT_OFF}}</button>
            <button class="btn-step" onclick="setMode('GEN')">GEN</button>
          </div>
        </div>
      </div>
    </div>

    <!-- Card 4: Audio & PA Power -->
    <div class="card radio-only">
      <h3>Audio & PA Power</h3>
      <div class="status-line"><span>{{L_VOL}}</span><span id="vol_val_web" style="color:var(--accent)">50%</span></div>
      <input type="range" id="vol_slider" class="slider" min="0" max="100" oninput="setVolWeb(this.value)">
      <div class="status-line" style="margin-top:15px"><span>{{TX_PWR_CTRL}}</span><span id="pwr_val_web" style="color:var(--danger)">100%</span></div>
      <input type="range" id="pwr_slider" class="slider pwr-slider" min="0" max="100" oninput="setPwrWeb(this.value)">
      <div class="status-line" style="margin-top:15px"><span>{{L_MIC_GAIN}}</span><span id="mic_val_web" style="color:#ffcc00">50%</span></div>
      <input type="range" id="mic_slider" class="slider" min="0" max="100" oninput="setMicWeb(this.value)">
    </div>

    <!-- Card 5: Digital Messaging -->
    <div class="card radio-only">
      <h3>{{H_DIGI_MSG}}</h3>
      <div class="btn-grid" id="digi-group" style="grid-template-columns: 1fr 1fr;">
        <button class="btn" id="m0" onclick="setDigi(0)">{{L_MORSE_MODE}}</button>
        <button class="btn" id="m1" onclick="setDigi(1)">{{L_RTTY_MODE}}</button>
      </div>
      <div style="margin-top:15px; border-top: 1px solid #2d3748; padding-top:15px;">
        <div class="status-line"><span><div id="tx_led" class="carrier-led"></div> {{L_TX_STATUS}}</span><span id="tx_status">{{L_IDLE}}</span></div>
        <div style="display:flex; gap:5px; margin-top:10px">
          <input type="text" id="tx_in" placeholder="{{H_MSG_PLACEHOLDER}}" style="flex:1; background:#121417; border:1px solid #4a5568; color:white; padding:8px;">
          <button id="btn_tx_send" class="btn btn-send" onclick="send()">{{L_SEND}}</button>
        </div>
      </div>
    </div>

    <!-- Card 6: Radio Email -->
    <div class="card radio-only">
      <h3>{{H_MAIL_GW}}</h3>
      <div class="email-container">
        <input type="text" id="mail_to" class="email-input" placeholder="{{L_MAIL_RECIPIENT}} (email@address.com)">
        <textarea id="mail_msg" class="email-input" style="height:60px; resize:none" placeholder="{{L_MAIL_BODY}}" oninput="updateMailCount()"></textarea>
        <div id="mail_count" style="font-size:0.7em; text-align:right">0 / 67</div>
        <div style="display:flex; gap:5px">
          <select id="mail_gw" class="email-input" style="flex:1"><option value="aprs">APRS</option><option value="js8">JS8Call</option></select>
          <button id="btn_mail_send" class="btn btn-send" style="padding:8px 15px" onclick="sendEmail()">{{L_SEND}}</button>
        </div>
      </div>
    </div>

    <!-- Card 7: Memory -->
    <div class="card radio-only">
      <h3>{{H_MEM_CHANNELS}}</h3>
      <div class="btn-grid" style="grid-template-columns: repeat(5, 1fr);">
        <button class="btn" id="mem0" style="padding:8px" disabled onclick="memRecall(0)">M1</button><button class="btn" id="mem1" style="padding:8px" disabled onclick="memRecall(1)">M2</button><button class="btn" id="mem2" style="padding:8px" disabled onclick="memRecall(2)">M3</button><button class="btn" id="mem3" style="padding:8px" disabled onclick="memRecall(3)">M4</button><button class="btn" id="mem4" style="padding:8px" disabled onclick="memRecall(4)">M5</button>
        <button class="btn" id="mem5" style="padding:8px" disabled onclick="memRecall(5)">M6</button><button class="btn" id="mem6" style="padding:8px" disabled onclick="memRecall(6)">M7</button><button class="btn" id="mem7" style="padding:8px" disabled onclick="memRecall(7)">M8</button><button class="btn" id="mem8" style="padding:8px" disabled onclick="memRecall(8)">M9</button><button class="btn" id="mem9" style="padding:8px" disabled onclick="memRecall(9)">M10</button>
      </div>
      <div class="control-row" style="margin-top:15px">
        <span style="font-size:0.7em; color:var(--dim)">{{L_SAVE_TO}}</span>
        <select id="mem_sel" style="background:#121417; color:white; border:1px solid #4a5568; margin:0 5px">
          <option value="0">CH 1</option><option value="1">CH 2</option><option value="2">CH 3</option><option value="3">CH 4</option><option value="4">CH 5</option>
          <option value="5">CH 6</option><option value="6">CH 7</option><option value="7">CH 8</option><option value="8">CH 9</option><option value="9">CH 10</option>
        </select>
        <button class="btn-step" onclick="memStore()">{{L_SICHERN}}</button>
      </div>
    </div>

    <!-- Card 8: System Settings -->
    <div class="card radio-only">
      <h3>System Settings</h3>
      <div class="status-line">
        <span>VOX System</span>
        <button class="btn-step" id="vox_toggle" onclick="toggleVox()">OFF</button>
      </div>
      <div style="margin-top:15px">
        <div class="status-line"><span>VOX Threshold</span><span id="vox_thresh_val" style="color:var(--accent)">1000</span></div>
        <input type="range" id="vox_thresh_slider" class="slider" min="0" max="4095" oninput="setVoxThresh(this.value)">
      </div>
      <div style="margin-top:15px">
        <div class="status-line"><span>VOX Delay (ms)</span><span id="vox_delay_val" style="color:var(--accent)">500ms</span></div>
        <input type="range" id="vox_delay_slider" class="slider" min="0" max="5000" step="50" oninput="setVoxDelay(this.value)">
      </div>
    </div>
  </div>

  <div class="card" style="max-width:1200px; margin:20px auto;">
    <h3>{{H_DECODED_STREAM}}</h3>
    <div class="terminal" id="rx_box">{{L_WAIT_SIGNAL}}</div>
  </div>

  <script>
    const bandIds = ['b10', 'b15', 'b20', 'b40', 'b80', 'b160'];
    let currentFreq = 0, currentStep = 1000, ritOffset = 0, ritEnabled = false;
    let fMin = 0, fMax = 30000000;
    let isVolDragging = false, isPwrDragging = false, isMicDragging = false, isUpdating = false;
    let isVoxThDragging = false, isVoxDelDragging = false;
    let isDragging = false; // Flag for Tuning Wheel
    let lastUserAction = 0; // Timestamp to prevent echo jumps

    function highlight(id, cond) {
        const el = document.getElementById(id);
        if (!el) return;
        if (cond) el.classList.add('active');
        else el.classList.remove('active');
    }

    function formatFreqDisplay(f) {
      const mhz = Math.floor(f / 1000000);
      const khz = Math.floor((f % 1000000) / 1000);
      const hz = f % 1000;
      document.getElementById('freq_main').innerText = mhz + "." + khz.toString().padStart(3, '0');
      document.getElementById('freq_sub').innerText = "." + hz.toString().padStart(3, '0');
    }

    function renderStatus(data) {
        if (!data || data.freq === undefined) return;

        // Synchronize UI Mode (RADIO, GEN, etc.)
        document.body.className = "mode-" + (data.ui_mode || "RADIO");

        // Block server updates if user is interacting or just finished (500ms cooldown)
        const now = Date.now();
        const interacting = isDragging || isVolDragging || isPwrDragging || isMicDragging || isVoxThDragging || isVoxDelDragging || (now - lastUserAction < 500);

        if (!isDragging && (now - lastUserAction > 500)) {
            currentFreq = data.freq;
        }

        ritOffset = data.rit_offset || 0;
        ritEnabled = data.rit_enabled;
        fMin = data.f_min || 0;
        fMax = data.f_max || 30000000;
        if (data.step_val) currentStep = data.step_val;

        formatFreqDisplay(currentFreq);
        if (document.activeElement.id !== 'freq_num' && !interacting) document.getElementById('freq_num').value = currentFreq;

        if(document.getElementById('band_name')) document.getElementById('band_name').innerText = data.band_name;
        document.getElementById('mode_name').innerText = data.usb ? "USB" : "LSB";
        document.getElementById('swr_val').innerText = ": " + data.swr;
        document.getElementById('pwr_val').innerText = data.power_w + "W";
        document.getElementById('swr_bar').style.width = Math.min(100, (parseFloat(data.swr) - 1) * 33) + "%";

        if (!isVolDragging && !interacting) { document.getElementById('vol_slider').value = data.vol; document.getElementById('vol_val_web').innerText = data.vol + "%"; }
        if (!isPwrDragging && !interacting) { document.getElementById('pwr_slider').value = data.pa_pwr; document.getElementById('pwr_val_web').innerText = data.pa_pwr + "%"; }
        if (!isMicDragging && !interacting) { document.getElementById('mic_slider').value = data.mic_gain; document.getElementById('mic_val_web').innerText = data.mic_gain + "%"; }

        document.getElementById('cpu0_bar').style.width = data.cpu0 + "%"; document.getElementById('cpu0_val').innerText = data.cpu0 + "%";
        document.getElementById('cpu1_bar').style.width = data.cpu1 + "%"; document.getElementById('cpu1_val').innerText = data.cpu1 + "%";

        // VOX Update
        const voxBtn = document.getElementById('vox_toggle');
        if (voxBtn && !isVoxThDragging && !isVoxDelDragging && (now - lastUserAction > 500)) {
            voxBtn.innerText = data.vox_en ? "ON" : "OFF";
            highlight('vox_toggle', data.vox_en);
            document.getElementById('vox_thresh_val').innerText = data.vox_thresh;
            document.getElementById('vox_thresh_slider').value = data.vox_thresh;
            document.getElementById('vox_delay_val').innerText = data.vox_delay + "ms";
            document.getElementById('vox_delay_slider').value = data.vox_delay;
        }

        bandIds.forEach((id, idx) => highlight(id, data.band == idx));
        highlight('m0', data.digi == 0);
        highlight('m1', data.digi == 1);
        highlight('vfo_a_lbl', data.vfo_active == 0);
        highlight('vfo_b_lbl', data.vfo_active == 1);
        highlight('rit_toggle', data.rit_enabled);
        highlight('vox_toggle', data.vox_en);

        const ritBtn = document.getElementById('rit_toggle');
        if(ritBtn) {
            ritBtn.innerText = data.rit_enabled ? "{{L_RIT_ON}}" : "{{L_RIT_OFF}}";
        }

        [10, 100, 1000, 10000].forEach(s => highlight('s' + s, data.step_val == s));

        const isTransmitting = data.tx && data.ui_mode !== "GEN";
        const isBusy = data.busy || isTransmitting;
        const isGen = data.ui_mode === "GEN";
        document.getElementById('tx_led').className = data.tx ? "carrier-led on" : "carrier-led";
        document.getElementById('tx_status').innerText = data.tx ? "{{L_SENDING}}" : "{{L_IDLE}}";

        const interactives = ['btn_tx_send', 'btn_mail_send', 'b10', 'b15', 'b20', 'b40', 'b80', 'b160'];
        interactives.forEach(id => {
            const btn = document.getElementById(id);
            if (!btn) return;

            const isVoxControl = id.includes('vox');
            const isLocked = isBusy || (isGen && (btn.classList.contains('btn') || btn.id.includes('send') || isVoxControl));

            btn.disabled = isLocked;
            if (id.includes('send')) {
                btn.innerText = isBusy ? "{{L_WAITING}}" : "{{L_SEND}}";
            }
            btn.style.opacity = isLocked ? "0.5" : "1";
        });

        // Update memory slot buttons
        if (data.mem_slots) {
            const bandNames = ['10m','15m','20m','40m','80m','160m'];
            data.mem_slots.forEach(function(slot, i) {
                const btn = document.getElementById('mem' + i);
                if (!btn) return;
                const occ = slot.occ;
                btn.disabled = !occ;
                btn.style.opacity = occ ? '1' : '0.5';
                if (occ) {
                    const mhz = Math.floor(slot.freq / 1000000);
                    const khz = Math.floor((slot.freq % 1000000) / 1000);
                    const bname = bandNames[slot.band] || '';
                    btn.title = bname + ' ' + mhz + '.' + khz.toString().padStart(3,'0') + ' MHz';
                } else {
                    btn.title = '';
                }
            });
        }
    }

    function update() {
      if (isUpdating) return;
      isUpdating = true;
      fetch('/api/v1/status').then(r => r.json()).then(data => {
        isUpdating = false;
        renderStatus(data);
      }).catch(e => { isUpdating = false; console.error("Update loop failed", e); });
    }

    const wheel = document.getElementById('tuning_wheel');
    let rotation = 0, startAngle = 0, lastRotationAngle = 0, freqAccumulator = 0;

    function getAngle(x, y) {
      const rect = wheel.getBoundingClientRect();
      const centerX = rect.left + rect.width / 2, centerY = rect.top + rect.height / 2;
      return Math.atan2(y - centerY, x - centerX) * 180 / Math.PI;
    }

    function handleStart(e) {
      isDragging = true;
      const cX = e.touches ? e.touches[0].clientX : e.clientX, cY = e.touches ? e.touches[0].clientY : e.clientY;
      startAngle = getAngle(cX, cY); lastRotationAngle = startAngle;
      e.preventDefault();
    }

    function handleMove(e) {
      if (!isDragging) return;
      const cX = e.touches ? e.touches[0].clientX : e.clientX, cY = e.touches ? e.touches[0].clientY : e.clientY;
      const currentAngle = getAngle(cX, cY);
      let delta = currentAngle - lastRotationAngle;
      if (delta > 180) delta -= 360; if (delta < -180) delta += 360;
      rotation += delta; wheel.style.transform = `rotate(${rotation}deg)`;
      freqAccumulator += delta * (currentStep / 10);
      if (Math.abs(freqAccumulator) >= currentStep) {
        let multi = Math.round(freqAccumulator / currentStep);
        stepFreq(multi); freqAccumulator -= (multi * currentStep);
      }
      lastRotationAngle = currentAngle;
    }

    function handleEnd() {
        isDragging = false;
        lastUserAction = Date.now();
    }
    wheel.addEventListener('mousedown', handleStart); window.addEventListener('mousemove', handleMove); window.addEventListener('mouseup', handleEnd);
    wheel.addEventListener('touchstart', handleStart); wheel.addEventListener('touchmove', handleMove); wheel.addEventListener('touchend', handleEnd);

    // API Helpers
    let apiThrottle = false;
    function setFreq(f) {
        f = Math.min(Math.max(parseInt(f), fMin), fMax);
        currentFreq = f;
        formatFreqDisplay(currentFreq);
        lastUserAction = Date.now(); // Mark user activity
        if (apiThrottle) return;
        apiThrottle = true;
        fetch('/api/v1/control?freq=' + currentFreq, { method: 'POST' }).finally(() => { setTimeout(() => { apiThrottle = false; }, 100); });
    }
    function stepFreq(dir) {
        setFreq(currentFreq + (dir * currentStep));
    }
    function setVolWeb(v) {
        lastUserAction = Date.now();
        document.getElementById('vol_val_web').innerText = v + "%";
        fetch('/api/v1/control?vol=' + v, { method: 'POST' });
    }
    function setPwrWeb(v) {
        lastUserAction = Date.now();
        document.getElementById('pwr_val_web').innerText = v + "%";
        fetch('/api/v1/control?pwr=' + v, { method: 'POST' });
    }
    function setMicWeb(v) {
        lastUserAction = Date.now();
        document.getElementById('mic_val_web').innerText = v + "%";
        fetch('/api/v1/control?mic=' + v, { method: 'POST' });
    }
    function selectVfo(i) {
        fetch('/api/v1/control?vfo_set=' + i, { method: 'POST' });
    }
    function vfoCopy() {
        fetch('/api/v1/control?vfo_copy=1', { method: 'POST' });
    }
    function setBand(i) {
        lastUserAction = 0; // Force-accept server's new band frequency, bypass cooldown
        fetch('/api/v1/control?band=' + i, { method: 'POST' });
    }
    function setDigi(i) {
        fetch('/api/v1/control?mode=' + i, { method: 'POST' });
    }
    function setStep(s) {
        currentStep = s;
        document.querySelectorAll('.btn-step').forEach(b => {
            if(b.id.startsWith('s')) b.classList.remove('active');
        });
        document.getElementById('s' + s)?.classList.add('active');
        fetch('/api/v1/control?step=' + s, { method: 'POST' });
    }
    function toggleRit() {
        fetch('/api/v1/control?rit_enable=' + (ritEnabled ? 0 : 1), { method: 'POST' });
    }
    function setMode(m) {
        fetch('/api/v1/control?ui_mode=' + m, { method: 'POST' });
    }
    function stepRit(dir) {
        fetch('/api/v1/control?rit_offset=' + (ritOffset + (dir * currentStep)), { method: 'POST' });
    }
    function resetRit() {
        fetch('/api/v1/control?rit_offset=0', { method: 'POST' });
    }
    function memRecall(i) {
        fetch('/api/v1/control?mem_recall=' + i, { method: 'POST' });
    }
    function memStore() {
        const i = document.getElementById('mem_sel').value;
        fetch('/api/v1/control?mem_store=' + i, { method: 'POST' });
    }
    function toggleVox() {
        const btn = document.getElementById('vox_toggle');
        const nextState = btn.innerText === "OFF";
        btn.innerText = nextState ? "ON" : "OFF";
        highlight('vox_toggle', nextState);
        lastUserAction = Date.now();
        fetch('/api/v1/control?vox_enable=toggle', { method: 'POST' });
    }
    function setVoxThresh(v) {
        lastUserAction = Date.now();
        document.getElementById('vox_thresh_val').innerText = v;
        fetch('/api/v1/control?vox_thresh=' + v, { method: 'POST' });
    }
    function setVoxDelay(v) {
        lastUserAction = Date.now();
        document.getElementById('vox_delay_val').innerText = v + "ms";
        fetch('/api/v1/control?vox_delay=' + v, { method: 'POST' });
    }
    function send() {
        const val = document.getElementById('tx_in').value;
        if(!val) return;
        fetch('/api/v1/transmit?text=' + encodeURIComponent(val), { method: 'POST' });
        document.getElementById('tx_in').value = '';
    }
    function sendEmail() {
        const to = document.getElementById('mail_to').value;
        const msg = document.getElementById('mail_msg').value;
        const gw = document.getElementById('mail_gw').value;
        if(!to || !msg) return;
        fetch(`/api/v1/email?to=${encodeURIComponent(to)}&msg=${encodeURIComponent(msg)}&gw=${gw}`, { method: 'POST' });
        document.getElementById('mail_msg').value = '';
    }
    function updateMailCount() {
        const t = document.getElementById('mail_to').value.length + document.getElementById('mail_msg').value.length + 1;
        document.getElementById('mail_count').innerText = t + " / 67";
    }

    window.onload = () => { update(); initWebSocket(); };
    let ws;
    function initWebSocket() {
      ws = new WebSocket(`ws://${window.location.hostname}/ws`);
      ws.onmessage = (e) => {
        let msg = e.data;
        if (msg.startsWith("RX:")) {
          const box = document.getElementById('rx_box');
          if (box.innerText.includes("Waiting") || box.innerText.includes("Warte")) box.innerText = '';
          box.innerText += msg.substring(3); box.scrollTop = box.scrollHeight;
        } else if (msg.startsWith("JSON_STATUS:")) {
          try { renderStatus(JSON.parse(msg.substring(12))); } catch(e) {}
        }
      };
      ws.onopen = () => {
          document.getElementById('ws_conn').style.color = '#00ff44';
          update(); // Trigger immediate status fetch on connect
          setTimeout(update, 500); // And once more after some initial stabilization
      };
      ws.onclose = () => { document.getElementById('ws_conn').style.color = '#4a5568'; setTimeout(initWebSocket, 2000); };
    }

    document.getElementById('vol_slider').addEventListener('mousedown', () => isVolDragging = true);
    document.getElementById('vol_slider').addEventListener('mouseup', () => isVolDragging = false);
    document.getElementById('vol_slider').addEventListener('touchstart', () => isVolDragging = true);
    document.getElementById('vol_slider').addEventListener('touchend', () => isVolDragging = false);
    document.getElementById('pwr_slider').addEventListener('mousedown', () => isPwrDragging = true);
    document.getElementById('pwr_slider').addEventListener('mouseup', () => isPwrDragging = false);
    document.getElementById('pwr_slider').addEventListener('touchstart', () => isPwrDragging = true);
    document.getElementById('pwr_slider').addEventListener('touchend', () => isPwrDragging = false);
    document.getElementById('mic_slider').addEventListener('mousedown', () => isMicDragging = true);
    document.getElementById('mic_slider').addEventListener('mouseup', () => isMicDragging = false);
    document.getElementById('mic_slider').addEventListener('touchstart', () => isMicDragging = true);
    document.getElementById('mic_slider').addEventListener('touchend', () => isMicDragging = false);

    document.getElementById('vox_thresh_slider').addEventListener('mousedown', () => isVoxThDragging = true);
    document.getElementById('vox_thresh_slider').addEventListener('mouseup', () => isVoxThDragging = false);
    document.getElementById('vox_thresh_slider').addEventListener('touchstart', () => isVoxThDragging = true);
    document.getElementById('vox_thresh_slider').addEventListener('touchend', () => isVoxThDragging = false);

    document.getElementById('vox_delay_slider').addEventListener('mousedown', () => isVoxDelDragging = true);
    document.getElementById('vox_delay_slider').addEventListener('mouseup', () => isVoxDelDragging = false);
    document.getElementById('vox_delay_slider').addEventListener('touchstart', () => isVoxDelDragging = true);
    document.getElementById('vox_delay_slider').addEventListener('touchend', () => isVoxDelDragging = false);
  </script>
</body></html>
)rawliteral";

// Internal helper for static access within AsyncWebServer callbacks
static NetworkManager* _instance = nullptr;

NetworkManager::NetworkManager() : _server(80), _ws("/ws"), _sse("/api/v1/rx/stream")
{
    _instance = this;
}

void NetworkManager::begin()
{
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("TRX-1-Hotspot", "");
    Serial.println(L_AP_START);

    IPAddress static_ip(192, 168, 1, 110), dns(192, 168, 1, 1), gateway(192, 168, 1, 1), subnet(255, 255, 255, 0);
    if (!WiFi.config(static_ip, gateway, subnet, dns))
    {
        Serial.println(L_WIFI_FAIL);
    }

    WiFi.begin(SECRET_SSID, SECRET_PASS);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20)
    {
        vTaskDelay(pdMS_TO_TICKS(500));
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.print(L_WIFI_OK);
        Serial.print(" IP: ");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println(L_WIFI_ERR);
        Serial.print(" AP-IP: ");
        Serial.println(WiFi.softAPIP());
    }

    _ws.onEvent([this](AsyncWebSocket *s, AsyncWebSocketClient *c, AwsEventType t, void *arg, uint8_t *d, size_t l)
    {
        this->_onWsEvent(s, c, t, arg, d, l);
    });

    _server.addHandler(&_ws);
    _server.addHandler(&_sse);

    _setupRoutes();
    _server.begin();

    _lastStatsTime = millis();
}

void NetworkManager::process()
{
    uint32_t startWork = micros();
    unsigned long now = millis();

    if (now - _lastWsCleanup > 2000)
    {
        _ws.cleanupClients();
        _lastWsCleanup = now;
    }

    // Called only when woken by notifyWebUpdate() or 2s timeout — always broadcast.
    broadcastStatus();

    _workTimeAccum += (micros() - startWork);
    if (now - _lastStatsTime > 1000)
    {
        g_cpuLoad0 = (_workTimeAccum * 100) / ((now - _lastStatsTime) * 1000);
        if (WiFi.status() == WL_CONNECTED)
        {
            g_cpuLoad0 += 8;
        }
        if (g_cpuLoad0 > 100)
        {
            g_cpuLoad0 = 100;
        }
        _workTimeAccum = 0;
        _lastStatsTime = now;
    }
}

void NetworkManager::broadcastStatus()
{
    String json;
    json.reserve(1024); // Pre-allocate to avoid repeated heap reallocations
    json = "{";
    json += "\"ui_mode\":\"" + String(ui.getCurrentMode()->getName()) + "\",";
    json += "\"freq\":" + String((long)radio.getFrequency()) + ",";
    json += "\"band\":" + String((int)radio.getBand()) + ",";
    json += "\"band_name\":\"" + String(BANDS[radio.getBand()].label) + "\",";
    json += "\"f_min\":" + String(radio.getMinFreq()) + ",";
    json += "\"f_max\":" + String(radio.getMaxFreq()) + ",";
    json += "\"usb\":" + String(radio.isUsb() ? "true" : "false") + ",";
    json += "\"tx\":" + String(g_tx ? "true" : "false") + ",";
    json += "\"busy\":" + String(digital.isBusy() ? "true" : "false") + ",";
    json += "\"vfo_active\":" + String(radio.getActiveVfo()) + ",";
    json += "\"rit_enabled\":" + String(radio.isRitEnabled() ? "true" : "false") + ",";
    json += "\"rit_offset\":" + String(radio.getRitOffset()) + ",";
    json += "\"step_val\":" + String(STEPS[radio.getStepIdx()]) + ",";
    json += "\"digi\":" + String(digital.getMode()) + ",";
    json += "\"vol\":" + String(audio.getVolume()) + ",";
    json += "\"pa_pwr\":" + String(audio.getPaPower()) + ",";
    json += "\"mic_gain\":" + String(audio.getMicGain()) + ",";
    json += "\"vox_en\":" + String(radio.isVoxEnabled() ? "true" : "false") + ",";
    json += "\"vox_thresh\":" + String(radio.getVoxThreshold()) + ",";
    json += "\"vox_delay\":" + String(radio.getVoxDelay()) + ",";
    json += "\"cpu0\":" + String(g_cpuLoad0) + ",";
    json += "\"cpu1\":" + String(g_cpuLoad1) + ",";
    // ADC reads are not thread-safe — serialize with g_hwMutex
    SWRResult m;
    if (xSemaphoreTakeRecursive(g_hwMutex, pdMS_TO_TICKS(20)))
    {
        m = readSWR();
        xSemaphoreGiveRecursive(g_hwMutex);
    }
    json += "\"swr\":\"" + String(m.swr, 1) + "\",";
    json += "\"power_w\":\"" + String(m.powerW, 1) + "\",";
    json += "\"s_level\":" + String(m.sLevel) + ",";
    json += "\"rssi\":" + String(m.rssi, 2) + ",";
    // Memory slots — 10 slots with occupied flag and stored frequency
    json += "\"mem_slots\":[";
    const VfoState* slots = radio.getMemChannels();
    for (int i = 0; i < NUM_MEM_CHANNELS; i++)
    {
        if (i > 0) json += ",";
        json += "{\"occ\":" + String(slots[i].occupied ? "true" : "false");
        json += ",\"freq\":" + String(slots[i].freq);
        json += ",\"band\":" + String(slots[i].band) + "}";
    }
    json += "]}";
    _ws.textAll("JSON_STATUS:" + json);
}

void NetworkManager::sendToAll(const String& msg)
{
    _ws.textAll(msg);
}

void NetworkManager::sendRxEvent(char c)
{
    _ws.textAll("RX:" + String(c));
    _sse.send(String(c).c_str(), "rx_char");
}

void NetworkManager::_onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    if (type == WS_EVT_CONNECT)
    {
        Serial.printf("WS: Client %u connected (IP: %s)\n", client->id(), client->remoteIP().toString().c_str());
        client->text("RX:*** BITFABRIK TRX ONLINE ***\n");
        if (xSemaphoreTake(g_mutex, pdMS_TO_TICKS(10)))
        {
            const char* log = digital.getRxText();
            xSemaphoreGive(g_mutex);
            if (log[0] != '\0') client->text(String("RX:") + log);
        }
    }
    else if (type == WS_EVT_DISCONNECT)
    {
        Serial.printf("WS: Client %u disconnected\n", client->id());
    }
}

String NetworkManager::getActiveIP()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        return WiFi.localIP().toString();
    }
    return WiFi.softAPIP().toString();
}

void NetworkManager::_setupRoutes()
{
    _server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        request->send(200, "text/html; charset=utf-8", _instance->getIndexHtml());
    });

    _server.on("/api/v1/status", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        String json;
        json.reserve(1024);
        json = "{";
        json += "\"ui_mode\":\"" + String(ui.getCurrentMode()->getName()) + "\",";
        json += "\"freq\":" + String((long)radio.getFrequency()) + ",";
        json += "\"band\":" + String((int)radio.getBand()) + ",";
        json += "\"band_name\":\"" + String(BANDS[radio.getBand()].label) + "\",";
        json += "\"f_min\":" + String(radio.getMinFreq()) + ",";
        json += "\"f_max\":" + String(radio.getMaxFreq()) + ",";
        json += "\"usb\":" + String(radio.isUsb() ? "true" : "false") + ",";
        json += "\"tx\":" + String(g_tx ? "true" : "false") + ",";
        json += "\"busy\":" + String(digital.isBusy() ? "true" : "false") + ",";
        json += "\"vfo_active\":" + String(radio.getActiveVfo()) + ",";
        json += "\"rit_enabled\":" + String(radio.isRitEnabled() ? "true" : "false") + ",";
        json += "\"rit_offset\":" + String(radio.getRitOffset()) + ",";
        json += "\"step_val\":" + String(STEPS[radio.getStepIdx()]) + ",";
        json += "\"digi\":" + String(digital.getMode()) + ",";
        json += "\"vol\":" + String(audio.getVolume()) + ",";
        json += "\"pa_pwr\":" + String(audio.getPaPower()) + ",";
        json += "\"mic_gain\":" + String(audio.getMicGain()) + ",";
        json += "\"vox_en\":" + String(radio.isVoxEnabled() ? "true" : "false") + ",";
        json += "\"vox_thresh\":" + String(radio.getVoxThreshold()) + ",";
        json += "\"vox_delay\":" + String(radio.getVoxDelay()) + ",";
        json += "\"cpu0\":" + String(g_cpuLoad0) + ",";
        json += "\"cpu1\":" + String(g_cpuLoad1) + ",";
        SWRResult m;
        if (xSemaphoreTakeRecursive(g_hwMutex, pdMS_TO_TICKS(20)))
        {
            m = readSWR();
            xSemaphoreGiveRecursive(g_hwMutex);
        }
        json += "\"swr\":\"" + String(m.swr, 1) + "\",";
        json += "\"power_w\":\"" + String(m.powerW, 1) + "\",";
        json += "\"s_level\":" + String(m.sLevel) + ",";
        json += "\"rssi\":" + String(m.rssi, 2) + ",";
        // Memory slots
        json += "\"mem_slots\":[";
        const VfoState* slots = radio.getMemChannels();
        for (int i = 0; i < NUM_MEM_CHANNELS; i++)
        {
            if (i > 0) json += ",";
            json += "{\"occ\":" + String(slots[i].occupied ? "true" : "false");
            json += ",\"freq\":" + String(slots[i].freq);
            json += ",\"band\":" + String(slots[i].band) + "}";
        }
        json += "]}";
        request->send(200, "application/json", json);
    });

    _server.on("/api/v1/control", HTTP_ANY, [](AsyncWebServerRequest *request)
    {
        bool handled = false;
        if (request->hasParam("ui_mode"))
        {
            ui.setMode(request->getParam("ui_mode")->value().c_str());
            handled = true;
        }
        if (request->hasParam("freq"))
        {
            radio.setFrequency(request->getParam("freq")->value().toInt());
            handled = true;
        }
        if (request->hasParam("vol"))
        {
            encManager.setMode(EncoderMode::Volume);
            audio.setVolume(request->getParam("vol")->value().toInt());
            handled = true;
        }
        if (request->hasParam("pwr"))
        {
            encManager.setMode(EncoderMode::Power);
            audio.setPaPower(request->getParam("pwr")->value().toInt());
            handled = true;
        }
        if (request->hasParam("mic"))
        {
            encManager.setMode(EncoderMode::Mic);
            audio.setMicGain(request->getParam("mic")->value().toInt());
            handled = true;
        }
        if (request->hasParam("band"))
        {
            radio.selectBand(request->getParam("band")->value().toInt());
            handled = true;
        }
        if (request->hasParam("mode"))
        {
            digital.setMode(request->getParam("mode")->value().toInt());
            g_guiNeedsUpdate = true;
            handled = true;
        }
        if (request->hasParam("vfo_set"))
        {
            radio.switchVfo(request->getParam("vfo_set")->value().toInt());
            handled = true;
        }
        if (request->hasParam("vfo_copy"))
        {
            radio.vfoCopy();
            handled = true;
        }
        if (request->hasParam("rit_enable"))
        {
            radio.setRitEnabled(request->getParam("rit_enable")->value().toInt() == 1);
            handled = true;
        }
        if (request->hasParam("rit_offset"))
        {
            radio.setRitOffset(request->getParam("rit_offset")->value().toInt());
            handled = true;
        }
        if (request->hasParam("mem_store"))
        {
            radio.memStore(request->getParam("mem_store")->value().toInt());
            handled = true;
        }
        if (request->hasParam("mem_recall"))
        {
            radio.memRecall(request->getParam("mem_recall")->value().toInt());
            handled = true;
        }
        if (request->hasParam("step"))
        {
            int s = request->getParam("step")->value().toInt();
            for(int i = 0; i < NUM_STEPS; i++)
            {
                if(STEPS[i] == s)
                {
                    radio.setStepIdx(i);
                    break;
                }
            }
            handled = true;
        }
        if (request->hasParam("vox_enable"))
        {
            radio.setVoxEnabled(!radio.isVoxEnabled());
            handled = true;
        }
        if (request->hasParam("vox_thresh"))
        {
            radio.setVoxThreshold(request->getParam("vox_thresh")->value().toInt());
            handled = true;
        }
        if (request->hasParam("vox_delay"))
        {
            radio.setVoxDelay(request->getParam("vox_delay")->value().toInt());
            handled = true;
        }

        if (handled)
        {
            settings.setUpdated();
            notifyWebUpdate();
            request->send(200, "text/plain", "OK");
        }
        else
        {
            request->send(400, "text/plain", "Unknown Command");
        }
    });

    _server.on("/api/v1/email", HTTP_ANY, [](AsyncWebServerRequest *request)
    {
        if (digital.isBusy())
        {
            request->send(409, "text/plain", "BUSY");
            return;
        }
        String to = request->getParam("to")->value();
        String msg = request->getParam("msg")->value();
        String gw = request->getParam("gw")->value();
        String formatted = (gw == "js8") ? "@APRSIS CMD :EMAIL-2 :" + to + " " + msg + " {01}" : "EMAIL-2 :" + to + " " + msg;
        if (formatted.length() > 100) formatted = formatted.substring(0, 100);
        if (xSemaphoreTake(g_mutex, pdMS_TO_TICKS(50)))
        {
            digital.queue.pushString(formatted);
            xSemaphoreGive(g_mutex);
        }
        notifyWebUpdate();
        request->send(200, "text/plain", "OK");
    });

    _server.on("/api/v1/transmit", HTTP_ANY, [](AsyncWebServerRequest *request)
    {
        if (digital.isBusy())
        {
            request->send(409, "text/plain", "BUSY");
            return;
        }
        if (request->hasParam("text"))
        {
            String text = request->getParam("text")->value();
            if (xSemaphoreTake(g_mutex, pdMS_TO_TICKS(50)))
            {
                digital.queue.pushString(text);
                xSemaphoreGive(g_mutex);
            }
            notifyWebUpdate();
        }
        request->send(200, "text/plain", "OK");
    });
}

String NetworkManager::getIndexHtml()
{
  String s;
  s.reserve(24000);
  s = String(index_html);

  // Headers & Cards
  s.replace("{{H_LIVE_MONITOR}}",   H_LIVE_MONITOR);
  s.replace("{{H_SIGNAL_HEALTH}}",  H_SIGNAL_HEALTH);
  s.replace("{{H_TUNING_CONTROLS}}", H_TUNING_CONTROLS);
  s.replace("{{H_DIGI_MSG}}",       H_DIGI_MSG);
  s.replace("{{H_MAIL_GW}}",        H_MAIL_GW);
  s.replace("{{H_MEM_CHANNELS}}",   H_MEM_CHANNELS);
  s.replace("{{H_DECODED_STREAM}}", H_DECODED_STREAM);
  s.replace("{{H_BAND_MODE}}",      H_BAND_MODE);

  // Tuning Controls
  s.replace("{{L_RIT_OFF}}",        L_RIT_OFF);
  s.replace("{{L_RIT_ON}}",         L_RIT_ON);

  // Audio & Power
  s.replace("{{L_VOL}}",            L_VOL);
  s.replace("{{TX_PWR_CTRL}}",      TX_PWR_CTRL);
  s.replace("{{L_MIC_GAIN}}",       L_MIC_GAIN);

  // Digital & Status
  s.replace("{{L_MORSE_MODE}}",     L_MORSE_MODE);
  s.replace("{{L_RTTY_MODE}}",      L_RTTY_MODE);
  s.replace("{{L_TX_STATUS}}",      L_TX_STATUS);
  s.replace("{{L_IDLE}}",           L_IDLE);
  s.replace("{{L_SENDING}}",        L_SENDING);
  s.replace("{{L_WAITING}}",        L_WAITING);
  s.replace("{{L_SEND}}",           L_SEND);

  // Messaging Labels
  s.replace("{{H_MSG_PLACEHOLDER}}", H_MSG_PLACEHOLDER);
  s.replace("{{L_MAIL_RECIPIENT}}", L_MAIL_RECIPIENT);
  s.replace("{{L_MAIL_BODY}}",      L_MAIL_BODY);

  // Memory & Decoded Stream
  s.replace("{{L_SAVE_TO}}",        L_SAVE_TO);
  s.replace("{{L_SICHERN}}",        L_SICHERN);
  s.replace("{{L_WAIT_SIGNAL}}",    L_WAIT_SIGNAL);

  return s;
}

static int getBandIndexByName(String name)
{
  name.toLowerCase();
  for (int i = 0; i < NUM_BANDS; i++)
  {
    String bandName = String(BANDS[i].label);
    bandName.toLowerCase();
    if (name == bandName)
    {
        return i;
    }
  }
  return -1;
}

void TaskNetwork(void *p)
{
    g_networkTaskHandle = xTaskGetCurrentTaskHandle();
    network.begin();
    for (;;)
    {
        // Sleep until notifyWebUpdate() wakes us, or 2s for periodic cleanup/stats.
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(2000));
        network.process();
    }
}
