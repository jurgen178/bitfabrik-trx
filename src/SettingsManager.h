#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

/**
 * ── SETTINGS MANAGER ───────────────────────────────────────────────────────
 * Centralizes all persistence logic (NVS) and handles the auto-save timer.
 * ──────────────────────────────────────────────────────────────────────────
 */

#include "Globals.h"

class SettingsManager
{
private:
    unsigned long _lastActivity = 0;
    bool _isUpdated = false;

public:
    SettingsManager();

    // ── Life Cycle ──
    void begin();
    void process(); // To be called in a loop to handle the 5s timer

    // ── Logic ──
    void setUpdated();
    void loadAll();
    void saveAll();
};

extern SettingsManager settings;

#endif
