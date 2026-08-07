#ifndef LOGMANAGER_H
#define LOGMANAGER_H

/**
 * ── LOG MANAGER ───────────────────────────────────────────────────────────
 * Automatically records transmission sessions to FFat.
 * Uses a hysteresis to group rapid TX/RX cycles into single log entries.
 * ──────────────────────────────────────────────────────────────────────────
 */

#include "Globals.h"

class LogManager
{
private:
    SemaphoreHandle_t logMutex = nullptr;

    unsigned long sessionStartTime = 0;
    unsigned long totalTxDurationMs = 0;
    unsigned long lastTxEndTime = 0;
    unsigned long currentTxStartTime = 0;

    long sessionFreq = 0;
    String sessionMode = "";

    bool inSession = false;
    bool currentlyTransmitting = false;
    bool wasDigitalSession = false; // Remembers if any part of the session used digital engine

    unsigned long currentTimeoutMs = 120000; // Default 120s
    const unsigned long DEFAULT_TIMEOUT_MS = 120000;
    const unsigned long DIGITAL_TIMEOUT_MS = 1000; // 1s is enough for digital packets

    void writeEntry(const char* timeStr, long freq, long duration, const char* mode);

public:
    LogManager();
    void begin();

    // Called by Hardware Sequencer
    void notifyTxState(bool tx);

    // Background processing (timeout check)
    void process();

    void clearLog();
    String getLogJson();
};

extern LogManager logger;

#endif
