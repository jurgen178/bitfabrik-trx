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
    volatile long _freq = 7100000L;    // Current VFO frequency in Hz
    volatile int  _band = 0;           // Active band index (10m by default)
    volatile bool _usb = false;        // true = USB, false = LSB
    int  _stepIdx = 1;                 // Index for tuning steps (100Hz default)

    volatile long _ritOffset = 0;               // Receiver Incremental Tuning offset (Hz)
    volatile bool _ritEnabled = false;          // RIT toggle
    double _bfoUsb = 9001500.0;        // Calibrated USB carrier offset
    double _bfoLsb = 8998500.0;        // Calibrated LSB carrier offset

    VfoState _vfoA = { 7100000, 3, false, false }; // State for VFO A (40m LSB)
    VfoState _vfoB = { 14200000, 2, true, false };  // State for VFO B (20m USB)
    volatile int      _activeVfo = 0;                 // Current selection
    VfoState  _memChannels[NUM_MEM_CHANNELS]; // Persistent memory slots
    uint32_t  _memRevision = 0;               // Incremented on every memStore()
    long _bandFreqs[6];                      // Per-band frequency memory
    bool _unlockedRange = false;             // Bypasses band limits (for GEN mode)
    int  _lastRelayBand = -1;                // Cache for I2C efficiency

    // ── VOX Settings ──
    volatile bool _voxEnabled = false;
    volatile int  _voxThreshold = 1000;               // ADC threshold (0-4095)
    volatile int  _voxDelay = 500;                    // Hang time in ms

    // ── Time & Location ──
    int  _utcOffset = 1;                     // Hours relative to UTC (e.g. +1 for Germany)
    bool _dstActive = true;                  // Whether Daylight Saving Time is currently active

public:
    RadioEngine();

    // ── Getters ──
    long getFrequency() const
    {
        return _freq;
    }
    int getBand() const
    {
        return _band;
    }
    bool isUsb() const
    {
        return _usb;
    }
    int getStepIdx() const
    {
        return _stepIdx;
    }
    long getRitOffset() const
    {
        return _ritOffset;
    }
    bool isRitEnabled() const
    {
        return _ritEnabled;
    }
    double getBfoUsb() const
    {
        return _bfoUsb;
    }
    double getBfoLsb() const
    {
        return _bfoLsb;
    }
    int getActiveVfo() const
    {
        return _activeVfo;
    }
    const VfoState& getVfoA() const
    {
        return _vfoA;
    }
    const VfoState& getVfoB() const
    {
        return _vfoB;
    }
    const VfoState* getMemChannels() const
    {
        return _memChannels;
    }
    bool isMemOccupied(int ch) const
    {
        return (ch >= 0 && ch < NUM_MEM_CHANNELS) && _memChannels[ch].occupied;
    }
    uint32_t getMemRevision() const { return _memRevision; }
    const long* getBandFreqs() const
    {
        return _bandFreqs;
    }

    // ── Setters ──
    void setFrequency(long f);
    void setStepIdx(int idx);
    void setRitEnabled(bool en);
    void setRitOffset(long offset);
    void setBfoUsb(double f)
    {
        _bfoUsb = f;
        updateBFO();
    }
    void setBfoLsb(double f)
    {
        _bfoLsb = f;
        updateBFO();
    }
    void setUsb(bool usb)
    {
        _usb = usb;
        updateBFO();
        updateLO();
        g_guiNeedsUpdate = true;
    }

    void setUnlockedRange(bool en);
    bool isUnlocked() const { return _unlockedRange; }

    long getMinFreq() const;
    long getMaxFreq() const;

    // ── VOX Control ──
    bool isVoxEnabled() const { return _voxEnabled; }
    void setVoxEnabled(bool en);
    int  getVoxThreshold() const { return _voxThreshold; }
    void setVoxThreshold(int val);
    int  getVoxDelay() const { return _voxDelay; }
    void setVoxDelay(int ms);

    // ── Time & Location ──
    int  getUtcOffset() const { return _utcOffset; }
    void setUtcOffset(int hours);
    bool isDstActive() const { return _dstActive; }
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
    void _updateLOInternal();
    void _updateBFOInternal();
    void _updateBandRelaysInternal(int oldIdx, int newIdx);
    void _selectBandInternal(int idx);
};

extern RadioEngine radio;

#endif
