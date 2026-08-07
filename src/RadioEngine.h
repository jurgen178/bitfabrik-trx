#ifndef RADIOENGINE_H
#define RADIOENGINE_H

/**
 * ── RADIO ENGINE ──────────────────────────────────────────────────────────
 * Core radio state management and hardware orchestration.
 *
 * This class encapsulates all variables and logic related to frequency
 * control, band switching, VFO management, and modulation modes.
 * ──────────────────────────────────────────────────────────────────────────
 */

#include "Globals.h"

class RadioEngine
{
private:
    // ── Internal State ──
    volatile long freq = 7100000L;    // Current VFO frequency in Hz
    volatile int  band = 0;           // Active band index (10m by default)
    volatile bool usb = false;        // true = USB, false = LSB
    int  stepIdx = 1;                 // Index for tuning steps (100Hz default)

    volatile long ritOffset = 0;               // Receiver Incremental Tuning offset (Hz)
    volatile bool ritEnabled = false;          // RIT toggle
    double bfoUsb = 9001500.0;        // Calibrated USB carrier offset
    double bfoLsb = 8998500.0;        // Calibrated LSB carrier offset

    VfoState vfoA = { 7100000, 3, false, false }; // State for VFO A (40m LSB)
    VfoState vfoB = { 14200000, 2, true, false };  // State for VFO B (20m USB)
    volatile int      activeVfo = 0;                 // Current selection
    VfoState  memChannels[NUM_MEM_CHANNELS]; // Persistent memory slots
    uint32_t  memRevision = 0;               // Incremented on every memStore()
    long bandFreqs[6];                      // Per-band frequency memory
    bool unlockedRange = false;             // Bypasses band limits (for GEN mode)
    int  lastRelayBand = -1;                // Cache for I2C efficiency

    // ── VOX Settings ──
    volatile bool voxEnabled = false;
    volatile int  voxThreshold = 1000;               // ADC threshold (0-4095)
    volatile int  voxDelay = 500;                    // Hang time in ms

    // ── Time & Location ──
    int  utcOffset = 1;                     // Hours relative to UTC (e.g. +1 for Germany)
    bool dstActive = true;                  // Whether Daylight Saving Time is currently active

public:
    RadioEngine();

    // ── Getters ──
    long getFrequency() const { return freq; }
    int getBand() const { return band; }
    bool isUsb() const { return usb; }
    int getStepIdx() const { return stepIdx; }
    long getRitOffset() const { return ritOffset; }
    bool isRitEnabled() const { return ritEnabled; }
    double getBfoUsb() const { return bfoUsb; }
    double getBfoLsb() const { return bfoLsb; }
    int getActiveVfo() const { return activeVfo; }
    const VfoState& getVfoA() const { return vfoA; }
    const VfoState& getVfoB() const { return vfoB; }
    const VfoState* getMemChannels() const { return memChannels; }
    bool isMemOccupied(int ch) const { return (ch >= 0 && ch < NUM_MEM_CHANNELS) && memChannels[ch].occupied; }
    uint32_t getMemRevision() const { return memRevision; }
    const long* getBandFreqs() const { return bandFreqs; }

    // ── Setters ──
    void setFrequency(long newFreq);
    void setStepIdx(int newIdx);
    void setRitEnabled(bool enabled);
    void setRitOffset(long newOffset);
    void setBfoUsb(double newBfo);
    void setBfoLsb(double newBfo);
    void setUsb(bool newUsb);

    void setUnlockedRange(bool en);
    bool isUnlocked() const { return unlockedRange; }

    long getMinFreq() const;
    long getMaxFreq() const;

    // ── VOX Control ──
    bool isVoxEnabled() const { return voxEnabled; }
    void setVoxEnabled(bool en);
    int  getVoxThreshold() const { return voxThreshold; }
    void setVoxThreshold(int val);
    int  getVoxDelay() const { return voxDelay; }
    void setVoxDelay(int ms);

    // ── Time & Location ──
    int  getUtcOffset() const { return utcOffset; }
    void setUtcOffset(int hours);
    bool isDstActive() const { return dstActive; }
    void setDstActive(bool active);

    // ── Logic Methods ──
    void updateLO();
    void updateBFO();
    void updateBandRelays();
    void refreshRelays();  // Force re-latch all band filter pins (e.g. after TX transition)
    void selectBand(int idx);
    void saveActiveToVfo();
    void switchVfo(int target);
    void vfoCopy();
    void memStore(int ch);
    void memRecall(int ch);
    void memDelete(int ch);

    // Dynamic Configuration
    void loadBandsFromJson();
    void saveBandsToJson();

    // Persistence helpers (friend access for save/load)
    void loadFromPreferences();
    void saveToPreferences();

private:
    // Internal hardware-near methods (must be called with g_hwMutex held!)
    void updateLOInternal();
    void updateBFOInternal();
    void updateBandRelaysInternal(int oldIdx, int newIdx);
    void selectBandInternal(int idx);
};

extern RadioEngine radio;

#endif
