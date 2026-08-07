#ifndef DISPLAY_H
#define DISPLAY_H

#include "Globals.h"
#include <functional>

/**
 * ── BUTTON ──────────────────────────────────────────────────────────────
 * Data-driven UI element: position, label, state, and tap callback.
 * Replaces scattered pixel arithmetic throughout AppMode subclasses.
 */
struct Button {
    int16_t  x = 0, y = 0, w = 0, h = 0;
    const char* label = nullptr;
    char     subtitleBuf[12] = {}; // Non-empty → shown below label (e.g. frequency)
    bool     active      = false;
    bool     disabled    = false;
    uint16_t colorActive   = TRX_BLUE;
    uint16_t colorInactive = 0x2104;
    const lgfx::GFXfont* labelFont = &fonts::FreeMono9pt7b;
    std::function<void()> onTap;

    bool hit(int tx, int ty) const {
        return !disabled && tx > x && tx < x + w && ty > y && ty < y + h;
    }
    void draw() const;
    void setFreqSubtitle(long freq) {
        snprintf(subtitleBuf, sizeof(subtitleBuf), "%ld.%03ld",
                 freq / 1000000L, (freq % 1000000L) / 1000L);
    }
};

/**
 * ── UI State Structure ──────────────────────────────────────────────────
 * Holds the last known state of the UI to allow smart redrawing.
 */
struct UIState
{
    long freq = -1;
    int  band = -1;
    int  vfo = -1;
    bool ritEnabled = false;
    long ritOffset = -2000000;
    EncoderMode mode = static_cast<EncoderMode>(-1);
    int vol = -1;
    int pwr = -1;
    int mic = -1;
    double bfo = -1;
};

/**
 * ── App Mode Interface ──────────────────────────────────────────────────
 * Base class for different functional modes (Radio, Generator, etc.)
 */
enum class DisplayMode {
    Radio,
    Generator,
    Settings,
    Rit,
    Volume,
    Power,
    Mic
};

class AppMode
{
protected:
    void drawFrequency(LGFX_Sprite& canvas, long freq, bool usb, bool showMode = true);
    void drawFullPageHeader(const char* label, uint16_t color, bool showBackButton = true);

public:
    virtual ~AppMode() {}
    virtual void onEnter() {}
    virtual void onLeave() {}
    virtual void render(bool force) = 0;
    virtual void handleTouch(int x, int y, bool longPress = false) = 0;
    virtual void onRotate(int delta) {}
    virtual void onButtonShort() {}
    virtual void onButtonLong() {}
    virtual const char* getName() = 0;
};

/**
 * ── SPECIALIZED MODES ───────────────────────────────────────────────────
 */

class RitMode : public AppMode {
public:
    void render(bool force) override;
    void handleTouch(int x, int y, bool longPress) override;
    void onRotate(int delta) override;
    const char* getName() override { return "RIT"; }
};

class ParamMode : public AppMode {
protected:
    void drawBar(LGFX_Sprite& canvas, const char* label, int val, uint16_t color);
public:
    void handleTouch(int x, int y, bool longPress) override;
    void onRotate(int delta) override;
};

class VolumeMode : public ParamMode {
public:
    void render(bool force) override;
    void onRotate(int delta) override;
    void onButtonShort() override;
    const char* getName() override { return "VOL"; }
};

class PowerMode : public ParamMode {
public:
    void render(bool force) override;
    void onRotate(int delta) override;
    void onButtonShort() override;
    const char* getName() override { return "PWR"; }
};

class MicMode : public ParamMode {
public:
    void render(bool force) override;
    void onRotate(int delta) override;
    void onButtonShort() override;
    const char* getName() override { return "MIC"; }
};

/**
 * ── Display Controller ──────────────────────────────────────────────────
 */
class DisplayController
{
private:
    LGFX_Sprite topCanvas;
    UIState last;
    AppMode* currentMode = nullptr;
    AppMode* previousMode = nullptr;
    uint32_t modeTimeout = 0;
    bool initialized = false;

    // Mode instances
    class RadioMode* radioMode;
    class GeneratorMode* genMode;
    class SettingsMode* settingsMode;
    class RitMode* ritMode;
    class VolumeMode* volMode;
    class PowerMode* pwrMode;
    class MicMode* micMode;

public:
    DisplayController();
    void begin();
    void drawFullUI();
    void update(bool force = false);

    // Mode Management
    void setMode(DisplayMode mode);
    AppMode* getCurrentMode() { return currentMode; }
    void checkTimeout();

    LGFX_Sprite& getCanvas() { return topCanvas; }
    UIState& getLastState() { return last; }
};

extern DisplayController ui;

// OLED updates remain standalone for the background task
void updateOled1();
void updateOled2();

#endif
