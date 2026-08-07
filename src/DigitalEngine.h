#ifndef DIGITALENGINE_H
#define DIGITALENGINE_H

#include "Globals.h"

// ── MORSE QUEUE ────────────────────────────────────────────────────────────
// Lock-free SPSC ring buffer — producer is web/touch UI, consumer is TaskDigital.
struct MorseQueue {
    static constexpr int SIZE = 64;
    char             buf[SIZE] = {};
    volatile uint8_t head = 0;
    volatile uint8_t tail = 0;

    bool push(char c) {
        uint8_t next = (tail + 1) % SIZE;
        if (next == head)
            return false; // full
        buf[tail] = c;
        tail = next;
        return true;
    }
    bool pop(char& c) {
        if (head == tail)
            return false; // empty
        c = buf[head];
        head = (head + 1) % SIZE;
        return true;
    }
    bool  empty() const { return head == tail; }
    void  clear()       { head = tail = 0; }
    void  pushString(const String& s) { for (char c : s) push(c); }
};

// ── DIGITAL ENGINE ─────────────────────────────────────────────────────────
// Owns all digital-mode state. TaskDigital drives the transmit loop on Core 0.
class DigitalEngine {
    int           mode = 0;        // 0 = Morse (CW), 1 = RTTY
    volatile bool busy    = false; // True while a message is being sent
    volatile bool keyed   = false; // Real-time CW keying signal read by TaskRadio
    bool          isRttyFigs = false;
    char          rxText[128] = {};
    uint8_t       rxLen = 0;
    String        lastActionType = ""; // e.g. "EMAIL"

public:
    MorseQueue queue; // Shared buffer between API writers and TaskDigital reader

    int  getMode() const          { return mode; }
    void setMode(int m)           { mode = m; }
    bool isBusy() const           { return busy; }
    void setBusy(bool b)          { busy = b; }
    bool isKeyed() const          { return keyed; }
    void setKeyed(bool k)         { keyed = k; }
    bool getRttyFigs() const      { return isRttyFigs; }
    void setRttyFigs(bool f)      { isRttyFigs = f; }
    const char* getRxText() const { return rxText; }

    void setActionType(String type) { lastActionType = type; }
    String getActionType() const    { return lastActionType; }

    void addRxChar(char c) {
        if (xSemaphoreTake(g_mutex, pdMS_TO_TICKS(5))) {
            if (rxLen < sizeof(rxText) - 1) {
                rxText[rxLen++] = c;
                rxText[rxLen]   = '\0';
            }
            xSemaphoreGive(g_mutex);
        }
    }

    // Returns up to the last n characters of the RX log
    String getLastRxText(int n) const {
        int start = (rxLen > n) ? rxLen - n : 0;
        return String(rxText + start);
    }
};

extern DigitalEngine digital;

// ── Protocol Functions ─────────────────────────────────────────────────────
void sendMorseChar(char c);
void sendRttyChar(char c);
void sendRttyByte(uint8_t data);
void sendRttyBit(bool mark);
void setRttyFreq(bool mark);

void TaskDigital(void *pvParameters);

#endif
