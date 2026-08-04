#include "Hardware.h"
#include "Display.h"
#include "EncoderActions.h"
#include "RadioEngine.h"
#include "AudioManager.h"
#include "SettingsManager.h"
#include "NetworkManager.h"
#include "DigitalEngine.h"
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
  if (xSemaphoreTakeRecursive(g_hwMutex, pdMS_TO_TICKS(50)))
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
  if (xSemaphoreTakeRecursive(g_hwMutex, pdMS_TO_TICKS(50)))
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

  if (xSemaphoreTakeRecursive(g_hwMutex, pdMS_TO_TICKS(50)))
  {
    if (tx)
    {
      mcp.digitalWrite(MCP_RELAY_TXRX, HIGH);
      delay(15);
      mcp.digitalWrite(MCP_PIN_PA_BIAS, HIGH);
    }
    else
    {
      mcp.digitalWrite(MCP_PIN_PA_BIAS, LOW);
      delay(10);
      mcp.digitalWrite(MCP_RELAY_TXRX, LOW);
    }
    xSemaphoreGiveRecursive(g_hwMutex);
  }

    // Re-latch band filter relays: relay coil inrush can cause I2C noise that
    // corrupts the MCP shadow register when pins 6/7 and 0-5 share GPIOA.
    radio.refreshRelays();

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

  for (int i=0; i<SWR_SAMPLES; i++)
  {
      fSum += analogRead(PIN_SWR_FWD);
      rSum += analogRead(PIN_SWR_REF);
      rssiSum += analogRead(PIN_RSSI);
  }

  float vF = ((float)fSum/SWR_SAMPLES)*(SWR_V_REF/SWR_ADC_MAX);
  float vR = ((float)rSum/SWR_SAMPLES)*(SWR_V_REF/SWR_ADC_MAX);
  float vRSSI = ((float)rssiSum/SWR_SAMPLES)*(SWR_V_REF/SWR_ADC_MAX);

  SWRResult res;
  res.vFwd = compensateDiode(vF);
  res.vRef = compensateDiode(vR);
  res.rssi = vRSSI;

  // RX Mode: Calculate S-Level
  if (!g_tx && !digital.isBusy())
  {
  if (vRSSI < 0.05f) {
#ifdef DEBUG_SIM
      res.rssi = 0.05f + (float)(random(0, 100)) / 1000.0f; // simulated noise floor
#else
      res.rssi = 0.0f;
#endif
  }

      // Typical S-Unit mapping (example: S9 = 50uV at ant, but here it's DC level from RX)
      // We map 0..1.0V to S1..S9 for now.
      res.sLevel = (int)(res.rssi * 9.0f);
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
        res.swr = (diff > 0.001f) ? (res.vFwd + res.vRef)/diff : 9.9f;
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

static void handlePTT(unsigned long now, bool mcpPtt)
{
    static int pttC = 0;
    static unsigned long lastKeyTime = 0;
    static unsigned long lastVoxTrigger = 0;

    bool rawP = (mcpPtt == LOW);
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
        }
        // Safety: ensure triggered only if threshold is not zero
        if (radio.getVoxThreshold() > 50 && (now - lastVoxTrigger < (unsigned long)radio.getVoxDelay()))
        {
            voxTriggered = true;
        }
    }

    if (digital.isKeyed()) lastKeyTime = now;
    // isBusy keeps TX on during inter-char/word gaps (which exceed CW_HANG_TIME)
    bool shouldBeInTx = (pttC >= 4) || digital.isKeyed() || digital.isBusy() || voxTriggered || (now - lastKeyTime < CW_HANG_TIME);

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
    bool isTouching = tft.getTouch(&tx, &ty);

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
        if (g_guiNeedsUpdate)
        {
            ui.update();
            updateOled1();
            notifyWebUpdate(); // Hardware state changed — keep web in sync
            g_guiNeedsUpdate = false;
            lastTftRefresh = now;
        }
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
void TaskRadio(void *p)
{
    g_guiNeedsUpdate = true;

    for (;;)
    {
        uint32_t startWork = micros();
        unsigned long now = millis();

        // Single Point MCP Read
        bool mcpPtt = HIGH;
        if (xSemaphoreTakeRecursive(g_hwMutex, pdMS_TO_TICKS(5)))
        {
            mcpPtt = mcp.digitalRead(MCP_PIN_PTT);
            xSemaphoreGiveRecursive(g_hwMutex);
        }

        handlePTT(now, mcpPtt);
        handleSoftKeying(mcpPtt);
        handleEncoder(now);
        handleTouch(now);
        handleDisplayUpdates(now);
        settings.process(); // Auto-save logic
        handleSystemStats(now, startWork);

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
