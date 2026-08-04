#include "Display.h"
#include "Hardware.h"
#include "Time.h"
#include "NetworkManager.h"
#include "EncoderActions.h"
#include "RadioEngine.h"
#include "AudioManager.h"
#include "DigitalEngine.h"
#include <WiFi.h>

/**
 * ── LOVYANGFX FONT REFERENCE ──────────────────────────────────────────────
 * Available fonts for use with tft.setFont() or Button::labelFont:
 *
 * 1. Standard Pixel Fonts (Fast, grid-based, scale with setTextSize):
 *    - &fonts::Font0  (or nullptr) : Standard 6x8 GLCD font
 *    - &fonts::Font2  : 5x7 small font
 *    - &fonts::Font4  : 11x18 medium font
 *    - &fonts::Font7  : 7-Segment digital font (Numbers only)
 *    - &fonts::Font8  : Large 32x48 font (Numbers only)
 *
 * 2. GFX FreeFonts (Smooth, proportional, size in pt):
 *    Families: FreeMono, FreeSans, FreeSerif
 *    Styles:   [None] (Regular), Bold, Oblique, BoldOblique
 *    Sizes:    9pt7b, 12pt7b, 18pt7b, 24pt7b
 *
 *    Examples:
 *    - &fonts::FreeMono9pt7b        (Current default for small buttons)
 *    - &fonts::FreeMonoBold18pt7b   (Current default for band buttons)
 *    - &fonts::FreeSans12pt7b       (Used for "MHz" and small labels)
 *    - &fonts::FreeSans24pt7b       (Used for main frequency)
 * ──────────────────────────────────────────────────────────────────────────
 */

// ── SHARED DRAWING TOOLS (AppMode) ──────────────────────────────────────────

void AppMode::_drawFrequency(LGFX_Sprite& canvas, long freq, bool usb, bool showMode)
{
    canvas.fillSprite(TFT_BLACK);

    // 1. Mode Flag (Top Right)
    if (showMode) {
        canvas.setFont(&fonts::FreeSans9pt7b);
        canvas.setTextDatum(top_right);
        canvas.setTextColor(0xB220); // Dimmed Amber
        canvas.drawString(usb ? "USB" : "LSB", 460, 2);
    }

    // 2. 7-Segment Frequency Look
    char mainBuf[16], subBuf[16];
    snprintf(mainBuf, sizeof(mainBuf), "%ld.%03ld", freq / 1000000, (freq % 1000000) / 1000);
    snprintf(subBuf, sizeof(subBuf), ".%03ld", freq % 1000);

    canvas.setFont(&fonts::Font7);
    canvas.setTextSize(1.0);
    int mainW = canvas.textWidth(mainBuf);
    canvas.setTextSize(0.6);
    int subW = canvas.textWidth(subBuf);

    canvas.setFont(&fonts::FreeSans12pt7b);
    canvas.setTextSize(1.0);
    int mhzW = canvas.textWidth(" MHz");

    int totalW = mainW + subW + mhzW;
    int startX = (464 - totalW) / 2;

    canvas.setTextColor(TRX_AMBER);
    canvas.setFont(&fonts::Font7);
    canvas.setTextSize(1.0);
    canvas.setTextDatum(top_left);
    canvas.drawString(mainBuf, startX, 10);

    canvas.setTextSize(0.6);
    canvas.drawString(subBuf, startX + mainW, 10);

    canvas.setFont(&fonts::FreeSans12pt7b);
    canvas.setTextSize(1.0);
    canvas.drawString(" MHz", startX + mainW + subW, 37);
}

void AppMode::_drawFullPageHeader(const char* label, uint16_t color)
{
    // Draw Static Frame
    tft.drawRect(5, 5, 470, 310, color);

    // Draw Title (Top Centered)
    tft.setFont(nullptr);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(middle_center);
    tft.drawString(label, 240, 30);
    tft.drawLine(10, 50, 470, 50, color);

    // Draw standard BACK button
    tft.fillRoundRect(350, 260, 110, 40, 4, TRX_BLUE);
    tft.drawRoundRect(350, 260, 110, 40, 4, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(2);
    tft.setTextDatum(middle_center);
    tft.drawString("BACK", 405, 280);
}

// ── SPECIALIZED MODES IMPLEMENTATION ────────────────────────────────────────

void RitMode::render(bool force)
{
    if (force) {
        tft.fillScreen(TFT_BLACK);
        _drawFullPageHeader("RIT CONTROL", 0x07FF);
        tft.setFont(nullptr);
        tft.setTextSize(2);
        tft.setTextColor(TFT_DARKGREY);
        tft.setTextDatum(middle_center);
        tft.drawString("Adjust receiver offset", 240, 85);
    }

    LGFX_Sprite& canvas = ui.getCanvas();
    canvas.fillSprite(TFT_BLACK);

    long offset = radio.getRitOffset();
    char valStr[16];
    snprintf(valStr, sizeof(valStr), "%ld", labs(offset));
    const char* sign = (offset >= 0) ? "+" : "-";

    canvas.setFont(&fonts::FreeSans18pt7b);
    int w1 = canvas.textWidth("RIT: ");
    int w2 = canvas.textWidth(sign);
    int w4 = canvas.textWidth(" Hz");

    canvas.setFont(&fonts::Font7);
    canvas.setTextSize(0.7);
    int w3 = canvas.textWidth(valStr);

    int totalW = w1 + w2 + w3 + w4;
    int startX = (464 - totalW) / 2;

    canvas.setTextColor(0x07FF); // RIT Blue
    canvas.setTextDatum(top_left);
    canvas.setFont(&fonts::FreeSans18pt7b);
    canvas.drawString("RIT: ", startX, 28);
    canvas.drawString(" Hz", startX + w1 + w2 + w3, 28);

    canvas.setTextDatum(middle_left);
    canvas.drawString(sign, startX + w1, 23);

    canvas.setTextDatum(top_left);
    canvas.setFont(&fonts::Font7);
    canvas.setTextSize(0.7);
    canvas.drawString(valStr, startX + w1 + w2, 10);

    canvas.pushSprite(8, 120); // Move down to avoid header flickering
}

void RitMode::handleTouch(int x, int y, bool longPress)
{
    // Back button hit-box
    if (x > 350 && y > 260) {
        radio.setRitEnabled(false);
        ui.setMode(DisplayMode::Radio);
    }
}

void RitMode::onRotate(int delta)
{
    radio.setRitOffset(radio.getRitOffset() + delta * 10);
}

void ParamMode::_drawBar(LGFX_Sprite& canvas, const char* label, int val, uint16_t color)
{
    canvas.fillSprite(TFT_BLACK);
    canvas.setFont(&fonts::FreeSans18pt7b);
    canvas.setTextColor(color);
    canvas.setTextDatum(middle_center);

    char buf[32];
    snprintf(buf, sizeof(buf), "%d%%", val);
    canvas.drawString(buf, 232, 25);

    int barW = 300, barH = 8;
    int barX = (464 - barW) / 2, barY = 50;
    canvas.drawRect(barX, barY, barW, barH, TFT_WHITE);
    canvas.fillRect(barX + 2, barY + 2, map(val, 0, 100, 0, barW - 4), barH - 4, color);

    canvas.pushSprite(8, 120); // Move down to avoid header flickering
}

void ParamMode::handleTouch(int x, int y, bool longPress)
{
    if (x > 350 && y > 260) {
        ui.setMode(DisplayMode::Radio);
    }
}

void ParamMode::onRotate(int delta)
{
    // Activity reset logic handled in ui.setMode or encoder manager
}

void VolumeMode::render(bool force)
{
    if (force) {
        tft.fillScreen(TFT_BLACK);
        _drawFullPageHeader("AUDIO VOLUME", TRX_BLUE);
        tft.setFont(nullptr);
        tft.setTextSize(2);
        tft.setTextColor(TFT_DARKGREY);
        tft.setTextDatum(middle_center);
        tft.drawString("Adjust output level", 240, 85);
    }
    _drawBar(ui.getCanvas(), "AUDIO VOLUME", audio.getVolume(), TRX_BLUE);
}
void VolumeMode::onButtonShort() { ui.setMode(DisplayMode::Power); }
void VolumeMode::onRotate(int delta) { audio.setVolume(audio.getVolume() + delta); ui.setMode(DisplayMode::Volume); }

void PowerMode::render(bool force)
{
    if (force) {
        tft.fillScreen(TFT_BLACK);
        _drawFullPageHeader("TRANSMIT POWER", TFT_RED);
        tft.setFont(nullptr);
        tft.setTextSize(2);
        tft.setTextColor(TFT_DARKGREY);
        tft.setTextDatum(middle_center);
        tft.drawString("Adjust PA drive level", 240, 85);
    }
    _drawBar(ui.getCanvas(), "TRANSMIT POWER", audio.getPaPower(), TFT_RED);
}
void PowerMode::onButtonShort() { ui.setMode(DisplayMode::Radio); }
void PowerMode::onRotate(int delta) { audio.setPaPower(audio.getPaPower() + delta); ui.setMode(DisplayMode::Power); }

void MicMode::render(bool force)
{
    if (force) {
        tft.fillScreen(TFT_BLACK);
        _drawFullPageHeader("MIC GAIN", TRX_AMBER);
        tft.setFont(nullptr);
        tft.setTextSize(2);
        tft.setTextColor(TFT_DARKGREY);
        tft.setTextDatum(middle_center);
        tft.drawString("Adjust microphone sensitivity", 240, 85);
    }
    _drawBar(ui.getCanvas(), "MIC GAIN", audio.getMicGain(), TRX_AMBER);
}
void MicMode::onButtonShort() { ui.setMode(DisplayMode::Radio); }
void MicMode::onRotate(int delta) { audio.setMicGain(audio.getMicGain() + delta); ui.setMode(DisplayMode::Mic); }

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
            if (s) ui.setMode(DisplayMode::Rit);
            else ui.setMode(DisplayMode::Radio);
        };
        _ctrlBtns[4].onTap = []() { ui.setMode(DisplayMode::Mic); };
        _ctrlBtns[5].onTap = []() { ui.setMode(DisplayMode::Generator); };
        _ctrlBtns[6].onTap = []() { ui.setMode(DisplayMode::Settings); };

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
        _drawFrequency(ui.getCanvas(), radio.getFrequency(), radio.isUsb());
        ui.getCanvas().pushSprite(8, 8);
        _updateButtons(force);
    }

    void onButtonShort() override { ui.setMode(DisplayMode::Volume); }

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
        tft.setFont(nullptr); // Use System Font for consistency with EXIT button
        tft.setTextSize(2);
        tft.setTextDatum(middle_center);
        tft.drawString("! SIGNAL GENERATOR ACTIVE - PA DISABLED !", 240, 150);
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
        _drawFrequency(ui.getCanvas(), radio.getFrequency(), radio.isUsb(), false);
        ui.getCanvas().pushSprite(8, 8);

        // Clean full-width EXIT button
        static bool drawn = false;
        if (force || !drawn)
        {
            tft.setFont(nullptr); // Ensure default font for this mode
            tft.fillRoundRect(10, 220, 460, 60, 8, TFT_DARKGREY);
            tft.drawRoundRect(10, 220, 460, 60, 8, TFT_WHITE);

            tft.setTextColor(TFT_WHITE);
            tft.setTextSize(3);
            tft.setTextDatum(middle_center);
            tft.drawString("EXIT TO RADIO", 240, 250);
            drawn = true;
        }
    }

    void handleTouch(int tx, int ty, bool longPress = false) override
    {
        // Exit Button hit-box
        if (ty > 220 && ty < 280)
        {
            ui.setMode(DisplayMode::Radio);
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
    int _focusParam = 0; // Index relative to active tab
    int _currentTab = 0; // 0=VOX, 1=TIME
    int _rotaryAccum = 0; // "Gearbox" for sensitive parameters

    struct {
        bool voxEn = false;
        int  voxThresh = -1;
        int  voxDelay = -1;
        int  utcOffset = -99;
        bool dstActive = false;
        int  focus = -1;
        int  tab = -1;
    } _localLast;

public:
    const char* getName() override { return "SETTINGS"; }

    void onEnter() override
    {
        tft.fillScreen(TFT_BLACK);
        _focusParam = 0;
        _rotaryAccum = 0;
        _localLast.tab = -1; // Force redraw
        _localLast.focus = -1;
        encManager.setMode(EncoderMode::Tune); // Ensure we are not adjusting Vol/Power
    }

    void render(bool force) override
    {
        bool changed = force ||
                      _localLast.tab != _currentTab ||
                      _localLast.focus != _focusParam ||
                      _localLast.voxEn != radio.isVoxEnabled() ||
                      _localLast.voxThresh != radio.getVoxThreshold() ||
                      _localLast.voxDelay != radio.getVoxDelay() ||
                      _localLast.utcOffset != radio.getUtcOffset() ||
                      _localLast.dstActive != radio.isDstActive();

        if (!changed)
            return;

        tft.setFont(nullptr);

        // Static frame and title
        if (force || _localLast.tab != _currentTab)
        {
            tft.drawRect(5, 5, 470, 310, TRX_AMBER_LOW);

            // Draw Tabs
            _drawTabButton(0, L_TAB_VOX, _currentTab == 0);
            _drawTabButton(1, L_TAB_TIME, _currentTab == 1);

            tft.drawLine(10, 60, 470, 60, 0x4208); // Neutral divider line

            // Exit Button
            tft.fillRoundRect(350, 260, 110, 40, 4, TRX_BLUE);
            tft.drawRoundRect(350, 260, 110, 40, 4, TFT_WHITE);
            tft.setTextColor(TFT_WHITE);
            tft.setTextSize(2);
            tft.setTextDatum(middle_center);
            tft.drawString("BACK", 405, 280);

            // Clear content area
            tft.fillRect(10, 65, 460, 190, TFT_BLACK);
        }

        if (_currentTab == 0) // VOX TAB
        {
            _drawCheckboxRow(0, L_VOX_ACTIVE, radio.isVoxEnabled(), _focusParam == 0);
            _drawParamRow(1, L_VOX_THRESH, String(radio.getVoxThreshold()), _focusParam == 1, !radio.isVoxEnabled());
            _drawParamRow(2, L_VOX_DELAY, String(radio.getVoxDelay()) + " ms", _focusParam == 2, !radio.isVoxEnabled());
        }
        else if (_currentTab == 1) // TIME TAB
        {
            _drawParamRow(0, L_UTC_OFFSET, (radio.getUtcOffset() >= 0 ? "+" : "") + String(radio.getUtcOffset()), _focusParam == 0);
            _drawCheckboxRow(1, L_DST, radio.isDstActive(), _focusParam == 1);
        }

        // Update cache
        _localLast.voxEn = radio.isVoxEnabled();
        _localLast.voxThresh = radio.getVoxThreshold();
        _localLast.voxDelay = radio.getVoxDelay();
        _localLast.utcOffset = radio.getUtcOffset();
        _localLast.dstActive = radio.isDstActive();
        _localLast.focus = _focusParam;
        _localLast.tab = _currentTab;
    }

    void onRotate(int delta) override
    {
        if (_currentTab == 0) // VOX
        {
            if (!radio.isVoxEnabled()) return;
            if (_focusParam == 1) radio.setVoxThreshold(radio.getVoxThreshold() + delta * 10);
            else if (_focusParam == 2) radio.setVoxDelay(radio.getVoxDelay() + delta * 50);
        }
        else if (_currentTab == 1) // TIME
        {
            if (_focusParam == 0) {
                _rotaryAccum += delta;
                // Gear ratio: 4 ticks required for 1 hour change
                if (abs(_rotaryAccum) >= 4) {
                    int change = _rotaryAccum / 4;
                    radio.setUtcOffset(constrain(radio.getUtcOffset() + change, -12, 14));
                    _rotaryAccum %= 4;
                }
            }
        }
        g_guiNeedsUpdate = true;
    }

    void handleTouch(int tx, int ty, bool longPress = false) override
    {
        // Exit
        if (tx > 350 && ty > 260) { ui.setMode(DisplayMode::Radio); return; }

        // Tab Switch (Check actual button boundaries)
        if (ty >= 10 && ty <= 45)
        {
            if (tx >= 10 && tx <= 105 && _currentTab != 0) { _currentTab = 0; _focusParam = 0; _rotaryAccum = 0; }
            else if (tx >= 110 && tx <= 205 && _currentTab != 1) { _currentTab = 1; _focusParam = 0; _rotaryAccum = 0; }
            g_guiNeedsUpdate = true;
            return;
        }

        // Content Interaction
        int row = (ty - 80) / 50;
        if (row >= 0 && row < 3 && ty >= 80)
        {
            if (_currentTab == 0) // VOX
            {
                if (row == 0) { radio.setVoxEnabled(!radio.isVoxEnabled()); _focusParam = 0; }
                else if (radio.isVoxEnabled()) _focusParam = row;
            }
            else if (_currentTab == 1) // TIME
            {
                if (row == 0) { _focusParam = 0; _rotaryAccum = 0; }
                else if (row == 1) { radio.setDstActive(!radio.isDstActive()); _focusParam = 1; }
            }
        }

        g_guiNeedsUpdate = true;
    }

    void onButtonShort() override
    {
        if (_currentTab == 0 && _focusParam == 0) radio.setVoxEnabled(!radio.isVoxEnabled());
        else if (_currentTab == 1 && _focusParam == 1) radio.setDstActive(!radio.isDstActive());
        g_guiNeedsUpdate = true;
    }

private:
    void _drawTabButton(int idx, const char* label, bool active)
    {
        int x = 10 + idx * 100;
        uint16_t color = active ? TRX_BLUE : 0x2104; // Blue for active tab
        uint16_t textColor = TFT_WHITE;

        tft.fillRoundRect(x, 10, 95, 35, 4, color);
        tft.drawRoundRect(x, 10, 95, 35, 4, active ? TFT_WHITE : 0x4208);
        tft.setTextColor(textColor);
        tft.setTextSize(2);
        tft.setTextDatum(middle_center);
        tft.drawString(label, x + 47, 27);
    }

    void _drawCheckboxRow(int row, String label, bool checked, bool focused)
    {
        int y = 85 + row * 50;
        uint16_t color = focused ? TRX_AMBER : TFT_DARKGREY;

        tft.setFont(nullptr);
        tft.setTextSize(2);
        tft.setTextColor(color, TFT_BLACK);
        tft.setTextDatum(middle_left);
        tft.drawString(label, 20, y + 10);

        // Draw Checkbox Container (centered in row)
        int cbX = 220;
        tft.drawRect(cbX, y - 5, 30, 30, TFT_WHITE);
        tft.fillRect(cbX + 2, y - 3, 26, 26, TFT_BLACK);

        if (checked)
        {
            tft.drawLine(cbX + 5, y + 10, cbX + 12, y + 20, TFT_GREEN);
            tft.drawLine(cbX + 6, y + 10, cbX + 13, y + 20, TFT_GREEN);
            tft.drawLine(cbX + 12, y + 20, cbX + 25, y, TFT_GREEN);
            tft.drawLine(cbX + 13, y + 20, cbX + 26, y, TFT_GREEN);
        }

        uint16_t borderColor = focused ? TRX_AMBER : TFT_BLACK;
        tft.drawRect(215, y - 8, 160, 35, borderColor);
    }

    void _drawParamRow(int row, String label, String val, bool focused, bool disabled = false)
    {
        int y = 85 + row * 50;
        uint16_t color = disabled ? 0x2104 : (focused ? TRX_AMBER : TFT_DARKGREY);
        uint16_t valColor = disabled ? 0x2104 : TFT_WHITE;

        tft.setFont(nullptr);
        tft.setTextSize(2);
        tft.setTextColor(color, TFT_BLACK);
        tft.setTextDatum(middle_left);
        tft.drawString(label, 20, y + 10);

        tft.setTextColor(valColor, TFT_BLACK);
        char buf[16];
        snprintf(buf, sizeof(buf), "%-12s", val.c_str());
        tft.drawString(buf, 220, y + 10);

        uint16_t borderColor = (focused && !disabled) ? TRX_AMBER : TFT_BLACK;
        tft.drawRect(215, y - 8, 160, 35, borderColor);
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
    _ritMode   = new RitMode();
    _volMode   = new VolumeMode();
    _pwrMode   = new PowerMode();
    _micMode   = new MicMode();
    _currentMode = _radioMode;
}

void DisplayController::begin()
{
    if (_initialized)
        return;
    _topCanvas.createSprite(464, 70);
    _topCanvas.setColorDepth(16);
    _initialized = true;
    _currentMode->onEnter();
}

void DisplayController::setMode(DisplayMode mode)
{
    AppMode* nextMode = nullptr;
    switch(mode)
    {
        case DisplayMode::Generator: nextMode = _genMode; break;
        case DisplayMode::Settings:  nextMode = _settingsMode; break;
        case DisplayMode::Rit:       nextMode = _ritMode; break;
        case DisplayMode::Volume:    nextMode = _volMode; break;
        case DisplayMode::Power:     nextMode = _pwrMode; break;
        case DisplayMode::Mic:       nextMode = _micMode; break;
        default:                     nextMode = _radioMode; break;
    }

    if (nextMode == _currentMode) {
        // Refresh timeout if already in a param mode
        if (mode == DisplayMode::Volume || mode == DisplayMode::Power || mode == DisplayMode::Mic)
            _modeTimeout = millis();
        return;
    }

    if (_currentMode)
        _currentMode->onLeave();

    _previousMode = _currentMode;
    _currentMode = nextMode;
    _currentMode->onEnter();

    // Synchronize Encoder Manager
    if (mode == DisplayMode::Volume) encManager.setMode(EncoderMode::Volume);
    else if (mode == DisplayMode::Power) encManager.setMode(EncoderMode::Power);
    else if (mode == DisplayMode::Mic) encManager.setMode(EncoderMode::Mic);
    else if (mode == DisplayMode::Rit) encManager.setMode(EncoderMode::Rit);
    else if (mode == DisplayMode::Radio) encManager.setMode(EncoderMode::Tune);

    if (mode == DisplayMode::Volume || mode == DisplayMode::Power || mode == DisplayMode::Mic)
        _modeTimeout = millis();
    else
        _modeTimeout = 0;

    notifyWebUpdate();
    drawFullUI();
}

void DisplayController::drawFullUI()
{
    tft.fillScreen(TFT_BLACK);
    if (_currentMode == _radioMode)
    {
        tft.drawRect(5, 5, 470, 75, TRX_AMBER_LOW);
    }
    _currentMode->render(true);
}

void DisplayController::update(bool force)
{
    checkTimeout();
    _currentMode->render(force);
}

void DisplayController::checkTimeout()
{
    if (_modeTimeout > 0 && millis() - _modeTimeout > 3000)
    {
        setMode(DisplayMode::Radio);
    }
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

      if (timeStatus() == timeSet)
      {
          time_t utc = now();
          time_t local = utc + (radio.getUtcOffset() * 3600);
          if (radio.isDstActive()) local += 3600;

          bool isRawUtc = (radio.getUtcOffset() == 0 && !radio.isDstActive());
          display2.printf("%s %02d:%02d:%02d", isRawUtc ? L_TIME_UTC : L_TIME,
                         hour(local), minute(local), second(local));
      }
      else
          display2.printf("%s %s", L_DIGITAL_MODE, digital.getMode() == 0 ? "Morse" : "RTTY");

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
