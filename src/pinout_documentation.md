# Anschlussplan (Pinout) - Arduino Nano ESP32

Diese Dokumentation zeigt die exakte physische Verbindung zwischen dem **Arduino Nano ESP32** und den Hardware-Komponenten.

## 1. Gemeinsamer I2C-Bus (OLEDs & MCP23017)
| Signal | GPIO (Code) | Board-Label |
| :--- | :--- | :--- |
| **SDA** | 11 | **A4** |
| **SCL** | 12 | **A5** |

## 2. Gemeinsamer SPI-Bus (TFT-Display & Touch)
| Signal | GPIO (Code) | Board-Label |
| :--- | :--- | :--- |
| **SCK** | 48 | **D13** |
| **MOSI** | 38 | **D11** |
| **MISO** | 47 | **D12** |
| **TFT_CS** | 18 | **D9** |
| **TFT_DC** | 21 | **D10** |
| **TOUCH_CS**| 1 | **A0** |

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
| **Volume PWM** | 13 | **A6** |
| **Encoder A** | 5 | **D2** |
| **Encoder B** | 6 | **D3** |
| **Encoder Btn** | 7 | **D4** |
| **PTT Taste** | 4 | **A3** |
| **SWR FWD** | 2 | **A1** |
| **SWR REF** | 3 | **A2** |
| **Mic Gain CS**| 43 | **D1** |

## 5. MCP23017 Ausgänge (Layout-optimiert)
Die Bänder sind von **hoch nach niedrig** sortiert, um dem Hardware-Layout (kürzeste Signalwege für hohe Frequenzen) gerecht zu werden.

| MCP-Pin | Band | Beschreibung |
| :--- | :--- | :--- |
| **0** | **10m** | Höchste Frequenz (Pin 0 = Erstes Relais) |
| **1** | **15m** | Tiefpass-Umschaltung |
| **2** | **20m** | Tiefpass-Umschaltung |
| **3** | **40m** | Tiefpass-Umschaltung |
| **4** | **80m** | Tiefpass-Umschaltung |
| **5** | **160m** | Tiefpass-Umschaltung |
| **6** | PA Bias | PA Ruhestrom Aktivierung |
| **7** | TX/RX Relais | Antennen-Umschaltung |
