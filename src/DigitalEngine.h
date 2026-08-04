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
    int           _mode = 0;        // 0 = Morse (CW), 1 = RTTY
    volatile bool _isBusy    = false; // True while a message is being sent
    volatile bool _isKeyed   = false; // Real-time CW keying signal read by TaskRadio
    bool          _isRttyFigs = false;
    char          _rxText[128] = {};
    uint8_t       _rxLen = 0;

public:
    MorseQueue queue; // Shared buffer between API writers and TaskDigital reader

    int  getMode() const          { return _mode; }
    void setMode(int m)           { _mode = m; }
    bool isBusy() const           { return _isBusy; }
    void setBusy(bool b)          { _isBusy = b; }
    bool isKeyed() const          { return _isKeyed; }
    void setKeyed(bool k)         { _isKeyed = k; }
    bool isRttyFigs() const       { return _isRttyFigs; }
    void setRttyFigs(bool f)      { _isRttyFigs = f; }
    const char* getRxText() const { return _rxText; }

    void addRxChar(char c) {
        if (_rxLen < sizeof(_rxText) - 1) {
            _rxText[_rxLen++] = c;
            _rxText[_rxLen]   = '\0';
        }
    }

    // Returns up to the last n characters of the RX log
    String getLastRxText(int n) const {
        int start = (_rxLen > n) ? _rxLen - n : 0;
        return String(_rxText + start);
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
