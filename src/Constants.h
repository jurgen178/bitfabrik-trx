#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <Arduino.h>

/**
 * ── BITFABRIK Transceiver v3.0 ──────────────────────────────────────────
 * Central Hardware Configuration (Pins, Constants, and i18n).
 *
 * Target Board: Arduino Nano ESP32 (ESP32-S3)
 * NOTE: Ensure "By GPIO number (standard)" is selected in Tools -> Pin Numbering.
 * ──────────────────────────────────────────────────────────────────────────
 */

// ── Build Flags ───────────────────────────────────────────────────────────
#define DEBUG_SIM // Enable simulated RSSI/SWR and dummy RX decode (no hardware needed)

// ── Internationalization (i18n) ──────────────────────────────────────────
#define LANG_DE // Standard auf Deutsch für Tests

#if defined(LANG_DE)
  #define L_STARTUP_MSG   "--- BITFABRIK Transceiver Start ---"
  #define L_I2C_OK        "I2C initialisiert (400kHz)"
  #define L_MCP_ERR       "MCP23017 Fehler!"
  #define L_MCP_OK        "MCP23017 initialisiert"
  #define L_ENC_OPTICAL   "Encoder: Optisch (EM14)"
  #define L_ENC_MECHANIC  "Encoder: Mechanisch (KY-040)"
  #define L_TFT_TOUCH_OK  "ILI9488 & Touch bereit"
  #define L_DISP1_ERR     "Display 1 (0x3C) Fehler!"
  #define L_DISP2_ERR     "Display 2 (0x3D) Fehler!"
  #define L_PREFS_SAVED   "Einstellungen gespeichert."
  #define L_SYS_STARTED   "System gestartet."
  #define L_WIFI_CONN     "Verbinde mit Heimnetzwerk..."
  #define L_WIFI_FAIL     "Statische IP Konfiguration fehlgeschlagen!"
  #define L_AP_START      "\n=== Access Point gestartet ==="
  #define L_WIFI_OK       "\n=== WiFi Station verbunden ==="
  #define L_WIFI_ERR      "\n=== Heimnetzwerk nicht erreichbar ==="
  #define L_AP_MSG        "Bitte mit 'TRX-1-Hotspot' verbinden"
  #define L_TX_ON         "Radio Task: TX -> AN"
  #define L_TX_OFF        "Radio Task: TX -> AUS"
  #define L_SWR_ALARM     "!!! SWR ALARM: %.1f !!!"
  #define L_SSE_CONN      "SSE: Client verbunden."
  #define L_API_TX        "API v1: Sende-Anfrage: "
  #define L_SWR_ALARM_AUX "AUX: SWR ALARM!"
  #define L_BAND          "Band"
  #define L_MODE          "Modus"
  #define L_SEQ_RX_TX     "Sequenzer: RX -> TX bereit"
  #define L_SEQ_TX_RX     "Sequenzer: TX -> RX bereit"
  #define L_QUICK_PRESETS "SCHNELLWAHL:"
  #define L_PRESETS       "PRESETS:"
  #define L_MEM_CHANNELS  "MEM:"
  #define L_STEP          "Schritt"
  #define L_CALIBRATION   "KALIBRIERUNG"
  #define L_TX_ACTIVE     "TX: AKTIV"
  #define L_RX            "RX"
  #define L_DIGITAL_MODE  "Digital:"
  #define L_TIME_UTC      "Zeit UTC:"
  #define L_TIME          "Zeit:"
  #define L_UPTIME        "Uptime"
  #define L_FREE          "Frei"
  #define L_WAIT_SIGNAL   "Warte auf Signal..."
  #define H_LIVE_MONITOR  "Live Station"
  #define H_SIGNAL_HEALTH "Signal & Status"
  #define H_TUNING_CONTROLS "Abstimmung"
  #define H_DIGI_MSG      "Digitaler Funk"
  #define H_MAIL_GW       "Radio Email Gateway"
  #define H_DECODED       "Decoder Stream"
  #define H_TUNING_KNOB   "Abstimmung"
  #define H_DIGI_MODE     "Digitaler Modus"
  #define H_TRANSMIT      "Senden"
  #define H_CARRIER       "Träger"
  #define H_IDLE          "BEREIT"
  #define H_SENDING       "SENDET"
  #define H_MSG_PLACEHOLDER "Nachricht eingeben..."
  #define H_SEND_BTN      "Senden"
  #define H_DECODED_STREAM "Decodierter Stream"
  #define H_CLEAR_BTN     "Löschen"
  #define H_MEM_CHANNELS  "Speicherkanäle"
  #define H_SAVE_TO       "Speichern in:"
  #define H_STORE_BTN     "Sichern"
  #define H_BAND_MODE     "Band & Modus"
  #define L_MIC_GAIN      "Mic Gain"
  #define L_WAITING       "Warten..."
  #define L_SEND          "Senden"
  #define L_RIT_ON        "RIT AN"
  #define L_RIT_OFF       "RIT AUS"
  #define L_IDLE          "IDLE"
  #define L_SENDING       "SENDET"
  #define L_VOL           "Lautstärke"
  #define TX_PWR_CTRL     "Sendeleistung"
  #define L_MORSE_MODE    "Morse Modus"
  #define L_RTTY_MODE     "RTTY Modus"
  #define L_TX_STATUS     "Sende-Status"
  #define L_MAIL_RECIPIENT "Empfänger"
  #define L_MAIL_BODY     "Inhalt..."
  #define L_SAVE_TO       "SICHERN IN"
  #define L_SICHERN       "Sichern"
  #define L_TAB_VOX       "VOX"
  #define L_TAB_TIME      "ZEIT"
  #define L_VOX_ACTIVE    "VOX Aktiv"
  #define L_VOX_THRESH    "VOX Pegel"
  #define L_VOX_DELAY     "VOX Haltezeit"
  #define L_UTC_OFFSET    "UTC Versatz"
  #define L_DST           "Sommerzeit"
#elif defined(LANG_EN)
  #define L_STARTUP_MSG   "--- BITFABRIK Transceiver Start ---"
  #define L_I2C_OK        "I2C initialized at 400kHz"
  #define L_MCP_ERR       "MCP23017 Error!"
  #define L_MCP_OK        "MCP23017 initialized"
  #define L_ENC_OPTICAL   "Encoder: Optical Mode (EM14)"
  #define L_ENC_MECHANIC  "Encoder: Mechanical Mode (KY-040)"
  #define L_TFT_TOUCH_OK  "ILI9488 & Touch initialized"
  #define L_DISP1_ERR     "Display 1 (0x3C) failed!"
  #define L_DISP2_ERR     "Display 2 (0x3D) failed!"
  #define L_PREFS_SAVED   "Settings saved."
  #define L_SYS_STARTED   "System started."
  #define L_WIFI_CONN     "Connecting to home network"
  #define L_WIFI_FAIL     "Static IP configuration failed!"
  #define L_AP_START      "\n=== Access Point Started ==="
  #define L_WIFI_OK       "\n=== WiFi Station Connected ==="
  #define L_WIFI_ERR      "\n=== Home network unreachable ==="
  #define L_AP_MSG        "Please connect to 'TRX-1-Hotspot'"
  #define L_TX_ON         "Radio Task: TX -> ON"
  #define L_TX_OFF        "Radio Task: TX -> OFF"
  #define L_SWR_ALARM     "!!! SWR ALARM: %.1f !!!"
  #define L_SSE_CONN      "SSE: Client connected."
  #define L_API_TX        "API v1: Transmit request: "
  #define L_SWR_ALARM_AUX "AUX: SWR ALARM!"
  #define L_BAND          "Band"
  #define L_MODE          "Mode"
  #define L_SEQ_RX_TX     "Sequencer: RX -> TX ready"
  #define L_SEQ_TX_RX     "Sequencer: TX -> RX ready"
  #define L_QUICK_PRESETS "QUICK PRESETS:"
  #define L_PRESETS       "PRESETS:"
  #define L_MEM_CHANNELS  "MEM:"
  #define L_STEP          "Step"
  #define L_CALIBRATION   "CALIBRATION"
  #define L_TX_ACTIVE     "TX: ACTIVE"
  #define L_RX            "RX"
  #define L_DIGITAL_MODE  "Digital:"
  #define L_TIME_UTC      "Time UTC:"
  #define L_TIME          "Time:"
  #define L_UPTIME        "Uptime"
  #define L_FREE          "Free"
  #define L_WAIT_SIGNAL   "Waiting for signal..."
  #define H_LIVE_MONITOR  "Live Station"
  #define H_SIGNAL_HEALTH "Signal & Health"
  #define H_TUNING_CONTROLS "Tuning Controls"
  #define H_DIGI_MSG      "Digital Messaging"
  #define H_MAIL_GW       "Radio Email Gateway"
  #define H_DECODED       "Decoded Stream"
  #define H_TUNING_KNOB   "Tuning"
  #define H_DRAG_TUNE     "Rotate to Tune"
  #define H_DIGI_MODE     "Digital Mode"
  #define H_TRANSMIT      "Transmit"
  #define H_CARRIER       "Carrier"
  #define H_IDLE          "IDLE"
  #define H_SENDING       "SENDING"
  #define H_MSG_PLACEHOLDER "Enter message..."
  #define H_SEND_BTN      "Send"
  #define H_DECODED_STREAM "Decoded Stream"
  #define H_CLEAR_BTN     "Clear"
  #define H_MEM_CHANNELS  "Memory Channels"
  #define H_SAVE_TO       "Save to:"
  #define H_STORE_BTN     "Store"
  #define H_BAND_MODE     "Band & Mode"
  #define L_MIC_GAIN      "Mic Gain"
  #define L_WAITING       "Waiting..."
  #define L_SEND          "Send"
  #define L_RIT_ON        "RIT ON"
  #define L_RIT_OFF       "RIT OFF"
  #define L_IDLE          "IDLE"
  #define L_SENDING       "SENDING"
  #define L_VOL           "Audio Volume"
  #define TX_PWR_CTRL     "TX Power Level"
  #define L_MORSE_MODE    "Morse Mode"
  #define L_RTTY_MODE     "RTTY Mode"
  #define L_TX_STATUS     "TX Status"
  #define L_MAIL_RECIPIENT "Recipient"
  #define L_MAIL_BODY     "Body..."
  #define L_SAVE_TO       "SAVE TO"
  #define L_SICHERN       "Store"
  #define L_TAB_VOX       "VOX"
  #define L_TAB_TIME      "TIME"
  #define L_VOX_ACTIVE    "VOX Active"
  #define L_VOX_THRESH    "VOX Threshold"
  #define L_VOX_DELAY     "VOX Delay"
  #define L_UTC_OFFSET    "UTC Offset"
  #define L_DST           "Daylight Savings"
#endif

// ── PIN DEFINITIONS (Native ESP32 GPIOs) ──────────────────────────────────
// Status LEDs — already defined by the board in pins_arduino.h, do not redeclare
// LED_RED = 46, LED_GREEN = 0, LED_BLUE = 45, LED_BUILTIN = 48

// SWR / RSSI Inputs
constexpr int PIN_SWR_FWD = 2;   // Stockton Forward Voltage
constexpr int PIN_SWR_REF = 3;   // Stockton Reflected Voltage
constexpr int PIN_RSSI    = 1;   // Board-Label A0: Signal Strength Input
constexpr int PIN_VOX_ADC = 4;   // Board-Label A3: Analog VOX Detector

// DDS Bus (Shared for all AD9850 modules, FQUD is per-module Chip Select)
constexpr int DDS_DATA  = 8;    // D5: Shared Data Line
constexpr int DDS_WCLK  = 9;    // D6: Shared Word Clock
constexpr int DDS_RESET = 17;   // D8: Shared Hardware Reset
constexpr int LO_FQUD   = 10;   // D7: Freq Update for LO (VFO)
constexpr int BFO_FQUD  = 44;   // D0: Freq Update for BFO (Mode)

// Audio & PA Control (PWM Outputs)
constexpr int      PIN_VOLUME_PWM  = 13;      // A6: TDA7052A DC Gain Control
constexpr int      VOL_PWM_CHAN    = 0;        // LEDC Channel for legacy ESP cores
constexpr uint32_t VOL_PWM_FREQ   = 100000;   // 100 kHz — optimal EMV for HF
constexpr uint8_t  VOL_PWM_RES    = 8;        // 8-Bit resolution (0-255)
constexpr int      VOL_PWM_MIN    = 23;        // ~0.3 V (Mute threshold)
constexpr int      VOL_PWM_MAX    = 108;       // ~1.4 V (Max gain)

constexpr int      PIN_PA_PWR_PWM  = 14;      // A7: Variable PA Bias / Driver Level
constexpr int      PA_PWR_PWM_CHAN = 1;        // LEDC Channel 1
constexpr uint32_t PA_PWR_PWM_FREQ = 100000;
constexpr uint8_t  PA_PWR_PWM_RES  = 8;
constexpr int      PA_PWR_MIN      = 126;      // FET Bias Start Point (0-255)

// SPI Bus — TFT Display
constexpr int TFT_SCK  = 48;   // D13
constexpr int TFT_MISO = 47;   // D12
constexpr int TFT_MOSI = 38;   // D11
constexpr int TFT_CS   = 18;   // D9
constexpr int TFT_DC   = 21;   // D10
constexpr int TFT_RST  = -1;   // Tied to hardware reset
constexpr int MIC_CS   = 43;   // D1: MCP41010 Mic Gain Pot

// User Inputs (Encoder)
constexpr int ENC_A   = D2;    // Optical Channel A
constexpr int ENC_B   = D3;    // Optical Channel B
constexpr int ENC_BTN = D4;    // Integrated Push Button

// Native GPIO
constexpr int PIN_TX_PA_ACTIVE = 14; // A7: Hardware enable for PA Stage

// MCP23017 Pin Assignments (I2C expander — NOT native GPIO!)
constexpr int MCP_RELAY_TXRX  = 7;  // Main antenna switch relay
constexpr int MCP_PIN_PA_BIAS = 6;  // PA transistor bias enable
constexpr int MCP_PIN_PTT     = 14; // PTT button input (GPA6)

// ── HARDWARE PARAMETERS ───────────────────────────────────────────────────
constexpr double REF_FREQ = 125000000.0; // AD9850 Reference Clock
constexpr double ZF_FREQ  = 9000000.0;   // 9.0 MHz Intermediate Frequency
constexpr int    RTTY_SHIFT  = 170;      // FSK Shift in Hz
constexpr int    RTTY_BIT_MS = 22;       // 45.45 Baud bit duration

// ENC_TYPE must stay as #define — used in preprocessor #if directives
#define ENC_TYPE             1   // 0 = Mechanical (KY-040), 1 = Optical (EM14)
constexpr int ENC_STEPS_PER_DETENT = 2;  // 4 Mechanical, 2 Precise Optical, 1 Fast Optical
constexpr int ENC_LONG_PRESS_MS    = 500; // Duration for long-press action

// OLED
constexpr int     SCREEN_WIDTH   = 128;
constexpr int     SCREEN_HEIGHT  = 64;
constexpr int     OLED_RESET     = -1;    // Shared with ESP reset
constexpr uint8_t OLED_ADRESSE_A = 0x3C;
constexpr uint8_t OLED_ADRESSE_B = 0x3D;

// Misc
constexpr int NUM_BANDS        = 6;
constexpr int NUM_STEPS        = 4;
constexpr int NUM_MEM_CHANNELS = 10;
constexpr int CW_HANG_TIME     = 500; // Semi-break-in fall-back delay (ms)

// SWR Metering (Stockton Bridge 1:20, BAT85 diode compensation)
constexpr float SWR_V_REF    = 3.3f;
constexpr float SWR_ADC_MAX  = 4095.0f;
constexpr float SWR_DIODE_VF = 0.35f;  // BAT85 Schottky forward voltage
constexpr float SWR_MIN_VFWD = 0.1f;   // Minimum forward voltage for measurement
constexpr int   SWR_SAMPLES  = 32;     // Oversampling count

// Colors (LovyanGFX RGB565)
constexpr uint16_t TRX_AMBER     = 0xFB20; // Vibrant Web-Orange (#ff6600)
constexpr uint16_t TRX_AMBER_LOW = 0x8200;
constexpr uint16_t TRX_BLUE      = 0x001F;

#endif
