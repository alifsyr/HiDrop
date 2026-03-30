# Hydroponic Monitoring and Auto-Dosing System

ESP32-based hydroponic controller for monitoring water quality and running basic automatic dosing.

## Bahasa Indonesia

### Ringkasan

Project ini adalah sistem hidroponik berbasis ESP32 yang memantau `TDS`, `pH`, dan `suhu air`, lalu menampilkan data ke LCD 20x4 dan Serial Monitor. Sistem juga mendukung jam realtime `WIB` dari Wi-Fi/NTP, target `pH` dan `PPM` yang bisa diubah saat runtime, serta auto dosing dasar untuk nutrisi dan koreksi pH.

### Fitur Utama

- Monitoring `TDS`, `pH`, dan `temperature`.
- LCD 20x4 dengan layout split:
  sensor di kiri, nama farm + tanggal/jam di kanan.
- Running text untuk area kanan jika teks tidak muat.
- Web dashboard bawaan dari ESP32 untuk monitoring lewat browser.
- Endpoint JSON `/api/status` untuk integrasi monitoring lain.
- Sinkronisasi waktu `WIB` melalui Wi-Fi dan NTP.
- Status boot di LCD saat Wi-Fi belum tersambung.
- Target `pH` dan `PPM` bisa diubah lewat Serial Monitor dan disimpan ke EEPROM.
- Notifikasi perubahan target langsung tampil di LCD.
- Auto dosing:
  - `PPM` rendah -> pompa `Nutrisi A` dan `Nutrisi B` nyala bersamaan.
  - `pH` tinggi -> pompa `pH Down`.
  - `pH` rendah -> pompa `pH Up`.
- Logging event dosing ke Google Sheets sudah disiapkan, tetapi default-nya masih nonaktif sampai URL Apps Script diisi.

### Target Default

- `pH`: `5.8 - 6.2`
- `PPM`: `600 - 800`

### Hardware dan Pin

| Fungsi          | GPIO   |
| --------------- | ------ |
| TDS Sensor      | `35` |
| pH Sensor       | `34` |
| DS18B20         | `4`  |
| I2C SDA         | `21` |
| I2C SCL         | `22` |
| Relay Nutrisi A | `25` |
| Relay Nutrisi B | `26` |
| Relay pH Down   | `27` |
| Relay pH Up     | `33` |

### Alur Auto Dosing

- Sistem membaca sensor secara periodik.
- Jika `PPM < target minimum`, relay `Nutrisi A` dan `Nutrisi B` aktif bersamaan dengan rasio waktu yang sama.
- Jika `pH > target maksimum`, relay `pH Down` aktif.
- Jika `pH < target minimum`, relay `pH Up` aktif.
- Setelah dosing, sistem menunggu waktu mixing lalu membaca ulang sensor.
- Jika `PPM > target maksimum`, sistem tidak mengencerkan otomatis.
  Pengenceran air masih manual.

### Status LCD

- `NORMAL` -> tidak ada dosing aktif.
- `NUTRI A+B` -> dosing nutrisi sedang berjalan atau menunggu recheck nutrisi.
- `PH ↓ DOSE` -> dosing pH down sedang berjalan atau menunggu recheck.
- `PH ↑ DOSE` -> dosing pH up sedang berjalan atau menunggu recheck.

### Web Dashboard

- Dashboard lokal tersedia di `http://IP_ESP32/`
- API live tersedia di `http://IP_ESP32/api/status`
- Menampilkan `pH`, `PPM`, suhu, status Wi-Fi, waktu lokal, mode sensor, mode dosing, target range, dan histori singkat dosing terakhir.
- Saat ESP32 berhasil connect Wi-Fi, alamat dashboard akan dicetak ke Serial Monitor.

Contoh respons API:

```json
{
  "device": {
    "wifi_connected": true,
    "ip_address": "192.168.1.50",
    "date": "2026-03-30",
    "time": "15:12:08"
  },
  "sensor": {
    "temperature_c": 25.12,
    "ppm": 712,
    "ph": 6.01,
    "mode": "MONITOR"
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

### Command Serial

Pengaturan target:

- `SET PH <min> <max>`
- `SET PPM <min> <max>`
- `SHOW TARGETS`
- `RESET TARGETS`

Kalibrasi TDS:

- `ENTER`
- `EXIT`
- command lain diteruskan ke library `GravityTDS`

### Build dan Upload

Sebelum build, buat file `.env` atau `.env.local` dari `.env.example`, lalu isi credential pribadi Anda.

Contoh:

```env
WIFI_SSID=your_wifi_ssid
WIFI_PASSWORD=your_wifi_password
GOOGLE_SHEETS_LOGGING_ENABLED=false
GOOGLE_SHEETS_WEB_APP_URL=
GOOGLE_SHEETS_SHARED_SECRET=
```

```bash
pio run
pio run -t upload
pio device monitor -b 115200
```

### Catatan Penting

- Credential penting sekarang dibaca dari `.env` atau `.env.local` melalui `scripts/load_env.py`.
- File `.env` dan `.env.local` sudah di-ignore oleh Git agar tidak ikut ter-push.
- Dashboard ESP32 adalah HTTP lokal. Untuk akses lewat internet, sebaiknya gunakan `VPN`, `Cloudflare Tunnel`, reverse proxy HTTPS, atau port forwarding yang diamankan.
- Nilai flow pump masih default dan perlu dikalibrasi di hardware asli.
- Logging Google Sheets belum aktif sampai `GOOGLE_SHEETS_WEB_APP_URL` diisi.
- Jika credential lama sudah pernah ter-commit, sebaiknya lakukan rotasi credential tersebut.

---

## English

### Overview

This project is an ESP32-based hydroponic system that monitors `TDS`, `pH`, and `water temperature`, then shows the data on a 20x4 LCD and the serial monitor. It also supports real-time `WIB` clock sync over Wi-Fi/NTP, runtime-configurable `pH` and `PPM` targets, and basic automatic dosing for nutrients and pH correction.

### Key Features

- `TDS`, `pH`, and temperature monitoring.
- 20x4 LCD split layout:
  sensor data on the left, farm name + date/time on the right.
- Running text for right-side content when it exceeds the display width.
- Built-in ESP32 web dashboard for browser-based monitoring.
- JSON endpoint at `/api/status` for external monitoring integrations.
- `WIB` time synchronization via Wi-Fi and NTP.
- LCD boot status while Wi-Fi is connecting.
- Runtime `pH` and `PPM` target updates through Serial Monitor, stored in EEPROM.
- LCD notification whenever targets are changed.
- Auto dosing:
  - low `PPM` -> `Nutrient A` and `Nutrient B` pumps run together.
  - high `pH` -> `pH Down` pump runs.
  - low `pH` -> `pH Up` pump runs.
- Google Sheets event logging is prepared, but disabled by default until the Apps Script URL is configured.

### Default Targets

- `pH`: `5.8 - 6.2`
- `PPM`: `600 - 800`

### Hardware and Pins

| Function         | GPIO   |
| ---------------- | ------ |
| TDS Sensor       | `35` |
| pH Sensor        | `34` |
| DS18B20          | `4`  |
| I2C SDA          | `21` |
| I2C SCL          | `22` |
| Nutrient A Relay | `25` |
| Nutrient B Relay | `26` |
| pH Down Relay    | `27` |
| pH Up Relay      | `33` |

### Auto-Dosing Flow

- The system reads sensors periodically.
- If `PPM < minimum target`, `Nutrient A` and `Nutrient B` relays run at the same time with the same dosing time ratio.
- If `pH > maximum target`, the `pH Down` relay runs.
- If `pH < minimum target`, the `pH Up` relay runs.
- After dosing, the system waits for mixing and then rechecks the sensors.
- If `PPM > maximum target`, dilution is not automatic yet.
  Water dilution is still manual.

### LCD Status

- `NORMAL` -> no active dosing.
- `NUTRI A+B` -> nutrient dosing is active or waiting for nutrient recheck.
- `PH ↓ DOSE` -> pH down dosing is active or waiting for recheck.
- `PH ↑ DOSE` -> pH up dosing is active or waiting for recheck.

### Web Dashboard

- Local dashboard is available at `http://ESP32_IP/`
- Live API is available at `http://ESP32_IP/api/status`
- Shows `pH`, `PPM`, temperature, Wi-Fi status, local time, sensor mode, dosing mode, target ranges, and a short recent dosing history.
- Once Wi-Fi is connected, the device prints the dashboard address to the serial monitor.

### Serial Commands

Target configuration:

- `SET PH <min> <max>`
- `SET PPM <min> <max>`
- `SHOW TARGETS`
- `RESET TARGETS`

TDS calibration:

- `ENTER`
- `EXIT`
- other commands are forwarded to the `GravityTDS` library

### Build and Upload

Before building, create a `.env` or `.env.local` file from `.env.example`, then fill in your private credentials.

Example:

```env
WIFI_SSID=your_wifi_ssid
WIFI_PASSWORD=your_wifi_password
GOOGLE_SHEETS_LOGGING_ENABLED=false
GOOGLE_SHEETS_WEB_APP_URL=
GOOGLE_SHEETS_SHARED_SECRET=
```

```bash
pio run
pio run -t upload
pio device monitor -b 115200
```

### Important Notes

- Sensitive credentials are now loaded from `.env` or `.env.local` through `scripts/load_env.py`.
- `.env` and `.env.local` are ignored by Git so they are not pushed to the public repository.
- The built-in dashboard is plain HTTP on the local network. For internet access, prefer a secure VPN, tunnel, HTTPS reverse proxy, or carefully secured port forwarding.
- Pump flow values are still defaults and should be calibrated on real hardware.
- Google Sheets logging stays disabled until `GOOGLE_SHEETS_WEB_APP_URL` is configured.
- If old credentials were already committed before, they should be rotated.
