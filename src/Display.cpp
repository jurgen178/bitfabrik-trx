#include "Display.h"
#include "Hardware.h"
#include "NetworkManager.h"
#include "EncoderActions.h"
#include "RadioEngine.h"
#include "AudioManager.h"
#include "DigitalEngine.h"
#include <WiFi.h>

// ── BUTTON RENDERER ────────────────────────────────────────────────────────

void Button::draw() const
{
    if (!label)
        return;
    uint16_t fill   = disabled ? 0x10A2
                    : active   ? colorActive
                               : colorInactive;
    uint16_t border = disabled ? 0x3186 : TFT_WHITE;
    uint16_t text   = disabled ? 0x4208 : TFT_WHITE;

    tft.fillRoundRect(x, y, w, h, 4, fill);
    tft.drawRoundRect(x, y, w, h, 4, border);

    tft.setTextColor(text, fill);
    tft.setTextDatum(middle_center);
    tft.setTextSize(1); // Scale 1 for GFX fonts

    if (subtitleBuf[0] != '\0') {
        // Split view: Label top, Subtitle bottom
        tft.setFont(&fonts::FreeMono9pt7b);
        tft.drawString(label, x + w / 2, y + h / 3 + 2);

        tft.setFont(nullptr); // Use compact pixel font for frequency
        tft.setTextSize(1);
        tft.drawString(subtitleBuf, x + w / 2, y + (2 * h) / 3 + 4);
    } else {
        // Single view: Centered label
        tft.setFont(labelFont);
        tft.drawString(label, x + w / 2, y + h / 2 + 2);
    }
}

/**
 * ── RADIO MODE ──────────────────────────────────────────────────────────
 * Standard operating mode for Ham Radio (Bands, VFOs, S-Meter).
 * ──────────────────────────────────────────────────────────────────────────
 */
class RadioMode : public AppMode
{
    Button _bandBtns[NUM_BANDS];
    Button _ctrlBtns[7]; // VFO A, VFO B, A=B, RIT, MIC, GEN, SET
    Button _memBtns[NUM_MEM_CHANNELS];
    uint32_t _lastMemRevision = 0xFFFFFFFF; // Force initial draw
    int    _lastBand = -1;

    void _initButtons()
    {
        // Band buttons: 3x2 grid
        for (int i = 0; i < NUM_BANDS; i++) {
            _bandBtns[i].x    = 10 + (i % 3) * 155;
            _bandBtns[i].y    = 90 + (i / 3) * 55;
            _bandBtns[i].w    = 145;
            _bandBtns[i].h    = 45;
            _bandBtns[i].label        = BANDS[i].enabled ? BANDS[i].label : "---";
            _bandBtns[i].disabled     = !BANDS[i].enabled;
            _bandBtns[i].labelFont    = &fonts::FreeMono18pt7b; // Large bold font for bands
            _bandBtns[i].colorInactive = 0x3186;
            _bandBtns[i].onTap        = [i]() { radio.selectBand(i); notifyWebUpdate(); };
        }
        // Control buttons: single row at y=210
        constexpr int BW = 60, GAP = 10, BY = 210;
        const char* ctrlLabels[] = { "VFO A", "VFO B", "A=B", "RIT", "MIC", "GEN", "SET" };
        for (int i = 0; i < 7; i++) {
            _ctrlBtns[i].x     = i * (BW + GAP);
            _ctrlBtns[i].y     = BY;
            _ctrlBtns[i].w     = BW;
            _ctrlBtns[i].h     = 40;
            _ctrlBtns[i].label = ctrlLabels[i];
        }
        _ctrlBtns[0].onTap = []() { radio.switchVfo(0); };
        _ctrlBtns[1].onTap = []() { radio.switchVfo(1); };
        _ctrlBtns[2].onTap = []() { radio.vfoCopy(); };
        _ctrlBtns[3].onTap = []() {
            bool s = !radio.isRitEnabled();
            radio.setRitEnabled(s);
            encManager.setMode(s ? EncoderMode::Rit : EncoderMode::Tune);
        };
        _ctrlBtns[4].onTap = []() {
            encManager.setMode(
                encManager.getMode() == EncoderMode::Mic ? EncoderMode::Tune : EncoderMode::Mic);
        };
        _ctrlBtns[5].onTap = []() { ui.setMode("GEN"); };
        _ctrlBtns[6].onTap = []() { ui.setMode("SETTINGS"); };

        // Memory buttons: 10 slots in one row beneath control buttons
        // Layout: x=11+i*46, y=272, w=44, h=32; gap=2px
        for (int i = 0; i < NUM_MEM_CHANNELS; i++)
        {
            _memBtns[i].x            = 11 + i * 46;
            _memBtns[i].y            = 272;
            _memBtns[i].w            = 44;
            _memBtns[i].h            = 32;
            _memBtns[i].disabled     = false; // memRecall() handles empty slots internally
            _memBtns[i].colorInactive = 0x18C3;
            _memBtns[i].colorActive   = TRX_AMBER_LOW;
        }
    }

    // Update mem button labels/subtitles/state and redraw all 10 buttons.
    void _refreshMemButtons()
    {
        static const char* MEM_LABELS[] = {
            "M1","M2","M3","M4","M5","M6","M7","M8","M9","M10"
        };
        const VfoState* slots = radio.getMemChannels();
        for (int i = 0; i < NUM_MEM_CHANNELS; i++)
        {
            bool occ = slots[i].occupied;
            _memBtns[i].label    = MEM_LABELS[i];

            if (occ)
            {
                _memBtns[i].setFreqSubtitle(slots[i].freq);
                _memBtns[i].colorInactive = TRX_AMBER_LOW; // Belegt = Bernstein (aktiv leuchtend)
            }
            else
            {
                _memBtns[i].subtitleBuf[0] = '\0';
                _memBtns[i].colorInactive = 0x4208; // Leer = Grau (neutraler Platzhalter)
            }
            _memBtns[i].draw();
        }
    }

    void _updateButtons(bool force)
    {
        UIState& last = ui.getLastState();
        int curBand   = radio.getBand();
        int curVfo    = radio.getActiveVfo();
        bool curRit   = radio.isRitEnabled();
        EncoderMode curMode = encManager.getMode();

        bool bandChanged = force || curBand != last.band;
        bool vfoChanged  = force || curVfo  != last.vfo;
        bool ritChanged  = force || curRit  != last.ritEnabled;
        bool modeChanged = force || curMode != last.mode
                           || curMode == EncoderMode::Volume
                           || curMode == EncoderMode::Power
                           || curMode == EncoderMode::Mic;

        uint32_t curMemRev = radio.getMemRevision();
        bool memChanged = force || (curMemRev != _lastMemRevision);

        if (bandChanged) {
            _lastBand = curBand;
            for (int i = 0; i < NUM_BANDS; i++)
                _bandBtns[i].active = (i == curBand);
            if (force) {
                for (auto& b : _bandBtns)
                    b.draw();
            } else {
                if (last.band >= 0) {
                    _bandBtns[last.band].active = false;
                    _bandBtns[last.band].draw();
                }
                _bandBtns[curBand].active = true;
                _bandBtns[curBand].draw();
            }
            last.band = curBand;
        }
        if (memChanged) {
            _lastMemRevision = curMemRev;
            _refreshMemButtons();
        }
        if (vfoChanged) {
            _ctrlBtns[0].active = (curVfo == 0);
            _ctrlBtns[1].active = (curVfo == 1);
            _ctrlBtns[0].draw();
            _ctrlBtns[1].draw();
            last.vfo = curVfo;
        }
        if (ritChanged) {
            _ctrlBtns[3].active = curRit;
            _ctrlBtns[3].draw();
            last.ritEnabled = curRit;
        }
        if (modeChanged) {
            _ctrlBtns[2].draw();
            _ctrlBtns[4].active = (curMode == EncoderMode::Mic);
            _ctrlBtns[4].draw();
            _ctrlBtns[5].draw();
            _ctrlBtns[6].draw();
            last.mode = curMode;
        }
    }

public:
    const char* getName() override { return "RADIO"; }

    void onEnter() override
    {
        pinMode(PIN_TX_PA_ACTIVE, OUTPUT);
        digitalWrite(PIN_TX_PA_ACTIVE, HIGH);
        g_tx = false;
        _initButtons();
    }

    void render(bool force) override
    {
        ui.renderTopArea(force);
        _updateButtons(force);
    }

    void onButtonShort() override { encManager.cycleMode(); }

    void onButtonLong() override
    {
        radio.setStepIdx((radio.getStepIdx() + 1) % NUM_STEPS);
        g_guiNeedsUpdate = true;
    }

    void onRotate(int delta) override { encManager.handleRotation(delta); }

    void handleTouch(int tx, int ty, bool longPress = false) override
    {
        for (int i = 0; i < NUM_MEM_CHANNELS; i++)
        {
            if (_memBtns[i].hit(tx, ty))
            {
                if (longPress)
                {
                    const VfoState* mems = radio.getMemChannels();
                    // Wenn Speicher belegt UND wir sind auf genau dieser Frequenz/Modus -> Löschen
                    if (mems[i].occupied && mems[i].freq == radio.getFrequency() && mems[i].usb == radio.isUsb())
                    {
                        radio.memDelete(i);
                    }
                    else
                    {
                        radio.memStore(i);
                    }
                }
                else {
                    radio.memRecall(i);
                    ui.getLastState().freq = -1;
                }
                g_guiNeedsUpdate = true;
                notifyWebUpdate();
                return;
            }
        }
        for (auto& b : _bandBtns)
        {
            if (b.hit(tx, ty))
            {
                b.onTap();
                notifyWebUpdate();
                return;
            }
        }
        for (auto& b : _ctrlBtns)
        {
            if (b.hit(tx, ty))
            {
                b.onTap();
                notifyWebUpdate();
                return;
            }
        }
    }
};

/**
 * ── SIGNAL GENERATOR MODE ───────────────────────────────────────────────
 * Pure RF output for testing filters. PA is disabled for safety.
 * ──────────────────────────────────────────────────────────────────────────
 */
class GeneratorMode : public AppMode
{
public:
    const char* getName() override { return "GEN"; }

    void onEnter() override
    {
        // Safety First: Disable PA but enable TX state for VFO output
        pinMode(PIN_TX_PA_ACTIVE, OUTPUT);
        digitalWrite(PIN_TX_PA_ACTIVE, LOW);
        radio.setUnlockedRange(true);
        setTxRx(true);
        encManager.setMode(EncoderMode::Tune); // Force exit from VOL/POWER etc.

        // Visual indicator on screen
        tft.fillScreen(TFT_BLACK);
        tft.drawRect(5, 5, 470, 75, TFT_RED); // Red frame for warning
        tft.setTextColor(TFT_RED);
        tft.setTextSize(2);
        tft.setCursor(15, 305);
        tft.print("! SIGNAL GENERATOR ACTIVE - PA DISABLED !");
    }

    void onLeave() override
    {
        radio.setUnlockedRange(false);
        radio.setFrequency(radio.getFrequency()); // Force snap back to band
        setTxRx(false);
        digitalWrite(PIN_TX_PA_ACTIVE, HIGH);
    }

    void onRotate(int delta) override
    {
        long mult = 1;
        unsigned long dT = g_encInterval;
        if (dT < 6)
            mult = 20;
        else if (dT < 12)
            mult = 10;
        else if (dT < 25)
            mult = 4;

        radio.setFrequency(radio.getFrequency() + delta * STEPS[radio.getStepIdx()] * mult);
    }

    void onButtonLong() override
    {
        radio.setStepIdx((radio.getStepIdx() + 1) % NUM_STEPS);
        g_guiNeedsUpdate = true;
    }

    void render(bool force) override
    {
        ui.renderTopArea(force);

        // Clean full-width EXIT button
        static bool drawn = false;
        if (force || !drawn)
        {
            tft.fillRoundRect(10, 220, 460, 60, 8, TFT_DARKGREY);
            tft.drawRoundRect(10, 220, 460, 60, 8, TFT_WHITE);
            tft.setTextColor(TFT_WHITE);
            tft.setTextSize(3);
            tft.setCursor(160, 235);
            tft.print("EXIT TO RADIO");
            drawn = true;
        }
    }

    void handleTouch(int tx, int ty, bool longPress = false) override
    {
        // Exit Button hit-box
        if (ty > 220 && ty < 280)
        {
            ui.setMode("RADIO");
        }
    }
};

/**
 * ── SETTINGS MODE ────────────────────────────────────────────────────────
 * Menu for system-wide parameters like VOX.
 * ──────────────────────────────────────────────────────────────────────────
 */
class SettingsMode : public AppMode
{
private:
    int _focusParam = 0; // 0=VOX En, 1=VOX Thresh, 2=VOX Delay

    struct {
        bool voxEn = false;
        int  voxThresh = -1;
        int  voxDelay = -1;
        int  focus = -1;
    } _localLast;

public:
    const char* getName() override { return "SETTINGS"; }

    void onEnter() override
    {
        tft.fillScreen(TFT_BLACK);
        _focusParam = 0;
        _localLast.focus = -1; // Force redraw
        encManager.setMode(EncoderMode::Tune); // Ensure we are not adjusting Vol/Power
    }

    void render(bool force) override
    {
        bool changed = force ||
                      _localLast.voxEn != radio.isVoxEnabled() ||
                      _localLast.voxThresh != radio.getVoxThreshold() ||
                      _localLast.voxDelay != radio.getVoxDelay() ||
                      _localLast.focus != _focusParam;

        if (!changed)
            return;

        // Static frame and title
        if (force)
        {
            tft.drawRect(5, 5, 470, 310, TRX_AMBER_LOW);
            tft.setTextColor(TFT_WHITE);
            tft.setTextSize(2);
            tft.setCursor(150, 15);
            tft.print("SYSTEM SETTINGS");

            // Exit Button
            tft.fillRoundRect(350, 260, 110, 40, 4, TRX_BLUE);
            tft.setTextColor(TFT_WHITE);
            tft.setTextSize(2);
            tft.setCursor(375, 272);
            tft.print("BACK");
        }

        // Clear Selection Borders (prevents ghost borders)
        for (int i=0; i<3; i++) {
            if (_localLast.focus == i && _focusParam != i) {
                tft.drawRect(195, 60 + i*50 - 8, 160, 35, TFT_BLACK);
            }
        }

        // VOX Enable
        _drawCheckboxRow(0, "VOX Active", radio.isVoxEnabled(), _focusParam == 0);
        // VOX Threshold
        _drawParamRow(1, "VOX Threshold", String(radio.getVoxThreshold()), _focusParam == 1);
        // VOX Delay
        _drawParamRow(2, "VOX Delay", String(radio.getVoxDelay()) + " ms", _focusParam == 2);

        // Update cache (at the very end to prevent flickering)
        _localLast.voxEn = radio.isVoxEnabled();
        _localLast.voxThresh = radio.getVoxThreshold();
        _localLast.voxDelay = radio.getVoxDelay();
        _localLast.focus = _focusParam;
    }

    void onRotate(int delta) override
    {
        if (_focusParam == 1) // Threshold
        {
            radio.setVoxThreshold(radio.getVoxThreshold() + delta * 10);
        }
        else if (_focusParam == 2) // Delay
        {
            radio.setVoxDelay(radio.getVoxDelay() + delta * 50);
        }
        g_guiNeedsUpdate = true;
    }

    void handleTouch(int tx, int ty, bool longPress = false) override
    {
        if (tx > 350 && ty > 260) { ui.setMode("RADIO"); return; }

        if (ty > 50 && ty < 100)
        {
            _focusParam = 0;
            radio.setVoxEnabled(!radio.isVoxEnabled());
        }
        else if (ty > 100 && ty < 150) _focusParam = 1;
        else if (ty > 150 && ty < 200) _focusParam = 2;

        g_guiNeedsUpdate = true;
    }

    void onButtonShort() override
    {
        if (_focusParam == 0)
        {
            radio.setVoxEnabled(!radio.isVoxEnabled());
            g_guiNeedsUpdate = true;
        }
    }

private:
    void _drawCheckboxRow(int row, String label, bool checked, bool focused)
    {
        int y = 60 + row * 50;
        uint16_t color = focused ? TRX_AMBER : TFT_DARKGREY;

        tft.setTextSize(2);
        tft.setTextColor(color, TFT_BLACK); // bg fills behind chars — no fillRect needed
        tft.setCursor(20, y);
        tft.print(label);

        // Draw Checkbox Container
        int cbX = 200;
        tft.drawRect(cbX, y - 5, 30, 30, TFT_WHITE);
        tft.fillRect(cbX + 2, y - 3, 26, 26, TFT_BLACK);

        if (checked)
        {
            // Draw a proper checkmark using lines
            tft.drawLine(cbX + 5, y + 10, cbX + 12, y + 20, TFT_GREEN);
            tft.drawLine(cbX + 6, y + 10, cbX + 13, y + 20, TFT_GREEN); // Thicker
            tft.drawLine(cbX + 12, y + 20, cbX + 25, y, TFT_GREEN);
            tft.drawLine(cbX + 13, y + 20, cbX + 26, y, TFT_GREEN); // Thicker
        }

        // Selection Border (Always draw to clear or set)
        uint16_t borderColor = focused ? TRX_AMBER : TFT_BLACK;
        tft.drawRect(195, y - 8, 160, 35, borderColor);
    }

    void _drawParamRow(int row, String label, String val, bool focused)
    {
        int y = 60 + row * 50;
        uint16_t color = focused ? TRX_AMBER : TFT_DARKGREY;

        tft.setTextSize(2);
        tft.setTextColor(color, TFT_BLACK); // bg fills behind chars — no fillRect needed
        tft.setCursor(20, y);
        tft.print(label);

        // Pad to fixed width so shorter values overwrite longer old ones
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setCursor(200, y);
        tft.printf("%-12s", val.c_str());

        // Selection Border (Always draw to clear or set)
        uint16_t borderColor = focused ? TRX_AMBER : TFT_BLACK;
        tft.drawRect(195, y - 8, 160, 35, borderColor);
    }
};

/**
 * ── DISPLAY CONTROLLER IMPLEMENTATION ─────────────────────────────────────
 */
DisplayController ui;

DisplayController::DisplayController() : _topCanvas(&tft)
{
    _radioMode = new RadioMode();
    _genMode   = new GeneratorMode();
    _settingsMode = new SettingsMode();
    _currentMode = _radioMode;
}

void DisplayController::begin()
{
    if (_initialized)
        return;
    _topCanvas.createSprite(464, 70);
    _topCanvas.setColorDepth(16);
    _initialized = true;
    _currentMode->onEnter(); // Initialize default mode state (button layout etc.)
}

void DisplayController::setMode(const char* modeName)
{
    if (_currentMode)
        _currentMode->onLeave();

    if (strcmp(modeName, "GEN") == 0)
        _currentMode = _genMode;
    else if (strcmp(modeName, "SETTINGS") == 0)
        _currentMode = _settingsMode;
    else
        _currentMode = _radioMode;

    _currentMode->onEnter();
    notifyWebUpdate(); // Sync web immediately on mode change
    drawFullUI();
}

void DisplayController::drawFullUI()
{
    if (strcmp(_currentMode->getName(), "RADIO") == 0)
    {
        tft.fillScreen(TFT_BLACK);
        tft.drawRect(5, 5, 470, 75, TRX_AMBER_LOW);
    }
    _currentMode->render(true);
}

void DisplayController::update(bool force)
{
    _currentMode->render(force);
}

void DisplayController::renderTopArea(bool force)
{
    EncoderMode mode = encManager.getMode();
    long freq = radio.getFrequency();
    bool ritEn = radio.isRitEnabled();
    long ritOff = radio.getRitOffset();
    int vol = audio.getVolume();
    int pwr = audio.getPaPower();
    int mic = audio.getMicGain();
    double bfo = radio.isUsb() ? radio.getBfoUsb() : radio.getBfoLsb();

    if (!force && _last.mode == mode && _last.freq == freq && _last.ritEnabled == ritEn &&
        _last.ritOffset == ritOff && _last.vol == vol && _last.pwr == pwr && _last.mic == mic && _last.bfo == bfo)
        return;

    _topCanvas.fillSprite(TFT_BLACK);

    // RIT Corner Flag
    if (ritEn && ritOff != 0)
    {
        _topCanvas.setFont(&fonts::FreeSans9pt7b);
        _topCanvas.setTextDatum(top_left);
        _topCanvas.setTextColor(0x07FF);
        _topCanvas.drawString("RIT", 4, 2);
    }

    // Main Values
    _topCanvas.setTextDatum(middle_center);
    char buf[32];
    if (mode == EncoderMode::Volume || mode == EncoderMode::Power || mode == EncoderMode::Mic || mode == EncoderMode::Rit)
    {
        _topCanvas.setFont(&fonts::FreeSans18pt7b);
        uint16_t barColor = TRX_AMBER;
        int val = 0;

        if (mode == EncoderMode::Rit)
        {
            _topCanvas.setTextColor(0x07FF);
            snprintf(buf, sizeof(buf), "RIT: %+ld Hz", ritOff);
        }
        else if (mode == EncoderMode::Volume)
        {
            _topCanvas.setTextColor(TRX_BLUE);
            snprintf(buf, sizeof(buf), "VOL: %d%%", vol);
            barColor = TRX_BLUE;
            val = vol;
        }
        else if (mode == EncoderMode::Power)
        {
            _topCanvas.setTextColor(TFT_RED);
            snprintf(buf, sizeof(buf), "PWR: %d%%", pwr);
            barColor = TFT_RED;
            val = pwr;
        }
        else
        {
            _topCanvas.setTextColor(TRX_AMBER);
            snprintf(buf, sizeof(buf), "MIC: %d%%", mic);
            val = mic;
        }

        _topCanvas.drawString(buf, 232, 25);

        // Draw Progress Bar on TFT
        if (mode != EncoderMode::Rit)
        {
            int barW = 300;
            int barH = 8;
            int barX = (464 - barW) / 2;
            int barY = 50;
            _topCanvas.drawRect(barX, barY, barW, barH, TFT_WHITE);
            _topCanvas.fillRect(barX + 2, barY + 2, map(val, 0, 100, 0, barW - 4), barH - 4, barColor);
        }
    }
    else if (mode == EncoderMode::Calibrate)
    {
        _topCanvas.setTextColor(TRX_AMBER);
        _topCanvas.setFont(&fonts::FreeSans12pt7b);
        _topCanvas.drawString("BFO CALIBRATION", 232, 18);
        _topCanvas.setFont(&fonts::FreeSans18pt7b);
        snprintf(buf, sizeof(buf), "%.4f MHz", bfo / 1000000.0);
        _topCanvas.drawString(buf, 232, 45);
    }
    else
    {
        // Elegant Frequency
        _topCanvas.setTextColor(TRX_AMBER);
        char mainBuf[16], subBuf[16];
        snprintf(mainBuf, sizeof(mainBuf), "%ld.%03ld", freq / 1000000, (freq % 1000000) / 1000);
        snprintf(subBuf, sizeof(subBuf), ".%03ld", freq % 1000);

        _topCanvas.setFont(&fonts::FreeSans24pt7b);
        int mainW = _topCanvas.textWidth(mainBuf);
        _topCanvas.setFont(&fonts::FreeSans12pt7b);
        int subW = _topCanvas.textWidth(subBuf);
        int totalW = mainW + subW + _topCanvas.textWidth(" MHz");
        int startX = (464 - totalW) / 2;

        // Main part (#ff6600)
        _topCanvas.setTextColor(TRX_AMBER);
        _topCanvas.setFont(&fonts::FreeSans24pt7b);
        _topCanvas.setTextDatum(top_left);
        _topCanvas.drawString(mainBuf, startX, 17);

        // Decimal and suffix
        _topCanvas.setTextColor(TRX_AMBER);
        _topCanvas.setFont(&fonts::FreeSans12pt7b);
        _topCanvas.drawString(subBuf, startX + mainW, 20);
        _topCanvas.drawString(" MHz", startX + mainW + subW, 33);
    }

    _topCanvas.pushSprite(8, 8);
    _last.mode = mode; _last.freq = freq; _last.ritEnabled = ritEn; _last.ritOffset = ritOff;
    _last.vol = vol; _last.pwr = pwr; _last.mic = mic; _last.bfo = bfo;
}

/**
 * ── HELPER WRAPPER ────────────────────────────────────────────────────────
 */
void updateOled1()
{
  if (xSemaphoreTakeRecursive(g_hwMutex, pdMS_TO_TICKS(50)))
  {
      display1.clearDisplay();
      display1.setTextSize(1);
      display1.setTextColor(SSD1306_WHITE);
      display1.setCursor(0, 0);

      EncoderHandler* handler = encManager.getHandler();
      display1.print(handler->getDisplayLabel());
      handler->renderFocused(display1);

      if (encManager.getMode() == EncoderMode::Volume || encManager.getMode() == EncoderMode::Power || encManager.getMode() == EncoderMode::Mic)
      {
        int val = (encManager.getMode() == EncoderMode::Volume) ? audio.getVolume() :
                 (encManager.getMode() == EncoderMode::Power ? audio.getPaPower() : audio.getMicGain());
        display1.drawRect(0, 36, 120, 6, SSD1306_WHITE);
        display1.fillRect(2, 38, map(val, 0, 100, 0, 116), 2, SSD1306_WHITE);
      }

      display1.setTextSize(1);
      display1.setCursor(0, 44);
      if (encManager.getMode() == EncoderMode::Calibrate)
          display1.print("[ KALIBRIERUNG ]");
      else if (g_tx || digital.isBusy())
          display1.print("[ TX: AKTIV  ]");
      else
      {
        char tickerBuf[16] = {0};
        if (xSemaphoreTake(g_mutex, pdMS_TO_TICKS(5)))
        {
          String sub = digital.getLastRxText(12);
          strncpy(tickerBuf, sub.c_str(), 15);
          xSemaphoreGive(g_mutex);
        }
        display1.printf("[ RX: %s ]", tickerBuf);
      }

      SWRResult m = readSWR();
      int y = 56;
      display1.setCursor(0, y);

      if (g_tx || digital.isBusy())
      {
          display1.printf("SWR:%.1f %2.1fW", m.swr, m.powerW);
          int barWidth = map(constrain((int)((m.vFwd / SWR_V_REF) * SWR_ADC_MAX), 0, 4095), 0, 4095, 0, 45);
          display1.drawRect(82, y, 45, 8, SSD1306_WHITE);
          display1.fillRect(82, y, barWidth, 8, SSD1306_WHITE);
      }
      else
      {
          display1.printf("S-LEVEL: %d", m.sLevel);
          int barWidth = map(constrain((int)((m.rssi / 1.5f) * 100), 0, 100), 0, 100, 0, 45);
          display1.drawRect(82, y, 45, 8, SSD1306_WHITE);
          display1.fillRect(82, y, barWidth, 8, SSD1306_WHITE);
      }

      display1.display();
      xSemaphoreGiveRecursive(g_hwMutex);
  }
}

void updateOled2()
{
  if (xSemaphoreTakeRecursive(g_hwMutex, pdMS_TO_TICKS(50)))
  {
      display2.clearDisplay();
      display2.setTextSize(1);
      display2.setTextColor(SSD1306_WHITE);
      long upS = millis() / 1000;
      int d = upS / 86400, h = (upS % 86400) / 3600, m = (upS % 3600) / 60, s = upS % 60;
      display2.setCursor(0, 0);
      display2.print("--- BITFABRIK TRX ---");
      display2.setCursor(0, 12);
      display2.printf("Digital: %s", digital.getMode() == 0 ? "Morse" : "RTTY");
      display2.setCursor(0, 23);
      display2.printf("IP: %s", network.getActiveIP().c_str());
      display2.setCursor(0, 34);

      if (WiFi.status() == WL_CONNECTED)
          display2.printf("WiFi:%ddBm T:%.1fC", WiFi.RSSI(), temperatureRead());
      else
          display2.printf("WiFi:AP T:%.1fC", temperatureRead());

      display2.setCursor(0, 45);
      if (d > 0)
          display2.printf("Uptime: %dd %02d:%02d:%02d", d, h, m, s);
      else
          display2.printf("Uptime: %02d:%02d:%02d", h, m, s);

      display2.setCursor(0, 56);
      display2.printf("Frei: %d KB", ESP.getFreeHeap() / 1024);
      display2.display();
      xSemaphoreGiveRecursive(g_hwMutex);
  }
}
