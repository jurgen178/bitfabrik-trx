#ifndef AUDIOMANAGER_H
#define AUDIOMANAGER_H

/**
 * ── AUDIO MANAGER ──────────────────────────────────────────────────────────
 * Handles all analog gain and power levels (PWM & SPI Potentiometers).
 * ──────────────────────────────────────────────────────────────────────────
 */

#include "Globals.h"

class AudioManager
{
private:
    volatile int _volume = 50;   // Audio Gain 0-100%
    volatile int _paPower = 100; // Transmission Power 0-100%
    volatile int _micGain = 50;  // Microphone Gain 0-100%

public:
    AudioManager();

    // ── Getters ──
    int getVolume() const
    {
        return _volume;
    }
    int getPaPower() const
    {
        return _paPower;
    }
    int getMicGain() const
    {
        return _micGain;
    }

    // ── Setters & Logic ──
    void setVolume(int vol);
    void setPaPower(int level);
    void setMicGain(int gain);

    // Initialization (PWM Pins)
    void begin();
};

extern AudioManager audio;

#endif
