#ifndef GLOBALS_H
#define GLOBALS_H

/**
 * ── BITFABRIK Transceiver v3.0 ──────────────────────────────────────────
 * Global State, Object Instances, and Shared Structures.
 * ──────────────────────────────────────────────────────────────────────────
 */

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <Adafruit_MCP23X17.h>
#include <Preferences.h>
#include <ESPAsyncWebServer.h>
#include "Constants.h"

// ── LovyanGFX Driver Configuration (Hosyond 4.0" TN) ────────────────────────
class LGFX_TRX : public lgfx::LGFX_Device
{
  lgfx::Panel_ST7796  _panel_instance;
  lgfx::Bus_SPI       _bus_instance;
  lgfx::Touch_FT5x06  _touch_instance; // Compatible with FT6336U

public:
  LGFX_TRX()
  {
    {
      auto cfg = _bus_instance.config();
      cfg.spi_host = SPI3_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 20000000;
      cfg.freq_read  = 8000000;
      cfg.pin_sclk = TFT_SCK;
      cfg.pin_mosi = TFT_MOSI;
      cfg.pin_miso = TFT_MISO;
      cfg.pin_dc   = TFT_DC;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }
    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs           = TFT_CS;
      cfg.pin_rst          = TFT_RST; // Pin A0
      cfg.panel_width      = 320;
      cfg.panel_height     = 480;
      cfg.bus_shared       = true;
      _panel_instance.config(cfg);
    }
    {
      auto cfg = _touch_instance.config();
      cfg.x_min      = 0;
      cfg.x_max      = 319;
      cfg.y_min      = 0;
      cfg.y_max      = 479;
      cfg.pin_sda    = 11; // A4
      cfg.pin_scl    = 12; // A5
      cfg.i2c_port   = 0;  // Shared I2C port
      cfg.i2c_addr   = 0x38;
      cfg.freq       = 400000;
      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);
    }
    setPanel(&_panel_instance);
  }
};

// ── Data Structures ────────────────────────────────────────────────────────

struct Band
{
  const char* label;      // Anzeige-Name (z.B. "40m")
  int         meter;      // Wellenlänge als Zahl (z.B. 40)
  long        freqMin;    // Untere Grenze für diesen Filter
  long        freqMax;    // Obere Grenze für diesen Filter
  long        freqDefault;// Startfrequenz
  int         rxRelay;    // Physischer Slot (0-5) für Empfang
  int         txRelay;    // Physischer Slot (0-5) für Senden
  const bool  sideBand;   // true = USB, false = LSB (Read-Only)
  bool        enabled;    // Ob dieser Slot bestückt/aktiv ist
};

struct Preset
{
  const char* label;
  long        freq;
};

struct VfoState
{
  long freq;
  int  band;
  bool usb;
  volatile bool occupied; // volatile: written by Core 0 (web), read by Core 1 (radio)
};

struct SWRResult
{
  float swr;
  float powerW;
  float vFwd;
  float vRef;
  float rssi;   // Voltage from receiver (0-3.3V)
  int   sLevel; // S-Unit (1-9)
};

// ── Shared Enumerations ──────────────────────────────────────────────────

enum class EncoderMode
{
    Tune,
    Volume,
    Power,
    Mic,
    Calibrate,
    Rit
};

// ── Global State Variables (Shared between Cores) ──────────────────────────

extern volatile bool g_tx;

// UI Synchronization Flags
extern volatile bool g_guiNeedsUpdate;       // VFO/Status changed → triggers TFT refresh
extern TaskHandle_t  g_networkTaskHandle;    // Set by TaskNetwork — used for event wakeup

// System Performance Metrics
extern volatile int g_cpuLoad0; // Core 0 utilization (%)
extern volatile int g_cpuLoad1; // Core 1 utilization (%)

// Encoder Accumulators
extern volatile int  g_encPos;
extern volatile unsigned long g_lastEncMove;
extern volatile unsigned long g_encInterval; // Time between last two pulses
extern volatile uint8_t g_oldEncState;
extern volatile int8_t  s_encAccum;
extern const int8_t ENC_STATES[];

// ── Hardware Object Singletons ──────────────────────────────────────────────

extern LGFX_TRX tft;
extern Adafruit_SSD1306 display1;
extern Adafruit_SSD1306 display2;
extern Adafruit_MCP23X17 mcp;
extern Preferences preferences;

// Forward declaration of the Manager
class EncoderManager;
extern EncoderManager encManager;

class RadioEngine;
extern RadioEngine radio;

class AudioManager;
extern AudioManager audio;

class SettingsManager;
extern SettingsManager settings;

// Concurrency
extern SemaphoreHandle_t g_mutex;
extern SemaphoreHandle_t g_hwMutex;

// ── Shared Function Prototypes ─────────────────────────────────────────────

SWRResult readSWR();

// ── Constant Arrays ────────────────────────────────────────────────────────

extern const Band BANDS[];
extern const Preset PRESETS[6][3];
extern const long STEPS[];

#endif
