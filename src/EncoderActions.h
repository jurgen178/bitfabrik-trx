#ifndef ENCODERACTIONS_H
#define ENCODERACTIONS_H

/**
 * ── BITFABRIK Transceiver v3.0 ──────────────────────────────────────────
 * Polymorphic Encoder Handling (Strategy Pattern).
 *
 * This module allows modular switching of the rotary encoder's function
 * (Tuning, Volume, Power, etc.) without complex 'if' trees.
 * ──────────────────────────────────────────────────────────────────────────
 */

#include "Globals.h"

/**
 * Interface for any mode the rotary encoder can operate in.
 */
class EncoderHandler
{
public:
    virtual ~EncoderHandler()
    {
    }
    virtual void onRotate(int delta) = 0;
    virtual String getDisplayLabel() = 0;
    virtual void renderFocused(Adafruit_SSD1306 &display) = 0; // Specialized rendering for OLED
    virtual void onEnter()
    {
    }
    virtual void onLeave()
    {
    }
};

// ── Specialized Handlers ────────────────────────────────────────────────────

class TuneHandler : public EncoderHandler
{
public:
    void onRotate(int delta) override;
    String getDisplayLabel() override;
    void renderFocused(Adafruit_SSD1306 &display) override;
};

class VolumeHandler : public EncoderHandler
{
public:
    void onRotate(int delta) override;
    String getDisplayLabel() override;
    void renderFocused(Adafruit_SSD1306 &display) override;
};

class PowerHandler : public EncoderHandler
{
public:
    void onRotate(int delta) override;
    String getDisplayLabel() override;
    void renderFocused(Adafruit_SSD1306 &display) override;
};

class MicGainHandler : public EncoderHandler
{
public:
    void onRotate(int delta) override;
    String getDisplayLabel() override;
    void renderFocused(Adafruit_SSD1306 &display) override;
};

class CalibrateHandler : public EncoderHandler
{
public:
    void onRotate(int delta) override;
    String getDisplayLabel() override;
    void renderFocused(Adafruit_SSD1306 &display) override;
};

class RitHandler : public EncoderHandler
{
public:
    void onRotate(int delta) override;
    String getDisplayLabel() override;
    void renderFocused(Adafruit_SSD1306 &display) override;
};

// Forward declaration of the Manager
enum EncoderMode;

class EncoderManager
{
private:
    EncoderHandler* currentHandler = nullptr;
    TuneHandler tune;
    VolumeHandler vol;
    PowerHandler power;
    MicGainHandler mic;
    CalibrateHandler cal;
    RitHandler rit;
    EncoderMode mode;
    unsigned long lastActivity = 0;

public:
    EncoderManager();
    void begin();
    void setMode(EncoderMode newMode);
    void cycleMode();
    void handleRotation(int delta);
    void checkTimeout();
    EncoderHandler* getHandler() { return currentHandler; }
    EncoderMode getMode() { return mode; }
};

extern EncoderManager encManager;

#endif
