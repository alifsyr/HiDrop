<div align="center">

# 💧 HiDrop — Smart Hydroponic Controller

**ESP32-based IoT hydroponic system with real-time monitoring, auto-dosing, Firebase integration, and OTA firmware updates.**

[![Build and Release](https://github.com/alifsyr/HiDrop/actions/workflows/release.yml/badge.svg)](https://github.com/alifsyr/HiDrop/actions/workflows/release.yml)
[![PlatformIO](https://img.shields.io/badge/Built%20with-PlatformIO-orange?logo=platformio)](https://platformio.org)
[![ESP32](https://img.shields.io/badge/Board-ESP32-blue?logo=espressif)](https://www.espressif.com/en/products/socs/esp32)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

![Dashboard Preview](https://raw.githubusercontent.com/alifsyr/HiDrop/master/image/README/1774870733653.png)

[Features](#-features) · [Hardware](#-hardware--wiring) · [Getting Started](#-getting-started) · [API Reference](#-api-reference) · [Serial Commands](#-serial-commands) · [Architecture](#-architecture)

</div>

---

## ✨ Features

| Category | Details |
|---|---|
| **Sensor Monitoring** | TDS (PPM), pH, and water temperature via DS18B20 |
| **Auto-Dosing** | Automatic nutrient A+B, pH Up/Down pump control |
| **Display** | 20×4 LCD with split layout — sensors left, farm info right |
| **Web Dashboard** | Local web UI at `http://ESP32_IP/` with live charts |
| **REST API** | JSON endpoints for status, history, and dosing reports |
| **Firebase** | Real-time sync, remote command, and OTA trigger via RTDB |
| **OTA Updates** | Firmware update via GitHub Releases or Firebase-triggered URL |
| **WebSerial** | Browser-based serial monitor and command interface |
| **NTP Clock** | WIB (UTC+7) time sync over Wi-Fi |
| **EEPROM Persistence** | pH and PPM targets survive reboots |
| **Google Sheets Logging** | Dosing event logging via Apps Script (optional) |
| **Dev Mode** | Safe development mode with auto-dosing disabled |

---

## 🔧 Hardware & Wiring

### Bill of Materials

| Component | Description |
|---|---|
| ESP32 DevKit | Main microcontroller |
| TDS Sensor | Analog gravity-type TDS sensor |
| pH Sensor | Analog pH probe with transmitter |
| DS18B20 | 1-Wire waterproof temperature sensor |
| LCD 20×4 I2C | Display module (I2C, addr `0x27`) |
| 4-Channel Relay | Controls 4 peristaltic pumps |
| Peristaltic Pumps ×4 | Nutrient A, Nutrient B, pH Down, pH Up |

### GPIO Pin Map

| Function | GPIO |
|---|---|
| TDS Sensor (Analog) | `35` |
| pH Sensor (Analog) | `34` |
| DS18B20 Temperature | `4` |
| I2C SDA (LCD) | `21` |
| I2C SCL (LCD) | `22` |
| Relay — Nutrient A | `25` |
| Relay — Nutrient B | `26` |
| Relay — pH Down | `27` |
| Relay — pH Up | `33` |

---

## 🚀 Getting Started

### Prerequisites

- [PlatformIO IDE](https://platformio.org/install/ide?install=vscode) (VS Code extension) or PlatformIO Core CLI
- Python 3.x (required by PlatformIO)
- ESP32 board and all hardware components above

### 1. Clone the Repository

```bash
git clone https://github.com/alifsyr/HiDrop.git
cd HiDrop
```

### 2. Configure Environment Variables

Copy the example file and fill in your credentials:

```bash
cp .env.example .env
```

Edit `.env`:

```env
# Wi-Fi Credentials
WIFI_SSID=your_wifi_ssid
WIFI_PASSWORD=your_wifi_password

# Google Sheets Logging (optional)
GOOGLE_SHEETS_LOGGING_ENABLED=false
GOOGLE_SHEETS_WEB_APP_URL=
GOOGLE_SHEETS_SHARED_SECRET=
```

> **Important:** `.env` is in `.gitignore` and will **never** be committed. Do not rename it or commit credentials.

### 3. Build and Flash

```bash
# Build firmware
pio run

# Flash to ESP32 (connect via USB)
pio run -t upload

# Open serial monitor
pio device monitor -b 115200
```

After a successful Wi-Fi connection, the IP address of the dashboard is printed to the serial monitor:

```
WiFi Connected! IP Address: 192.168.1.50
```

Open `http://192.168.1.50/` in a browser to access the dashboard.

---

## 📊 Default Targets

| Parameter | Default Range |
|---|---|
| pH | `5.8 – 6.2` |
| PPM (TDS) | `600 – 800` |

Targets can be changed at runtime via serial commands or WebSerial and are persisted to EEPROM.

---

## 🤖 Auto-Dosing Logic

The dosing controller reads sensors periodically and acts on these rules:

```
PPM < target_min  →  Nutrient A + B pumps run simultaneously
pH  > target_max  →  pH Down pump runs
pH  < target_min  →  pH Up pump runs
PPM > target_max  →  No automatic dilution (manual water change required)
```

After each dosing cycle, the system waits for a configurable **mixing delay** before rechecking sensors.

### LCD Status Codes

| LCD Display | Meaning |
|---|---|
| `NORMAL` | Monitoring only, no active dosing |
| `NUTRI A+B` | Nutrient dosing active or awaiting recheck |
| `PH ↓ DOSE` | pH Down dosing active or awaiting recheck |
| `PH ↑ DOSE` | pH Up dosing active or awaiting recheck |

---

## 🌐 Web Dashboard & API Reference

The embedded web dashboard is served directly from the ESP32.

**Dashboard:** `http://ESP32_IP/`

### Endpoints

| Endpoint | Method | Description |
|---|---|---|
| `/api/status` | GET | Live device, sensor, target, and dosing snapshot |
| `/api/history` | GET | Historical pH and PPM chart data samples |
| `/api/reports` | GET | Latest dosing event log |
| `/webserial` | GET | Browser-based serial monitor (WebSerial UI) |

### `GET /api/status` — Example Response

```json
{
  "device": {
    "wifi_connected": true,
    "ip_address": "192.168.1.50",
    "time_valid": true,
    "date": "2026-03-30",
    "time": "15:12:08",
    "uptime_seconds": 5432
  },
  "sensor": {
    "temperature_c": 25.12,
    "ppm": 712,
    "ph_voltage": 1.324,
    "ph": 6.01,
    "mode": "MONITOR",
    "calibration_mode": false
  },
  "targets": {
    "ph_min": 5.8,
    "ph_max": 6.2,
    "ppm_min": 600,
    "ppm_max": 800
  },
  "dosing": {
    "busy": false,
    "state": "Monitoring",
    "display_mode": "NORMAL"
  }
}
```

---

## 💬 Serial Commands

Commands can be sent via USB Serial Monitor (115200 baud) or through the **WebSerial** browser interface at `http://ESP32_IP/webserial`.

### Target Configuration

| Command | Description |
|---|---|
| `SET PH <min> <max>` | Set pH target range, e.g. `SET PH 5.8 6.2` |
| `SET PPM <min> <max>` | Set PPM target range, e.g. `SET PPM 600 800` |
| `SHOW TARGETS` | Print current targets to serial |
| `RESET TARGETS` | Restore default targets |

### Manual Dosing

| Command | Description |
|---|---|
| `DOSE NUTRI` | Trigger a manual nutrient A+B dosing cycle |
| `DOSE PH DOWN` | Trigger a manual pH Down dosing cycle |
| `DOSE PH UP` | Trigger a manual pH Up dosing cycle |

### TDS Calibration

| Command | Description |
|---|---|
| `ENTER` | Enter TDS calibration mode |
| `EXIT` | Exit TDS calibration mode |
| *(other)* | Forwarded to the `GravityTDS` library calibration handler |

### System

| Command | Description |
|---|---|
| `RESET WIFI` | Clear Wi-Fi settings and reboot (triggers Wi-Fi Manager captive portal) |
| `DEV STATUS` | Print dev mode info (only in DEV_MODE builds) |

---

## ☁️ Firebase Integration

HiDrop supports two-way communication with Firebase Realtime Database:

- **Telemetry push** — sensor readings, dosing state, and uptime are continuously synced to RTDB.
- **Remote commands** — target range changes (`SET PH`, `SET PPM`) can be pushed from the cloud.
- **OTA trigger** — a firmware update URL and version tag written to RTDB triggers the ESP32 to self-update over-the-air.

### OTA Firmware Update Flow

```
GitHub Push → CI Build → GitHub Release (.bin) → Firebase RTDB ← ESP32 polls
                                                         ↓
                                               ESP32 downloads & flashes
```

The CI/CD pipeline (`.github/workflows/release.yml`) automatically builds and publishes a release binary on every push to `main`/`master`. The binary version is tagged as `v1.0.<run_number>`.

---

## 🏗️ Architecture

```
src/
└── main.cpp                  # Entry point, setup & loop

include/
├── actuators/                # Relay / pump control
├── config/                   # App constants, pin definitions
├── control/                  # Dosing controller, sensor manager, target manager
├── display/                  # LCD display rendering
├── models/                   # Data structures (SensorData, DosingReport)
├── network/                  # WiFi/NTP, Firebase, Google Sheets, OTA
├── sensors/                  # TDS, pH, temperature sensor wrappers
└── utils/                    # Logger and shared utilities
```

### Key Libraries

| Library | Purpose |
|---|---|
| `ESPAsyncWebServer` | Async HTTP server for dashboard and API |
| `WebSerial` | Browser-based serial terminal |
| `Firebase Arduino Client` | Firebase RTDB client |
| `DallasTemperature` / `OneWire` | DS18B20 temperature sensor |
| `LiquidCrystal_I2C` | I2C LCD display |
| `ArduinoJson` | JSON serialization |
| `ESPAsyncWiFiManager` | Captive portal for Wi-Fi provisioning |

---

## 🔁 CI/CD

Every push to `main` or `master` triggers the GitHub Actions workflow:

1. **Build** — PlatformIO compiles the firmware with version `1.0.<run_number>`
2. **Release** — A GitHub Release is created and `firmware.bin` is attached as an asset
3. **OTA** — The release binary can be referenced via Firebase to trigger remote device updates

---

## 🤝 Contributing

Contributions are welcome! Please open an issue or pull request.

1. Fork the repository
2. Create your feature branch: `git checkout -b feat/your-feature`
3. Commit your changes: `git commit -m 'feat: add your feature'`
4. Push to the branch: `git push origin feat/your-feature`
5. Open a Pull Request

---

## 📄 License

This project is licensed under the [MIT License](LICENSE).

---

<div align="center">
Made with ❤️ for smarter farming · <a href="http://alif-tech.my.id/">alif-tech.my.id</a>
</div>
