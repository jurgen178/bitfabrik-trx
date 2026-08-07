#include "LogManager.h"
#include "RadioEngine.h"
#include "DigitalEngine.h"
#include <FFat.h>
#include <time.h>

LogManager logger;

LogManager::LogManager() {}

void LogManager::begin()
{
    logMutex = xSemaphoreCreateMutex();

    if (!FFat.exists("/tx_log.json")) {
        File f = FFat.open("/tx_log.json", "w");
        if (f) {
            f.print("[]");
            f.close();
        }
    }
}

void LogManager::notifyTxState(bool tx)
{
    if (!logMutex) return;
    if (xSemaphoreTake(logMutex, pdMS_TO_TICKS(10)) != pdTRUE) return;

    unsigned long now = millis();

    if (tx && !currentlyTransmitting) {
        // --- TX START ---
        if (!inSession) {
            inSession = true;
            sessionStartTime = time(nullptr);
            sessionFreq = radio.getFrequency();
            sessionMode = radio.isUsb() ? "USB" : "LSB";
            totalTxDurationMs = 0;
            currentTimeoutMs = DEFAULT_TIMEOUT_MS;
            wasDigitalSession = false;
        }
        currentTxStartTime = now;
        currentlyTransmitting = true;

        if (digital.isBusy()) wasDigitalSession = true;
    }
    else if (!tx && currentlyTransmitting) {
        // --- TX END ---
        totalTxDurationMs += (now - currentTxStartTime);
        lastTxEndTime = now;
        currentlyTransmitting = false;

        if (wasDigitalSession || digital.isBusy()) {
            currentTimeoutMs = DIGITAL_TIMEOUT_MS;
            if (digital.getActionType() == "EMAIL") {
                sessionMode = "EMAIL";
            } else {
                sessionMode = (digital.getMode() == 0) ? "CW" : "RTTY";
            }
        }
    }

    xSemaphoreGive(logMutex);
}

void LogManager::process()
{
    if (!logMutex) return;

    char timeStr[25] = "";
    long outFreq = 0;
    long outDuration = 0;
    String outMode = "";
    bool shouldWrite = false;

    if (xSemaphoreTake(logMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        if (inSession && !currentlyTransmitting) {
            if (millis() - lastTxEndTime > currentTimeoutMs) {
                // Session is over - copy data to local variables for safe writing
                if (totalTxDurationMs >= 100) {
                    struct tm *timeinfo;
                    time_t t = sessionStartTime;
                    timeinfo = gmtime(&t);
                    strftime(timeStr, sizeof(timeStr), "%Y-%m-%dT%H:%M:%SZ", timeinfo);

                    outFreq = sessionFreq;
                    outDuration = totalTxDurationMs / 1000;
                    if (outDuration == 0) outDuration = 1;
                    outMode = sessionMode;
                    shouldWrite = true;
                }

                // Reset session state in RAM immediately
                inSession = false;
            }
        }
        xSemaphoreGive(logMutex);
    }

    if (shouldWrite) {
        writeEntry(timeStr, outFreq, outDuration, outMode.c_str());
    }
}

void LogManager::writeEntry(const char* timeStr, long freq, long duration, const char* mode)
{
    File f = FFat.open("/tx_log.json", "r+");
    if (!f) return;

    if (f.size() > 2) {
        f.seek(f.size() - 1);
        f.print(",");
    } else {
        f.seek(0);
        f.print("[");
    }

    f.printf("{\"time\":\"%s\",\"freq\":%ld,\"duration\":%ld,\"mode\":\"%s\"}]",
             timeStr, freq, duration, mode);
    f.close();

    Serial.printf("LOG: Saved TX session (%ld s on %ld Hz)\n", duration, freq);
}

void LogManager::clearLog()
{
    if (!logMutex) return;
    if (xSemaphoreTake(logMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        File f = FFat.open("/tx_log.json", "w");
        if (f) {
            f.print("[]");
            f.close();
        }
        xSemaphoreGive(logMutex);
    }
}
