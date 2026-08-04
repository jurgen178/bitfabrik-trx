#ifndef HARDWARE_H
#define HARDWARE_H

/**
 * ── BITFABRIK Transceiver v3.0 ──────────────────────────────────────────
 * Hardware Abstraction Layer (HAL).
 *
 * Manages low-level DDS communication, Band/TX/RX sequencing,
 * and power/SWR measurement logic.
 * ──────────────────────────────────────────────────────────────────────────
 */

#include "Globals.h"
#include "Network.h"

// ── DDS Driver (Shared Bus API) ───────────────────────────────────────────
void dds_pulse(int pin);
void dds_setFreq(double freq, int fqud);
void dds_reset();

// ── Radio Control ─────────────────────────────────────────────────────────
void setTxRx(bool tx);

// ── VFO & Memory Persistence ──────────────────────────────────────────────
void savePreferences();

// ── Power & Metering ──────────────────────────────────────────────────────
SWRResult readSWR();
void setVolume(int vol);
void setPaPower(int level);
void setMicGain(int gain);

// ── Background Tasks ──────────────────────────────────────────────────────
void TaskRadio(void *pvParameters);

// ── ISR Callbacks ─────────────────────────────────────────────────────────
void IRAM_ATTR encISR_Optical();
void IRAM_ATTR encISR();

#endif
