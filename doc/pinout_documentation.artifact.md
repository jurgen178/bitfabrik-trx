# Anschlussplan (Pinout) - Arduino Nano ESP32

Diese Dokumentation zeigt die exakte physische Verbindung zwischen dem **Arduino Nano ESP32** und den Hardware-Komponenten (Display, Touch, MCP23017, DDS).

> [!IMPORTANT]
> Da die Board-Einstellung **"By GPIO number (standard)"** aktiv ist, entsprechen die Nummern im Code den echten ESP32-GPIOs. Die Beschriftungen auf dem Board (D2, A0, etc.) sind zur Orientierung beigefügt.

## 1. Gemeinsamer I2C-Bus (OLEDs & MCP23017)
Alle I2C-Geräte teilen sich diese beiden Leitungen.

| Signal | GPIO (Code) | Board-Label | Ziel-Pin am Modul |
| :--- | :--- | :--- | :--- |
| **SDA** | 13 | **A4 / D18** | SDA / Data |
| **SCL** | 14 | **A5 / D19** | SCL / Clock |

*Hinweis: Der MCP23017 benötigt für die Adresse 0x27 alle drei DIP-Schalter (A0, A1, A2) auf ON.*

---

## 2. Gemeinsamer SPI-Bus (TFT-Display & Touch)
Beide Module teilen sich SCK, MOSI und MISO, haben aber eigene CS-Leitungen.

| Signal | GPIO (Code) | Board-Label | Ziel-Pin (TFT / Touch) |
| :--- | :--- | :--- | :--- |
| **SCK** | 48 | **D13** | T_CLK / CLK |
| **MOSI** | 38 | **D11** | T_DIN / SDI |
| **MISO** | 47 | **D12** | T_DO / SDO |
| **TFT_CS** | 18 | **D9** | CS (Display) |
| **TFT_DC** | 21 | **D10** | DC / RS (Display) |
| **TFT_RST** | -1 | - | (Nicht verbunden / Reset am Modul an 3.3V) |
| **TOUCH_CS**| 1 | **A0** | T_CS (Touch) |

---

## 3. DDS Module (AD9850)

### LO (VFO) - DDS #1
| Signal | GPIO (Code) | Board-Label |
| :--- | :--- | :--- |
| **DATA** | 8 | **D5** |
| **WCLK** | 9 | **D6** |
| **FQUD** | 10 | **D7** |
| **RESET**| 17 | **D8** |

### BFO (Mode) - DDS #2
| Signal | GPIO (Code) | Board-Label |
| :--- | :--- | :--- |
| **DATA** | 13 | **A6** |
| **WCLK** | 14 | **A7** |
| **FQUD** | 44 | **D0** |
| **RESET**| 43 | **D1** |

---

## 4. Bedienelemente & Sensorik

| Funktion | GPIO (Code) | Board-Label |
| :--- | :--- | :--- |
| **Encoder A** | 5 | **D2** |
| **Encoder B** | 6 | **D3** |
| **Encoder Button** | 7 | **D4** |
| **PTT Taste** | 4 | **A3** |
| **SWR Vorlauf (FWD)** | 2 | **A1** |
| **SWR Rücklauf (REF)** | 3 | **A2** |

---

## 5. MCP23017 Ausgänge (Relais-Steuerung)
Diese Pins befinden sich direkt am MCP23017 Chip (nicht am Arduino).

| MCP-Pin | Funktion |
| :--- | :--- |
| **0** | Relais 80m |
| **1** | Relais 40m |
| **2** | Relais 20m |
| **3** | Relais 15m |
| **4** | Relais 10m |
| **6** | PA Bias (Sende-Verstärker) |
| **7** | Haupt-Antennenrelais (TX/RX) |
