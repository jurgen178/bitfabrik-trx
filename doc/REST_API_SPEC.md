# BITFABRIK Transceiver REST API Specification

This document describes the REST API and communication interfaces for the BITFABRIK Transceiver.

## 1. Web Interfaces

| Method | Path | Description |
| :--- | :--- | :--- |
| `GET` | `/` | Main Control Dashboard (HTML) |
| `GET` | `/bands` | Band Configuration Editor (HTML) |

## 2. Configuration API

| Method | Path | Description |
| :--- | :--- | :--- |
| `GET` | `/bands.json` | Retrieves current band configuration in JSON format. |
| `POST` | `/bands.json` | Saves a new band configuration. Triggers ESP restart. |
| `DELETE` | `/bands.json` | Resets band configuration to defaults. Triggers ESP restart. |

### Band Configuration Schema
Die Datei `/bands.json` steuert die Hardware-Filter und Frequenzbereiche.
**Struktur (Array von Objekten):**
```json
[
  {
    "id": "40m",
    "min": 7000000,
    "max": 7200000,
    "def": 7047000,
    "rx_filter": 3,
    "tx_filter": 3,
    "usb": false,
    "active": true
  }
]
```

### Band Management Examples
**Band-Tabelle hochladen (Überschreiben):**
```bash
curl -X POST -H "Content-Type: application/json" --data-binary @my_bands.json "http://192.168.1.110/bands.json"
```

**Band-Tabelle zurücksetzen (löscht /bands.json auf dem ESP):**
```bash
curl -X DELETE "http://192.168.1.110/bands.json"
```

## 3. Real-Time Status & Control

### Status Update
**`GET /api/v1/status`**
Returns the complete system state as a JSON object.

### Device Control
**`GET/POST /api/v1/control`**
Updates hardware parameters. Multiple parameters can be combined in one request.

**Parameters:**
*   `ui_mode`: `RADIO`, `GEN`, `SETTINGS`, `RIT`
*   `freq`: Frequency in Hz (e.g., `7100000`)
*   `vol`: Audio Volume (0-100)
*   `pwr`: PA Drive Power (0-100)
*   `mic`: Mic Gain (0-100)
*   `band`: Band Index (0-5)
*   `mode`: Digital Mode (0=Morse, 1=RTTY)
*   `vfo_set`: Switch VFO (0=A, 1=B)
*   `vfo_copy`: Trigger A=B sync (any value)
*   `rit_enable`: Enable RIT (0=Off, 1=On)
*   `rit_offset`: RIT Offset in Hz
*   `mem_store`: Store current VFO in Memory Slot (0-9)
*   `mem_recall`: Recall VFO from Memory Slot (0-9)
*   `step`: Tuning Step Index
*   `vox_enable`: Toggle/Set VOX state
*   `vox_thresh`: VOX ADC Threshold (0-4095)
*   `vox_delay`: VOX Release Delay in ms

## 4. Messaging & Transmission

### Digital Transmission
**`GET/POST /api/v1/transmit`**
Queues text for digital transmission.
*   `text`: The string to transmit.

### Radio Email (APRS/JS8 Gateway)
**`GET/POST /api/v1/email`**
Sends an email via radio gateway.
*   `to`: Recipient email address.
*   `msg`: Email body (max 67-100 chars).
*   `gw`: Gateway type (`aprs` or `js8`).

## 5. Streaming & Real-Time Events

### WebSocket
**`WS /ws`**
Primary bi-directional channel for real-time operation.
*   **Downlink:** Sends `JSON_STATUS:{...}` and `RX:<char>`
*   **Uplink:** Accepts `SET:<key>:<value>`, `TX:<text>`, and `MAIL|<to>|<msg>|<gw>`

### Server-Sent Events (SSE)
**`GET /api/v1/rx/stream`**
Event stream for decoded RX characters.
*   Event: `rx_char`

## 6. Usage Examples

Replace `192.168.1.110` with your transceiver's actual IP address.

### Control Examples (curl)
**Set frequency to 7.100 MHz:**
```bash
curl "http://192.168.1.110/api/v1/control?freq=7100000"
```

### PowerShell Example (Windows)
PowerShell ist ideal für kleine Desktop-Scripts.
**Frequenz setzen und Status abrufen:**
```powershell
# Frequenz auf 14.250 MHz setzen
Invoke-RestMethod -Uri "http://192.168.1.110/api/v1/control?freq=14250000"

# Aktuellen Status abholen und Frequenz anzeigen
$status = Invoke-RestMethod -Uri "http://192.168.1.110/api/v1/status"
Write-Host "Aktuelle Frequenz: $($status.freq / 1e6) MHz"
```

### Python Example
Ideal für Automatisierung oder Logging. Benötigt die `requests` Bibliothek.
```python
import requests

# Frequenz setzen
params = {'freq': 7047000, 'vol': 40}
r = requests.get("http://192.168.1.110/api/v1/control", params=params)

# Status auslesen
status = requests.get("http://192.168.1.110/api/v1/status").json()
print(f"SWR: {status['swr']} | Power: {status['power_w']}W")
```

### JavaScript / Node.js (Fetch API)
```javascript
// Frequenz ändern
fetch("http://192.168.1.110/api/v1/control?freq=3600000")
  .then(response => console.log("Status: " + response.status));

// Status periodisch abfragen
async function getStatus() {
  const res = await fetch("http://192.168.1.110/api/v1/status");
  const data = await res.json();
  console.log(`RSSI: ${data.rssi} dBm`);
}
```

### Messaging Examples

**Set Volume to 50% and switch to VFO B:**
```bash
curl "http://192.168.1.110/api/v1/control?vol=50&vfo_set=1"
```

**Toggle VOX System:**
```bash
curl "http://192.168.1.110/api/v1/control?vox_enable=1"
```

### Messaging Examples
**Send "CQ CQ DE BITFABRIK" in current digital mode:**
```bash
curl --data "text=CQ CQ DE BITFABRIK" "http://192.168.1.110/api/v1/transmit"
```

**Send a Radio Email via APRS Gateway:**
```bash
curl "http://192.168.1.110/api/v1/email?to=test@example.com&msg=Hello+from+Radio&gw=aprs"
```

### WebSocket Example (JavaScript)
```javascript
const ws = new WebSocket('ws://192.168.1.110/ws');

ws.onmessage = (event) => {
    if (event.data.startsWith('JSON_STATUS:')) {
        const status = JSON.parse(event.data.substring(12));
        console.log("Current Frequency:", status.freq);
    }
};

// Send command via WebSocket
ws.onopen = () => {
    ws.send("SET:freq:14250000");
};
```
