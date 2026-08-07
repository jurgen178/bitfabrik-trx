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
    volatile int volume = 50;   // Audio Gain 0-100% (Controls BOTH ZF and NF)
    volatile int paPower = 100; // Sendeleistung 0-100%
    volatile int micGain = 50;  // Microphone Gain 0-100%

public:
    AudioManager();

    // ── Getters ──
    int getVolume() const { return volume; }
    int getPaPower() const { return paPower; }
    int getMicGain() const { return micGain; }

    // ── Setters & Logic ──
    void setVolume(int newVolume);
    void setPaPower(int newLevel);
    void setMicGain(int newGain);

    // Initialization (PWM Pins)
    void begin();
};

extern AudioManager audio;

#endif
