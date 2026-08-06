# Anschlussplan (Pinout) - Arduino Nano ESP32

Diese Dokumentation zeigt die exakte physische Verbindung für das **Hosyond 4.0" TN Display** (ST7796S & FT5x06) und alle weiteren Hardware-Komponenten.

## 1. Gemeinsamer I2C-Bus (OLEDs, MCP23017 & Touch)
Alle I2C-Geräte hängen parallel an denselben zwei Leitungen.
- **CTP_SDA** (Touch) geht an **A4**
- **CTP_SCL** (Touch) geht an **A5**

| Signal | GPIO (Code) | Board-Label |
| :--- | :--- | :--- |
| **SDA** | 11 | **A4** |
| **SCL** | 12 | **A5** |

## 2. SPI-Bus (TFT-Display)
| Signal | GPIO (Code) | Board-Label | Display-Pin |
| :--- | :--- | :--- | :--- |
| **SCK** | 48 | **D13** | 7 (SCK) |
| **MOSI** | 38 | **D11** | 6 (SDI) |
| **MISO** | 47 | **D12** | 9 (SDO) |
| **LCD_CS** | 18 | **D9** | 3 (CS) |
| **LCD_RS** | 21 | **D10** | 5 (RS/DC) |
| **TFT_RST** | -1 | **RST** | 4 (LCD_RST) & 11 (CTP_RST) |

## 3. DDS BUS (Shared Architektur)
| Signal | GPIO (Code) | Board-Label | Ziel |
| :--- | :--- | :--- | :--- |
| **DDS_DATA** | 8 | **D5** | DATA (beide Module) |
| **DDS_WCLK** | 9 | **D6** | WCLK (beide Module) |
| **DDS_RESET**| 17 | **D8** | RESET (beide Module) |
| **LO_FQUD**  | 10 | **D7** | FQUD (nur VFO) |
| **BFO_FQUD** | 44 | **D0** | FQUD (nur BFO) |

## 4. Audio & Bedienelemente
| Funktion | GPIO (Code) | Board-Label |
| :--- | :--- | :--- |
| **Audio Volume PWM** | 13 | **A6** |
| **Encoder A** | 5 | **D2** |
| **Encoder B** | 6 | **D3** |
| **Encoder Btn** | 7 | **D4** |
| **PTT Taste** | 4 | **A3** |
| **SWR FWD** | 2 | **A1** |
| **SWR REF** | 3 | **A2** |
| **RSSI / S-Meter**| 1 | **A0** |
| **ZF_AGC_PWM** | 14 | **A7** |
| **Mic Gain CS**| 43 | **D1** |

## 5. MCP23017 Ausgänge (Getrennte RX/TX Filterbänke)
Der Transceiver nutzt zwei unabhängige Gruppen von je 6 Relais für Empfangs- (RX) und Sende-Filter (TX). Ein automatisches Interlock in der Software stellt sicher, dass immer nur die benötigte Gruppe aktiv ist.

| MCP-Pin | Gruppe | Funktion |
| :--- | :--- | :--- |
| **0 - 5** | **RX Filter** | Slots für 10m, 15m, 20m, 40m, 80m, 160m (Empfang) |
| **8 - 13**| **TX Filter** | Slots für 10m, 15m, 20m, 40m, 80m, 160m (Senden) |
| **7** | **TX Control** | **PA_ACTIVE** / Haupt-Antennenrelais |
| **6** | **RESERVE** | Frei |
| **14 - 15**| **RESERVE** | Frei |

## 6. Ressourcen-Check
- **Native GPIOs**: 0 frei (Alle Header-Pins belegt).
- **MCP23017 Pins**: 8 frei.
- **Interne RGB-LED**: Rot (46), Grün (0), Blau (45) - für Statusmeldungen reserviert.
