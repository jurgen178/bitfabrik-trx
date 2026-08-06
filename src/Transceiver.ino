/**
 * ── BITFABRIK Transceiver v1.0 ──────────────────────────────────────────
 */

#include <FFat.h>
#include "Constants.h"
#include "Globals.h"
#include "Hardware.h"
#include "Display.h"
#include "DigitalEngine.h"
#include "Network.h"
#include "EncoderActions.h"
#include "RadioEngine.h"
#include "AudioManager.h"
#include "SettingsManager.h"

// ── GLOBAL STATE ALLOCATION ───────────────────────────────────────────────

std::atomic<bool> g_tx(false);
std::atomic<bool> g_mcpOk(false);
std::atomic<unsigned long> g_lastActivityTime(0);
SyncState g_sync;

// UI Synchronization
std::atomic<bool> g_guiNeedsUpdate(true);
TaskHandle_t  g_networkTaskHandle = NULL;

// System Health
volatile int g_cpuLoad0 = 0;
volatile int g_cpuLoad1 = 0;
volatile int g_wifiRssi = -100;
volatile int g_webClients = 0;
volatile int g_netActivity = 0;

// Encoder State Machine
volatile int  g_encPos      = 0;
volatile unsigned long g_lastEncMove = 0;
volatile unsigned long g_encInterval = 999;
volatile uint8_t g_oldEncState = 0;
volatile int8_t  s_encAccum  = 0;
const int8_t ENC_STATES[] = { 0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0 };

// ── HARDWARE SINGLETONS ───────────────────────────────────────────────────

LGFX_TRX tft;
Adafruit_SSD1306 display1(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_SSD1306 display2(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_MCP23X17 mcp;
Preferences preferences;
EncoderManager encManager;

SemaphoreHandle_t g_mutex;
SemaphoreHandle_t g_hwMutex;

// ── CONSTANT DATA (defined in Constants.cpp) ─────────────────────────────
// extern declarations are in Globals.h

// ── SYSTEM BOOT ───────────────────────────────────────────────────────────

void setup()
{
  Serial.begin(115200);
  delay(2000); // Give user time to open monitor
  Serial.println("\n\n--- BITFABRIK Transceiver Start ---");

  // Init concurrency locks
  g_mutex = xSemaphoreCreateMutex();
  g_hwMutex = xSemaphoreCreateRecursiveMutex();

  // Basic I/O
  pinMode(LED_RED, OUTPUT);
  digitalWrite(LED_RED, HIGH);

  // FFat Filesystem
  Serial.println("FS: Initializing FFat...");
  if (!FFat.begin(false)) {
      Serial.println("FS: FFat mount failed. Formatting...");
      FFat.begin(true);
  }
  Serial.println("FS: OK");

  Serial.println("I2C: Initializing...");
  Wire.begin();
  Wire.setClock(200000); // Stable 200kHz for multiple devices
  Wire.setTimeOut(100); // Prevent hang if device missing
  Serial.println("I2C: Scan start...");
  for (uint8_t address = 1; address < 127; address++)
  {
    Wire.beginTransmission(address);
    if (Wire.endTransmission() == 0)
    {
      Serial.printf("I2C: Found device at 0x%02X\n", address);
    }
  }

  // MCP23017 Expander Init
  Serial.println("MCP: Initializing...");
  if (!mcp.begin_I2C(0x27))
  {
      Serial.println("MCP: ERR - Not found at 0x27");
      g_mcpOk = false;
  }
  else
  {
      Serial.println("MCP: OK");
      g_mcpOk = true;
      // Initialize all 16 pins to a safe state
      for (int i = 0; i < 16; i++)
      {
          if (i == MCP_PIN_PTT)
          {
              mcp.pinMode(i, INPUT_PULLUP);
          }
          else
          {
              mcp.pinMode(i, OUTPUT);
              mcp.digitalWrite(i, LOW); // Ensure all relays/PA_ACTIVE are OFF
          }
      }
  }

  // Hardware Pin Config
  Serial.println("HW: Configuring pins...");
  const int busPins[] = { DDS_DATA, DDS_WCLK, DDS_RESET, LO_FQUD, BFO_FQUD };
  for (int p : busPins)
  {
      pinMode(p, OUTPUT);
  }
  pinMode(AUDIO_VOLUME_PWM, OUTPUT);
  pinMode(ZF_AGC_PWM, OUTPUT);
  pinMode(MIC_CS, OUTPUT);
  digitalWrite(MIC_CS, HIGH);
  pinMode(ENC_A, INPUT);
  pinMode(ENC_B, INPUT);
  pinMode(ENC_BTN, INPUT_PULLUP);

  // Encoder ISR Setup
  g_oldEncState = (digitalRead(ENC_A) << 1) | digitalRead(ENC_B);
  if (ENC_TYPE == 1)
  {
    attachInterrupt(digitalPinToInterrupt(ENC_A), encISR_Optical, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENC_B), encISR_Optical, CHANGE);
  }
  else
  {
    attachInterrupt(digitalPinToInterrupt(ENC_A), encISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENC_B), encISR, CHANGE);
  }

  // Visuals & Touch
  Serial.println("TFT: Initializing (ST7796S)...");
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  Serial.println("TFT: OK");

  Serial.println("OLED: Initializing Display 1 (0x3C)...");
  if(!display1.begin(SSD1306_SWITCHCAPVCC, OLED_ADRESSE_A))
  {
    Serial.println("OLED 1: ERR");
  }
  else
  {
    display1.clearDisplay();
    display1.display(); // Force clear to avoid "snow" at start
    Serial.println("OLED 1: OK");
  }

  Serial.println("OLED: Initializing Display 2 (0x3D)...");
  if(!display2.begin(SSD1306_SWITCHCAPVCC, OLED_ADRESSE_B))
  {
    Serial.println("OLED 2: ERR");
  }
  else
  {
    display2.clearDisplay();
    display2.display();
    Serial.println("OLED 2: OK");
  }

  Serial.println("DDS: Syncing bus...");
  dds_reset(); // Bus synchronization

  Serial.println("Settings: Loading...");
  settings.begin();

  Serial.println("Audio: Starting PWM...");
  audio.begin();

  Serial.println("Radio: Loading dynamic configuration...");
  radio.loadBandsFromJson();

  Serial.println("Radio: Setting default band...");
  ui.begin();
  radio.selectBand(radio.getBand());
  ui.drawFullUI();

  encManager.begin();

  // Multi-Core Task Scheduling - START LAST
  Serial.println("OS: Starting Background Services...");
  xTaskCreatePinnedToCore(TaskNetwork, "Network", 8192, NULL, 1, NULL, 0); // Core 0: Comm Background
  xTaskCreatePinnedToCore(TaskDigital, "Digital", 4096, NULL, 2, NULL, 0); // Core 0: FSK Timing

  Serial.println("OS: Starting Radio...");
  xTaskCreatePinnedToCore(TaskRadio,   "Radio",   8192, NULL, 3, NULL, 1); // Core 1: Hardware High Priority

  Serial.println("BOOT COMPLETE.");
}

void loop()
{
  // Tasks are managed by FreeRTOS. loop() just yields.
  vTaskDelay(pdMS_TO_TICKS(1000));
}
