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

void AppMode::drawFrequency(LGFX_Sprite& canvas, long freq, bool usb, bool showMode)
{
    canvas.fillSprite(TFT_BLACK);
    // ...

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

void AppMode::drawFullPageHeader(const char* label, uint16_t color, bool showBackButton)
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

    if (showBackButton) {
        // Draw standard BACK button
        tft.fillRoundRect(350, 260, 110, 40, 4, TRX_BLUE);
        tft.drawRoundRect(350, 260, 110, 40, 4, TFT_WHITE);
        tft.setTextColor(TFT_WHITE);
        tft.setTextSize(2);
        tft.setTextDatum(middle_center);
        tft.drawString(L_BACK, 405, 280);
    }
}

// ── SPECIALIZED MODES IMPLEMENTATION ────────────────────────────────────────

void RitMode::render(bool force)
{
    if (force) {
        tft.fillScreen(TFT_BLACK);
        drawFullPageHeader("RIT CONTROL", 0x07FF);
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

void ParamMode::drawBar(LGFX_Sprite& canvas, const char* label, int val, uint16_t color)
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
        drawFullPageHeader("AUDIO VOLUME", TRX_BLUE);
        tft.setFont(nullptr);
        tft.setTextSize(2);
        tft.setTextColor(TFT_DARKGREY);
        tft.setTextDatum(middle_center);
        tft.drawString("Adjust output level", 240, 85);
    }
    drawBar(ui.getCanvas(), "AUDIO VOLUME", audio.getVolume(), TRX_BLUE);
}
void VolumeMode::onButtonShort() { ui.setMode(DisplayMode::Power); }
void VolumeMode::onRotate(int delta) { audio.setVolume(audio.getVolume() + delta); ui.setMode(DisplayMode::Volume); }

void PowerMode::render(bool force)
{
    if (force) {
        tft.fillScreen(TFT_BLACK);
        drawFullPageHeader("TRANSMIT POWER", TFT_RED);
        tft.setFont(nullptr);
        tft.setTextSize(2);
        tft.setTextColor(TFT_DARKGREY);
        tft.setTextDatum(middle_center);
        tft.drawString("Adjust PA drive level", 240, 85);
    }
    drawBar(ui.getCanvas(), "TRANSMIT POWER", audio.getPaPower(), TFT_RED);
}
void PowerMode::onButtonShort() { ui.setMode(DisplayMode::Radio); }
void PowerMode::onRotate(int delta) { audio.setPaPower(audio.getPaPower() + delta); ui.setMode(DisplayMode::Power); }

void MicMode::render(bool force)
{
    if (force) {
        tft.fillScreen(TFT_BLACK);
        drawFullPageHeader("MIC GAIN", TRX_AMBER);
        tft.setFont(nullptr);
        tft.setTextSize(2);
        tft.setTextColor(TFT_DARKGREY);
        tft.setTextDatum(middle_center);
        tft.drawString("Adjust microphone sensitivity", 240, 85);
    }
    drawBar(ui.getCanvas(), "MIC GAIN", audio.getMicGain(), TRX_AMBER);
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
    Button bandBtns[NUM_BANDS];
    Button ctrlBtns[7]; // VFO A, VFO B, A=B, RIT, MIC, GEN, SET
    Button memBtns[NUM_MEM_CHANNELS];
    uint32_t lastMemRevision = 0xFFFFFFFF; // Force initial draw
    int    lastBand = -1;

    void initButtons()
    {
        // Band buttons: 3x2 grid
        for (int i = 0; i < NUM_BANDS; i++) {
            bandBtns[i].x    = 10 + (i % 3) * 155;
            bandBtns[i].y    = 90 + (i / 3) * 55;
            bandBtns[i].w    = 145;
            bandBtns[i].h    = 45;
            bandBtns[i].label        = BANDS[i].enabled ? BANDS[i].name : "---";
            bandBtns[i].disabled     = !BANDS[i].enabled;
            bandBtns[i].labelFont    = &fonts::FreeMono18pt7b; // Large bold font for bands
            bandBtns[i].colorInactive = 0x3186;
            bandBtns[i].onTap        = [i]() { radio.selectBand(i); notifyWebUpdate(); };
        }
        // Control buttons: single row at y=210
        constexpr int BW = 60, GAP = 10, BY = 210;
        const char* ctrlLabels[] = { "VFO A", "VFO B", "A=B", "RIT", "MIC", "GEN", "SET" };
        for (int i = 0; i < 7; i++) {
            ctrlBtns[i].x     = i * (BW + GAP);
            ctrlBtns[i].y     = BY;
            ctrlBtns[i].w     = BW;
            ctrlBtns[i].h     = 40;
            ctrlBtns[i].label = ctrlLabels[i];
        }
        ctrlBtns[0].onTap = []() { radio.switchVfo(0); };
        ctrlBtns[1].onTap = []() { radio.switchVfo(1); };
        ctrlBtns[2].onTap = []() { radio.vfoCopy(); };
        ctrlBtns[3].onTap = []() {
            bool s = !radio.isRitEnabled();
            radio.setRitEnabled(s);
            if (s) ui.setMode(DisplayMode::Rit);
            else ui.setMode(DisplayMode::Radio);
        };
        ctrlBtns[4].onTap = []() { ui.setMode(DisplayMode::Mic); };
        ctrlBtns[5].onTap = []() { ui.setMode(DisplayMode::Generator); };
        ctrlBtns[6].onTap = []() { ui.setMode(DisplayMode::Settings); };

        // Memory buttons: 10 slots in one row beneath control buttons
        // Layout: x=11+i*46, y=272, w=44, h=32; gap=2px
        for (int i = 0; i < NUM_MEM_CHANNELS; i++)
        {
            memBtns[i].x            = 11 + i * 46;
            memBtns[i].y            = 272;
            memBtns[i].w            = 44;
            memBtns[i].h            = 32;
            memBtns[i].disabled     = false; // memRecall() handles empty slots internally
            memBtns[i].colorInactive = 0x18C3;
            memBtns[i].colorActive   = TRX_AMBER_LOW;
        }
    }

    // Update mem button labels/subtitles/state and redraw all 10 buttons.
    void refreshMemButtons()
    {
        static const char* MEM_LABELS[] = {
            "M1","M2","M3","M4","M5","M6","M7","M8","M9","M10"
        };
        const VfoState* slots = radio.getMemChannels();
        for (int i = 0; i < NUM_MEM_CHANNELS; i++)
        {
            bool occ = slots[i].occupied;
            memBtns[i].label    = MEM_LABELS[i];

            if (occ)
            {
                memBtns[i].setFreqSubtitle(slots[i].freq);
                memBtns[i].colorInactive = TRX_AMBER_LOW; // Belegt = Bernstein (aktiv leuchtend)
            }
            else
            {
                memBtns[i].subtitleBuf[0] = '\0';
                memBtns[i].colorInactive = 0x4208; // Leer = Grau (neutraler Platzhalter)
            }
            memBtns[i].draw();
        }
    }

    void updateButtons(bool force)
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
        bool memChanged = force || (curMemRev != lastMemRevision);

        if (bandChanged) {
            lastBand = curBand;
            for (int i = 0; i < NUM_BANDS; i++)
                bandBtns[i].active = (i == curBand);
            if (force) {
                for (auto& b : bandBtns)
                    b.draw();
            } else {
                if (last.band >= 0) {
                    bandBtns[last.band].active = false;
                    bandBtns[last.band].draw();
                }
                bandBtns[curBand].active = true;
                bandBtns[curBand].draw();
            }
            last.band = curBand;
        }
        if (memChanged) {
            lastMemRevision = curMemRev;
            refreshMemButtons();
        }
        if (vfoChanged) {
            ctrlBtns[0].active = (curVfo == 0);
            ctrlBtns[1].active = (curVfo == 1);
            ctrlBtns[0].draw();
            ctrlBtns[1].draw();
            last.vfo = curVfo;
        }
        if (ritChanged) {
            ctrlBtns[3].active = curRit;
            ctrlBtns[3].draw();
            last.ritEnabled = curRit;
        }
        if (modeChanged) {
            ctrlBtns[2].draw();
            ctrlBtns[4].active = (curMode == EncoderMode::Mic);
            ctrlBtns[4].draw();
            ctrlBtns[5].draw();
            ctrlBtns[6].draw();
            last.mode = curMode;
        }
    }

public:
    const char* getName() override { return "RADIO"; }

    void onEnter() override
    {
        // PA state is managed by the Hardware Sequencer (setTxRx)
        g_tx = false;
        initButtons();
    }

    void render(bool force) override
    {
        drawFrequency(ui.getCanvas(), radio.getFrequency(), radio.isUsb());
        ui.getCanvas().pushSprite(8, 8);
        updateButtons(force);
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
            if (memBtns[i].hit(tx, ty))
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
        for (auto& b : bandBtns)
        {
            if (b.hit(tx, ty))
            {
                b.onTap();
                notifyWebUpdate();
                return;
            }
        }
        for (auto& b : ctrlBtns)
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
        // Safety First: Ensure PA remains disabled while in Generator Mode
        if (g_mcpOk)
        {
            mcp.digitalWrite(MCP_TX_PA_ACTIVE, LOW);
        }
        radio.setUnlockedRange(true);
        // Note: We might want VFO output but NO PA activation.
        // For now, we set g_tx directly to bypass the sequencer's PA activation if needed,
        // or ensure setTxRx(true) is handled safely.
        g_tx = true;
        radio.refreshRelays();

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
        if (force) {
            tft.fillScreen(TFT_BLACK);
            drawFullPageHeader(L_SIGNAL_GEN, TFT_RED, false);

            // Warning text above the large button
            tft.setFont(nullptr);
            tft.setTextSize(2);
            tft.setTextColor(TFT_RED);
            tft.setTextDatum(middle_center);
            tft.drawString(L_PA_DISABLED, 240, 180);

            // Large Exit Button
            tft.fillRoundRect(10, 220, 460, 60, 8, TFT_DARKGREY);
            tft.drawRoundRect(10, 220, 460, 60, 8, TFT_WHITE);
            tft.setTextColor(TFT_WHITE);
            tft.setTextSize(3);
            tft.setTextDatum(middle_center);
            tft.drawString(L_BACK, 240, 250);
        }

        drawFrequency(ui.getCanvas(), radio.getFrequency(), radio.isUsb(), false);
        ui.getCanvas().pushSprite(8, 100);
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
    int focusParam = 0; // Index relative to active tab
    int currentTab = 0; // 0=VOX, 1=TIME
    int rotaryAccum = 0; // "Gearbox" for sensitive parameters

    struct {
        bool voxEn = false;
        int  voxThresh = -1;
        int  voxDelay = -1;
        int  utcOffset = -99;
        bool dstActive = false;
        int  focus = -1;
        int  tab = -1;
    } localLast;

public:
    const char* getName() override { return "SETTINGS"; }

    void onEnter() override
    {
        tft.fillScreen(TFT_BLACK);
        focusParam = 0;
        rotaryAccum = 0;
        localLast.tab = -1; // Force redraw
        localLast.focus = -1;
        encManager.setMode(EncoderMode::Tune); // Ensure we are not adjusting Vol/Power
    }

    void render(bool force) override
    {
        bool changed = force ||
                      localLast.tab != currentTab ||
                      localLast.focus != focusParam ||
                      localLast.voxEn != radio.isVoxEnabled() ||
                      localLast.voxThresh != radio.getVoxThreshold() ||
                      localLast.voxDelay != radio.getVoxDelay() ||
                      localLast.utcOffset != radio.getUtcOffset() ||
                      localLast.dstActive != radio.isDstActive();

        if (!changed)
            return;

        tft.setFont(nullptr);

        // Static frame and title
        if (force || localLast.tab != currentTab)
        {
            tft.drawRect(5, 5, 470, 310, TRX_AMBER_LOW);

            // Draw Tabs
            drawTabButton(0, L_TAB_VOX, currentTab == 0);
            drawTabButton(1, L_TAB_TIME, currentTab == 1);

            tft.drawLine(10, 60, 470, 60, 0x4208); // Neutral divider line

            // Exit Button
            tft.fillRoundRect(350, 260, 110, 40, 4, TRX_BLUE);
            tft.drawRoundRect(350, 260, 110, 40, 4, TFT_WHITE);
            tft.setTextColor(TFT_WHITE);
            tft.setTextSize(2);
            tft.setTextDatum(middle_center);
            tft.drawString(L_BACK, 405, 280);

            // Clear content area
            tft.fillRect(10, 65, 460, 190, TFT_BLACK);
        }

        if (currentTab == 0) // VOX TAB
        {
            drawCheckboxRow(0, L_VOX_ACTIVE, radio.isVoxEnabled(), focusParam == 0);
            drawParamRow(1, L_VOX_THRESH, String(radio.getVoxThreshold()), focusParam == 1, !radio.isVoxEnabled());
            drawParamRow(2, L_VOX_DELAY, String(radio.getVoxDelay()) + " ms", focusParam == 2, !radio.isVoxEnabled());
        }
        else if (currentTab == 1) // TIME TAB
        {
            drawParamRow(0, L_UTC_OFFSET, (radio.getUtcOffset() >= 0 ? "+" : "") + String(radio.getUtcOffset()), focusParam == 0);
            drawCheckboxRow(1, L_DST, radio.isDstActive(), focusParam == 1);
        }

        // Update cache
        localLast.voxEn = radio.isVoxEnabled();
        localLast.voxThresh = radio.getVoxThreshold();
        localLast.voxDelay = radio.getVoxDelay();
        localLast.utcOffset = radio.getUtcOffset();
        localLast.dstActive = radio.isDstActive();
        localLast.focus = focusParam;
        localLast.tab = currentTab;
    }

    void onRotate(int delta) override
    {
        if (currentTab == 0) // VOX
        {
            if (!radio.isVoxEnabled()) return;
            if (focusParam == 1) radio.setVoxThreshold(radio.getVoxThreshold() + delta * 10);
            else if (focusParam == 2) radio.setVoxDelay(radio.getVoxDelay() + delta * 50);
        }
        else if (currentTab == 1) // TIME
        {
            if (focusParam == 0) {
                rotaryAccum += delta;
                // Gear ratio: 4 ticks required for 1 hour change
                if (abs(rotaryAccum) >= 4) {
                    int change = rotaryAccum / 4;
                    radio.setUtcOffset(constrain(radio.getUtcOffset() + change, -12, 14));
                    rotaryAccum %= 4;
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
            if (tx >= 10 && tx <= 105 && currentTab != 0) { currentTab = 0; focusParam = 0; rotaryAccum = 0; }
            else if (tx >= 110 && tx <= 205 && currentTab != 1) { currentTab = 1; focusParam = 0; rotaryAccum = 0; }
            g_guiNeedsUpdate = true;
            return;
        }

        // Content Interaction
        int row = (ty - 80) / 50;
        if (row >= 0 && row < 3 && ty >= 80)
        {
            if (currentTab == 0) // VOX
            {
                if (row == 0) { radio.setVoxEnabled(!radio.isVoxEnabled()); focusParam = 0; }
                else if (radio.isVoxEnabled()) focusParam = row;
            }
            else if (currentTab == 1) // TIME
            {
                if (row == 0) { focusParam = 0; rotaryAccum = 0; }
                else if (row == 1) { radio.setDstActive(!radio.isDstActive()); focusParam = 1; }
            }
        }

        g_guiNeedsUpdate = true;
    }

    void onButtonShort() override
    {
        if (currentTab == 0 && focusParam == 0) radio.setVoxEnabled(!radio.isVoxEnabled());
        else if (currentTab == 1 && focusParam == 1) radio.setDstActive(!radio.isDstActive());
        g_guiNeedsUpdate = true;
    }

private:
    void drawTabButton(int idx, const char* label, bool active)
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

    void drawCheckboxRow(int row, String label, bool checked, bool focused)
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

    void drawParamRow(int row, String label, String val, bool focused, bool disabled = false)
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

DisplayController::DisplayController() : topCanvas(&tft)
{
    radioMode = new RadioMode();
    genMode   = new GeneratorMode();
    settingsMode = new SettingsMode();
    ritMode   = new RitMode();
    volMode   = new VolumeMode();
    pwrMode   = new PowerMode();
    micMode   = new MicMode();
    currentMode = radioMode;
}

void DisplayController::begin()
{
    if (initialized)
        return;
    topCanvas.createSprite(464, 70);
    topCanvas.setColorDepth(16);
    initialized = true;
    currentMode->onEnter();
}

void DisplayController::setMode(DisplayMode mode)
{
    if (xSemaphoreTakeRecursive(g_hwMutex, portMAX_DELAY))
    {
        AppMode* nextMode = nullptr;
        switch(mode)
        {
            case DisplayMode::Generator: nextMode = genMode; break;
            case DisplayMode::Settings:  nextMode = settingsMode; break;
            case DisplayMode::Rit:       nextMode = ritMode; break;
            case DisplayMode::Volume:    nextMode = volMode; break;
            case DisplayMode::Power:     nextMode = pwrMode; break;
            case DisplayMode::Mic:       nextMode = micMode; break;
            default:                     nextMode = radioMode; break;
        }

        if (nextMode == currentMode) {
            // Refresh timeout if already in a param mode
            if (mode == DisplayMode::Volume || mode == DisplayMode::Power || mode == DisplayMode::Mic)
                modeTimeout = millis();
            xSemaphoreGiveRecursive(g_hwMutex);
            return;
        }

        if (currentMode)
            currentMode->onLeave();

        previousMode = currentMode;
        currentMode = nextMode;
        currentMode->onEnter();

        // Synchronize Encoder Manager
        if (mode == DisplayMode::Volume) encManager.setMode(EncoderMode::Volume);
        else if (mode == DisplayMode::Power) encManager.setMode(EncoderMode::Power);
        else if (mode == DisplayMode::Mic) encManager.setMode(EncoderMode::Mic);
        else if (mode == DisplayMode::Rit) encManager.setMode(EncoderMode::Rit);
        else if (mode == DisplayMode::Radio) encManager.setMode(EncoderMode::Tune);

        if (mode == DisplayMode::Volume || mode == DisplayMode::Power || mode == DisplayMode::Mic)
            modeTimeout = millis();
        else
            modeTimeout = 0;

        notifyWebUpdate();
        drawFullUI();
        xSemaphoreGiveRecursive(g_hwMutex);
    }
}

void DisplayController::drawFullUI()
{
    if (xSemaphoreTakeRecursive(g_hwMutex, portMAX_DELAY))
    {
        tft.fillScreen(TFT_BLACK);
        if (currentMode == radioMode)
        {
            tft.drawRect(5, 5, 470, 75, TRX_AMBER_LOW);
        }
        currentMode->render(true);
        xSemaphoreGiveRecursive(g_hwMutex);
    }
}

void DisplayController::update(bool force)
{
    if (xSemaphoreTakeRecursive(g_hwMutex, portMAX_DELAY))
    {
        checkTimeout();
        currentMode->render(force);
        xSemaphoreGiveRecursive(g_hwMutex);
    }
}

void DisplayController::checkTimeout()
{
    if (modeTimeout > 0 && millis() - modeTimeout > 3000)
    {
        setMode(DisplayMode::Radio);
    }
}


/**
 * ── HELPER WRAPPER ────────────────────────────────────────────────────────
 */
void updateOled1()
{
  if (xSemaphoreTakeRecursive(g_hwMutex, portMAX_DELAY))
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
  if (xSemaphoreTakeRecursive(g_hwMutex, portMAX_DELAY))
  {
      display2.clearDisplay();
      display2.setTextSize(1);
      display2.setTextColor(SSD1306_WHITE);
      long upS = millis() / 1000;
      int d = upS / 86400, h = (upS % 86400) / 3600, m = (upS % 3600) / 60, s = upS % 60;
      display2.setCursor(0, 0);
      display2.print("--- BITFABRIK TRX ---");
      display2.setCursor(0, 12);

      struct tm timeinfo;
      if (getLocalTime(&timeinfo))
      {
          bool isRawUtc = (radio.getUtcOffset() == 0 && !radio.isDstActive());
          display2.printf("%s %02d:%02d:%02d", isRawUtc ? L_TIME_UTC : L_TIME,
                         timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
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
