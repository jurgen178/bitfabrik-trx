#include "Hardware.h"
#include "Display.h"
#include "EncoderActions.h"
#include "RadioEngine.h"
#include "AudioManager.h"
#include "SettingsManager.h"
#include "NetworkManager.h"
#include "DigitalEngine.h"
#include "LogManager.h"
#include <SPI.h>

/**
 * ── DDS BUS DRIVER ────────────────────────────────────────────────────────
 * Implements a shared bus for multiple AD9850 modules.
 * Standard wiring:
 * - DATA/WCLK/RESET: Parallel to all modules.
 * - FQUD: Dedicated signal per module (serves as Chip Select).
 * ──────────────────────────────────────────────────────────────────────────
 */

void dds_pulse(int pin)
{
  digitalWrite(pin, HIGH);
  delayMicroseconds(1);
  digitalWrite(pin, LOW);
}

/**
 * Transmits a 40-bit Frequency Tuning Word (FTW) to the selected module.
 */
void dds_setFreq(double freq, int fqud)
{
  if (xSemaphoreTakeRecursive(g_hwMutex, portMAX_DELAY))
  {
    // Formula: FTW = (Freq * 2^32) / Reference_Clock
    uint32_t tw = (uint32_t)((freq / REF_FREQ) * 4294967296.0);

    // Serial transfer (LSB first)
    for (int i = 0; i < 32; i++)
    {
      digitalWrite(DDS_DATA, (tw >> i) & 1);
      dds_pulse(DDS_WCLK);
    }
    // Phase and Control bits (trailing 8 bits, all 0)
    for (int i = 0; i < 8; i++)
    {
      digitalWrite(DDS_DATA, LOW);
      dds_pulse(DDS_WCLK);
    }
    // Frequency Update Pulse (Latch data to DAC)
    dds_pulse(fqud);
    xSemaphoreGiveRecursive(g_hwMutex);
  }
}

/**
 * Hardware Reset for the serial bus interfaces.
 */
void dds_reset()
{
  if (xSemaphoreTakeRecursive(g_hwMutex, portMAX_DELAY))
  {
    digitalWrite(DDS_RESET, HIGH);
    delayMicroseconds(5);
    digitalWrite(DDS_RESET, LOW);
    delayMicroseconds(5);
    dds_pulse(DDS_WCLK); // Sync serial interface
    // Latch both modules to clean state
    dds_pulse(LO_FQUD);
    dds_pulse(BFO_FQUD);
    xSemaphoreGiveRecursive(g_hwMutex);
  }
}

/**
 * ── RF SEQUENCER ──────────────────────────────────────────────────────────
 * Manages the transition between Receive and Transmit modes.
 * Ensures antenna relays and PA bias are switched with safe timing.
 * ──────────────────────────────────────────────────────────────────────────
 */

/**
 * Physical TX/RX switching logic.
 * NOTE: Band filters are now handled only on band selection for stability.
 */
void setTxRx(bool tx)
{
  g_tx = tx;
  logger.notifyTxState(tx);

  if (xSemaphoreTakeRecursive(g_hwMutex, portMAX_DELAY))
  {
    if (g_mcpOk)
    {
      // PA is only active if TX is requested AND we are NOT in Generator Mode (unlocked)
      bool paState = tx && !radio.isUnlocked();
      mcp.digitalWrite(MCP_TX_PA_ACTIVE, paState ? HIGH : LOW);
    }

    // Re-latch band filter relays inside the same mutex block for stability
    radio.refreshRelays();

    xSemaphoreGiveRecursive(g_hwMutex);
  }

  notifyWebUpdate();
}

// ── VFO & STORAGE ─────────────────────────────────────────────────────────

// ── ANALOG/PWM CONTROL ────────────────────────────────────────────────────

/**
 * ── METERING ENGINE ───────────────────────────────────────────────────────
 * Samples Stockton bridge voltages and applies diode knee compensation.
 * ──────────────────────────────────────────────────────────────────────────
 */

static float compensateDiode(float v)
{
  if (v <= 0.001f)
  {
      return 0.0f;
  }
  // Non-linear correction for BAT85 Schottky diode drop
  if (v < SWR_DIODE_VF)
  {
      return sqrtf(v * SWR_DIODE_VF);
  }
  return v + SWR_DIODE_VF;
}

SWRResult readSWR()
{
  uint32_t fSum = 0;
  uint32_t rSum = 0;
  uint32_t rssiSum = 0;

  SWRResult res;

  if (xSemaphoreTakeRecursive(g_hwMutex, portMAX_DELAY))
  {
    for (int i=0; i<SWR_SAMPLES; i++)
    {
        fSum += analogRead(PIN_SWR_FWD);
        rSum += analogRead(PIN_SWR_REF);
        rssiSum += analogRead(PIN_S_METER);
    }
    xSemaphoreGiveRecursive(g_hwMutex);
  }

  float vF = (static_cast<float>(fSum) / SWR_SAMPLES) * (SWR_V_REF / SWR_ADC_MAX);
  float vR = (static_cast<float>(rSum) / SWR_SAMPLES) * (SWR_V_REF / SWR_ADC_MAX);
  float vRSSI = (static_cast<float>(rssiSum) / SWR_SAMPLES) * (SWR_V_REF / SWR_ADC_MAX);

  res.vFwd = compensateDiode(vF);
  res.vRef = compensateDiode(vR);
  res.rssi = vRSSI;

  // RX Mode: Calculate S-Level
  if (!g_tx && !digital.isBusy())
  {
    if (vRSSI < 0.05f) {
#ifdef DEBUG_SIM
      res.rssi = 0.05f + static_cast<float>(random(0, 100)) / 1000.0f; // simulated noise floor
#else
      res.rssi = 0.0f;
#endif
    }

    // Typical S-Unit mapping (example: S9 = 50uV at ant, but here it's DC level from RX)
    // We map 0..1.0V to S1..S9 for now.
    res.sLevel = static_cast<int>(res.rssi * 9.0f);
    if (res.sLevel < 1)
        res.sLevel = 1;
    if (res.sLevel > 9)
        res.sLevel = 9;

    res.swr = 1.0f;
    res.powerW = 0.0f;
  }
  else
  {
    // TX Mode: Calculate SWR/Power
    res.powerW = res.vFwd * res.vFwd * 4.0f;
    if (res.vFwd > SWR_MIN_VFWD)
    {
      float diff = res.vFwd - res.vRef;
      res.swr = (diff > 0.001f) ? (res.vFwd + res.vRef) / diff : 9.9f;
    }
    else
    {
        res.swr = 1.0f;
    }
    if (res.swr > 9.9f)
        res.swr = 9.9f;
    if (res.swr < 1.0f)
        res.swr = 1.0f;
    res.sLevel = 0;
  }

  return res;
}

// ── ISR HANDLERS ──────────────────────────────────────────────────────────

void IRAM_ATTR encISR_Optical()
{
  uint8_t s = (digitalRead(ENC_A) << 1) | digitalRead(ENC_B);
  uint8_t index = (g_oldEncState << 2) | s;
  int8_t move = ENC_STATES[index];
  if (move != 0)
  {
    s_encAccum += move;
    if (abs(s_encAccum) >= ENC_STEPS_PER_DETENT)
    {
      if (s_encAccum > 0)
      {
          g_encPos++;
      }
      else
      {
          g_encPos--;
      }
      unsigned long now = millis();
      g_encInterval = now - g_lastEncMove;
      g_lastEncMove = now;
      s_encAccum = 0;
    }
  }
  g_oldEncState = s;
}

void IRAM_ATTR encISR()
{
  uint8_t s = (digitalRead(ENC_A) << 1) | digitalRead(ENC_B);
  uint8_t index = (g_oldEncState << 2) | s;
  int8_t move = ENC_STATES[index];
  if (move != 0)
  {
    s_encAccum += move;
    if (s_encAccum >= 4)
    {
        g_encPos++;
        unsigned long now = millis();
        g_encInterval = now - g_lastEncMove;
        g_lastEncMove = now;
        s_encAccum -= 4;
    }
    else if (s_encAccum <= -4)
    {
        g_encPos--;
        unsigned long now = millis();
        g_encInterval = now - g_lastEncMove;
        g_lastEncMove = now;
        s_encAccum += 4;
    }
  }
  g_oldEncState = s;
}

// ── TASK RADIO HELPERS ──────────────────────────────────────────────────

static int pttC = 0;
static unsigned long lastKeyTime = 0;
static unsigned long lastVoxTrigger = 0;
static bool lastKeyTimeValid = false;
static bool lastVoxTriggerValid = false;

void handlePTT(unsigned long now, bool mcpPtt)
{
    // 0. Generator Mode Bypass
    if (radio.isUnlocked()) return;

    // 1. Physical PTT logic with safety check (Avoid TX if MCP is disconnected/unstable)
    bool rawP = (g_mcpOk && mcpPtt == LOW);
    if (rawP)
    {
        if (pttC < 5)
            pttC++;
    }
    else
    {
        if (pttC > 0)
            pttC--;
    }

    // 2. VOX Logic
    bool voxTriggered = false;
    if (radio.isVoxEnabled() && !rawP)
    {
        int voxLevel = analogRead(PIN_VOX_ADC);
        if (voxLevel > radio.getVoxThreshold())
        {
            lastVoxTrigger = now;
            lastVoxTriggerValid = true;
        }
        // Safety: ensure triggered only if threshold is not zero
        if (radio.getVoxThreshold() > 50 && lastVoxTriggerValid && (now - lastVoxTrigger < (unsigned long)radio.getVoxDelay()))
        {
            voxTriggered = true;
        }
    }

    if (digital.isKeyed()) {
        lastKeyTime = now;
        lastKeyTimeValid = true;
    }
    // isBusy keeps TX on during inter-char/word gaps (which exceed CW_HANG_TIME)
    bool shouldBeInTx = (pttC >= 4) || digital.isKeyed() || digital.isBusy() || voxTriggered || (lastKeyTimeValid && (now - lastKeyTime < CW_HANG_TIME));

    if (shouldBeInTx != g_tx)
    {
        setTxRx(shouldBeInTx);
        g_guiNeedsUpdate = true;
        if (!shouldBeInTx)
            radio.updateLO();
    }

    bool ledActive = digital.isKeyed() || rawP || voxTriggered;
    digitalWrite(LED_RED, ledActive ? LOW : HIGH);
}

static void handleSoftKeying(bool mcpPtt)
{
    static bool lastKeyedState = false;
    bool physicalPttActive = (mcpPtt == LOW);

    if (g_tx && digital.getMode() == 0 && !physicalPttActive)
    {
        if (digital.isKeyed() != lastKeyedState)
        {
            if (digital.isKeyed()) radio.updateLO();
            else dds_setFreq(0, LO_FQUD);
            lastKeyedState = digital.isKeyed();
        }
    }
}

static void handleEncoder(unsigned long now)
{
    static int lastP = 0;
    static bool lastB = HIGH;
    static unsigned long btnD = 0;

    if (!ui.getCurrentMode()) return;

    // 1. Rotation Event
    int pos = g_encPos;
    if (pos != lastP)
    {
        ui.getCurrentMode()->onRotate(pos - lastP);
        lastP = pos;
    }

    // 2. Button Event
    int btn = digitalRead(ENC_BTN);
    if (btn != lastB)
    {
        if (btn == LOW)
            btnD = now;
        else
        {
            unsigned long dur = now - btnD;
            if (dur > 50 && dur < ENC_LONG_PRESS_MS)
            {
                ui.getCurrentMode()->onButtonShort();
            }
            else if (dur >= ENC_LONG_PRESS_MS)
            {
                radio.setStepIdx((radio.getStepIdx() + 1) % NUM_STEPS);
                g_guiNeedsUpdate = true;
            }
        }
        lastB = btn;
        vTaskDelay(pdMS_TO_TICKS(5)); // debounce
    }
}

static void handleTouch(unsigned long now)
{
    static bool wasTouching = false;
    static unsigned long touchStart = 0;
    static int touchX = 0, touchY = 0;

    int tx, ty;
    bool isTouching = false;

    // Crucial Sync-Fix: If we cannot acquire the hardware mutex immediately,
    // we MUST abort this check completely and try again next cycle.
    // Do NOT assume the user released their finger!
    if (!xSemaphoreTakeRecursive(g_hwMutex, pdMS_TO_TICKS(5)))
    {
        return;
    }
    isTouching = tft.getTouch(&tx, &ty);
    xSemaphoreGiveRecursive(g_hwMutex);

    if (isTouching && !wasTouching)
    {
        // Finger down — record start position and time
        touchStart = now;
        touchX = tx;
        touchY = ty - 16;
        // Serial.printf("Touch Debug -> Raw X:%d, Raw Y:%d | Corrected Y:%d\n", tx, ty, touchY);
        wasTouching = true;
    }
    else if (!isTouching && wasTouching)
    {
        // Finger up — dispatch as long-press (≥500 ms) or short-press
        unsigned long dur = now - touchStart;
        if (ui.getCurrentMode())
            ui.getCurrentMode()->handleTouch(touchX, touchY, dur >= 500);
        wasTouching = false;
    }
}

static void handleDisplayUpdates(unsigned long now)
{
    static unsigned long lastTftRefresh = 0;
    static unsigned long lastOled1Refresh = 0;
    static unsigned long lastOled2Refresh = 0;
    static unsigned long lastDummyDecode = 0;
    static int dummyIdx = 0;
    const char dummyMessage[] = "BITFABRIK ";

    // 1. TFT Refresh (Throttled to ~16Hz for tuning smoothness)
    if (now - lastTftRefresh > 60)
    {
        if (xSemaphoreTakeRecursive(g_hwMutex, portMAX_DELAY))
        {
            // Always check for mode timeouts (3s return to Radio Mode)
            ui.checkTimeout();

            if (g_guiNeedsUpdate.exchange(false))
            {
                ui.update();
                updateOled1(); // High priority update
                notifyWebUpdate(); // Hardware state changed — keep web in sync
                lastOled1Refresh = now; // Sync standalone cycle
            }
            xSemaphoreGiveRecursive(g_hwMutex);
        }
        lastTftRefresh = now;
    }

    // 2. OLED1 (Fast Cycle)
    if (now - lastOled1Refresh > 200)
    {
        updateOled1();
        lastOled1Refresh = now;
    }

    // 3. OLED2 (Slow Cycle + Ticker)
    if (now - lastOled2Refresh > 1000)
    {
        updateOled2();
        lastOled2Refresh = now;

        if (now - lastDummyDecode > 8000)
        {
#ifdef DEBUG_SIM
            if (xSemaphoreTake(g_mutex, pdMS_TO_TICKS(5)))
            {
                char c = dummyMessage[dummyIdx];
                digital.addRxChar(c);
                dummyIdx = (dummyIdx + 1) % (sizeof(dummyMessage) - 1);
                network.sendRxEvent(c);
                xSemaphoreGive(g_mutex);
            }
#endif
            lastDummyDecode = now;
        }
    }
}

static void handleSystemStats(unsigned long now, uint32_t startWork)
{
    static unsigned long lastStatsTime = 0;
    static uint32_t workTimeAccum = 0;

    workTimeAccum += (micros() - startWork);
    if (now - lastStatsTime > 1000)
    {
        g_cpuLoad1 = (workTimeAccum * 100) / ((now - lastStatsTime) * 1000);
        if (g_cpuLoad1 > 100)
            g_cpuLoad1 = 100;
        workTimeAccum = 0;
        lastStatsTime = now;
    }
}

/**
 * ── RADIO ORCHESTRATOR TASK (CORE 1) ──────────────────────────────────────
 * Main loop for high-priority radio operations:
 * - Real-time PTT & Sequencer management.
 * - Soft-Keying for Digital Modes.
 * - Modular Encoder delegation.
 * - Throttled hardware display refreshes (I2C protection).
 * ──────────────────────────────────────────────────────────────────────────
 */
static void handleSyncState(unsigned long now)
{
    // 1. Command Processing (Read Targets from Web)
    // We process this first to ensure hardware reacts to web commands immediately
    if (g_sync.updatePending.exchange(false))
    {
        uint32_t mask = g_sync.updateMask.exchange(0);
        bool silent = (mask & SYNC_NO_UI);

        if (mask & SYNC_FREQ)  radio.setFrequency(g_sync.targetFreq.load());
        if (mask & SYNC_BAND)  radio.selectBand(g_sync.targetBand.load());
        if (mask & SYNC_VFO)   radio.switchVfo(g_sync.targetVfo.load());
        if (mask & SYNC_VFO_COPY) {
            radio.vfoCopy();
            g_sync.vfoCopyFlash = true;
        }
        if (mask & SYNC_RIT) {
            radio.setRitEnabled(g_sync.currRitEn.load()); // curr holds the enable bit during command
            radio.setRitOffset(g_sync.targetRitOffset.load());
        }
        if (mask & SYNC_STEP)  radio.setStepIdx(g_sync.targetStepIdx.load());
        if (mask & SYNC_DIGI)  digital.setMode(g_sync.targetDigiMode.load());
        if (mask & SYNC_VOL) {
            audio.setVolume(g_sync.targetVol.load());
            if (!silent) ui.setMode(DisplayMode::Volume);
        }
        if (mask & SYNC_PWR) {
            audio.setPaPower(g_sync.targetPwr.load());
            if (!silent) ui.setMode(DisplayMode::Power);
        }
        if (mask & SYNC_MIC) {
            audio.setMicGain(g_sync.targetMic.load());
            if (!silent) ui.setMode(DisplayMode::Mic);
        }
        if (mask & SYNC_VOX) {
            // Check if this was a toggle (from web) or a specific update
            // For now, if both target values are unchanged, it's a toggle
            radio.setVoxEnabled(g_sync.currVoxEn.load());
            radio.setVoxThreshold(g_sync.targetVoxThresh.load());
            radio.setVoxDelay(g_sync.targetVoxDelay.load());
        }
        if (mask & SYNC_MEM_STORE)  radio.memStore(g_sync.targetVfo.load()); // reusing targetVfo for slot
        if (mask & SYNC_MEM_RECALL) radio.memRecall(g_sync.targetVfo.load());

        if (mask & SYNC_MODE) {
            // We use targetStepIdx as a proxy for mode index to avoid another atomic
            if (!silent) ui.setMode(static_cast<DisplayMode>(g_sync.targetStepIdx.load()));
        }

        g_guiNeedsUpdate = true;
    }

    // 2. State Mirroring (Write current Hardware state to RAM for Web)
    // We do this every cycle so the Web always has the latest data
    long  f = radio.getFrequency();
    int   b = radio.getBand();
    bool  u = radio.isUsb();
    int   v = radio.getActiveVfo();
    bool  re = radio.isRitEnabled();
    long  ro = radio.getRitOffset();
    bool  ve = radio.isVoxEnabled();
    int   vt = radio.getVoxThreshold();
    int   vd = radio.getVoxDelay();
    int   vol = audio.getVolume();
    int   pwr = audio.getPaPower();
    int   mic = audio.getMicGain();

    int   mIdx = 0;
    const char* mName = ui.getCurrentMode() ? ui.getCurrentMode()->getName() : "";
    if (strcmp(mName, "GEN") == 0) mIdx = 1;
    else if (strcmp(mName, "SETTINGS") == 0) mIdx = 2;
    else if (strcmp(mName, "RIT") == 0) mIdx = 3;

    bool changed = (f != g_sync.currFreq || b != g_sync.currBand || u != g_sync.currUsb ||
                    v != g_sync.currVfo || re != g_sync.currRitEn || ro != g_sync.currRitOff ||
                    ve != g_sync.currVoxEn || vt != g_sync.currVoxThresh || vd != g_sync.currVoxDelay ||
                    vol != g_sync.currVol || pwr != g_sync.currPwr || mic != g_sync.currMic ||
                    mIdx != g_sync.currModeIdx);

    g_sync.currFreq = f;
    g_sync.currBand = b;
    g_sync.currUsb  = u;
    g_sync.currVfo  = v;
    g_sync.currRitEn = re;
    g_sync.currRitOff = ro;
    g_sync.currVoxEn = ve;
    g_sync.currVoxThresh = vt;
    g_sync.currVoxDelay = vd;
    g_sync.currVol = vol;
    g_sync.currPwr = pwr;
    g_sync.currMic = mic;
    g_sync.currModeIdx = mIdx;

    g_sync.currMinFreq = radio.getMinFreq();
    g_sync.currMaxFreq = radio.getMaxFreq();
    g_sync.currStepVal = STEPS[radio.getStepIdx()];
    g_sync.currDigiMode = digital.getMode();
    g_sync.currBusy = digital.isBusy();
    g_sync.currTx = g_tx.load();
    g_sync.currMinFreq = radio.getMinFreq();
    g_sync.currMaxFreq = radio.getMaxFreq();

    // Mirror memory slots safely for tooltips
    uint32_t currentMemRev = radio.getMemRevision();
    if (currentMemRev != g_sync.memRevision.load()) {
        const VfoState* mems = radio.getMemChannels();
        for(int i=0; i<NUM_MEM_CHANNELS; i++) {
            g_sync.memMirror[i].occ = mems[i].occupied;
            g_sync.memMirror[i].freq = mems[i].freq;
            g_sync.memMirror[i].band = mems[i].band;
        }
        g_sync.memRevision.store(currentMemRev);
        changed = true; // Force a web update when memory changes
    }

    if (changed) {
        g_lastActivityTime.store(now);
        notifyWebUpdate();
    }

    // 3. Sensor Update (Metrics)
    static unsigned long lastSensorUpdate = 0;
    if (now - lastSensorUpdate > 100) // 10Hz is enough for SWR/RSSI
    {
        SWRResult m = readSWR();
        g_sync.swr.store(m.swr);
        g_sync.pwrW.store(m.powerW);
        g_sync.rssi.store(m.rssi);
        g_sync.sLevel.store(m.sLevel);
        lastSensorUpdate = now;

        // Signal present (> S1) counts as activity to keep S-Meter fluid
        if (m.sLevel > 1) g_lastActivityTime.store(now);

        notifyWebUpdate(); // Trigger web update for new SWR/RSSI
    }
}

void TaskRadio(void *p)
{
    g_guiNeedsUpdate = true;

    // Initialize SyncState with current hardware values
    g_sync.currFreq = radio.getFrequency();
    g_sync.currBand = radio.getBand();
    g_sync.currUsb  = radio.isUsb();
    g_sync.currVol  = audio.getVolume();
    g_sync.currPwr  = audio.getPaPower();
    g_sync.currMic  = audio.getMicGain();
    g_sync.memRevision.store(0xFFFFFFFF); // Force first sync in handleSyncState
    g_sync.updatePending = false;
    g_sync.updateMask = 0;

    for (;;)
    {
        uint32_t startWork = micros();
        unsigned long now = millis();

        // Single Point MCP Read
        bool mcpPtt = HIGH;
        if (xSemaphoreTakeRecursive(g_hwMutex, portMAX_DELAY))
        {
            mcpPtt = mcp.digitalRead(MCP_PIN_PTT);
            xSemaphoreGiveRecursive(g_hwMutex);
        }

        handlePTT(now, mcpPtt);
        handleSoftKeying(mcpPtt);
        handleEncoder(now);
        handleTouch(now);
        handleSyncState(now);
        handleDisplayUpdates(now);
        settings.process(); // Auto-save logic
        handleSystemStats(now, startWork);

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
