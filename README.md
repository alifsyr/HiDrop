# Hydroponic Monitoring System (ESP32)

Project ini adalah sistem monitoring hidroponik berbasis ESP32 yang membaca tiga parameter utama air:

- `TDS` untuk konsentrasi nutrisi
- `pH` untuk tingkat keasaman
- `Temperature` untuk suhu air

Data sensor ditampilkan di LCD I2C 20x4, dikirim ke Serial Monitor, dan dilengkapi jam realtime WIB yang disinkronkan lewat Wi-Fi/NTP.

README ini menjelaskan arsitektur proyek, hardware yang dibutuhkan, wiring, alur kerja program, konfigurasi, dan cara menjalankan proyek dari awal sampai monitoring.

## 1. Cakupan Sistem Saat Ini

Saat ini repo ini berfokus pada:

- pembacaan sensor TDS
- pembacaan sensor pH
- pembacaan suhu air dengan DS18B20
- tampilan LCD monitoring
- sinkronisasi tanggal dan jam realtime WIB
- pemilihan mode tampilan melalui Serial Monitor
- status boot LCD saat Wi-Fi belum terhubung

Yang belum ada di repo ini:

- kontrol pompa
- kontrol relay
- dosing otomatis nutrisi
- closed-loop control berbasis nilai sensor
- unit test aktif di folder `test/`

Catatan penting:

- Mode seperti `PH DOWN CAL`, `PH UP CAL`, `NUTRI A`, dan `NUTRI B` saat ini adalah mode tampilan pada LCD.
- Mode tersebut belum mengubah logika dosing atau aktuator.
- Kalibrasi yang benar-benar diteruskan ke library saat ini adalah kalibrasi TDS melalui command serial.

## 2. Fitur Utama

- Monitoring `TDS`, `pH`, dan suhu air.
- LCD 20x4 dengan layout dua area:
  - sisi kiri untuk data sensor
  - sisi kanan untuk nama farm, tanggal, dan jam
- Running text pada sisi kanan jika teks lebih panjang dari lebar area LCD.
- Pembatas visual antara area sensor dan area informasi.
- Status boot:
  - `Initializing...` saat Wi-Fi belum terhubung
  - `Initializing Finish` setelah Wi-Fi berhasil terhubung
- Sinkronisasi waktu realtime WIB lewat Wi-Fi dan NTP.
- Auto reconnect Wi-Fi bila koneksi terputus.
- Logging data sensor ke Serial Monitor.
- Pengurangan noise pembacaan TDS dengan exponential smoothing.
- Penjadwalan pembacaan sensor untuk mengurangi interferensi TDS dan pH.

## 3. Hardware yang Dibutuhkan

Komponen minimum yang dibutuhkan:

| Komponen | Fungsi |
| --- | --- |
| ESP32 Dev Board | Mikrokontroler utama |
| Gravity TDS Sensor | Pembacaan TDS nutrisi |
| pH Sensor Module analog | Pembacaan pH |
| DS18B20 waterproof | Pembacaan suhu air |
| LCD 20x4 I2C | Tampilan lokal |
| Akses Wi-Fi | Sinkronisasi tanggal dan jam WIB |
| Catu daya stabil | Menyalakan ESP32 dan sensor |

Catatan hardware:

- Semua ground sensor dan ESP32 harus tersambung bersama.
- DS18B20 umumnya membutuhkan resistor pull-up 4.7k antara `DATA` dan `VCC`.
- Pastikan output analog sensor TDS dan pH sesuai rentang ADC ESP32.
- LCD diasumsikan memakai alamat I2C `0x27`.

## 4. Pin Mapping

Mapping pin di proyek ini didefinisikan di `include/config/pins.h`.

| Fungsi | GPIO ESP32 |
| --- | --- |
| TDS Sensor | `35` |
| pH Sensor | `34` |
| DS18B20 | `4` |
| I2C SDA | `21` |
| I2C SCL | `22` |

## 5. Stack Software

Environment build menggunakan PlatformIO dengan board `esp32dev`.

Framework dan dependency utama:

- Arduino framework untuk ESP32
- `GravityTDS`
- `OneWire`
- `DallasTemperature`
- `LiquidCrystal_I2C`

Konfigurasi PlatformIO ada di `platformio.ini`.

## 6. Struktur Project

Struktur folder utama:

```text
.
├── include/
│   ├── config/
│   ├── control/
│   ├── display/
│   ├── models/
│   ├── network/
│   └── sensors/
├── src/
│   ├── control/
│   ├── display/
│   ├── network/
│   └── sensors/
├── lib/
├── test/
└── platformio.ini
```

Penjelasan singkat:

- `src/main.cpp`
  - entry point aplikasi
  - inisialisasi sensor, LCD, Wi-Fi, dan loop utama
- `src/control/`
  - koordinasi pembacaan sensor
  - penjadwalan TDS dan pH
- `src/sensors/`
  - implementasi driver pembacaan sensor
- `src/display/`
  - pengaturan layout LCD
  - boot screen
  - running text
- `src/network/`
  - koneksi Wi-Fi
  - sinkronisasi waktu NTP
- `include/models/`
  - model data sensor dan mode tampilan

## 7. Alur Kerja Program

### 7.1 Boot Sequence

Saat ESP32 menyala:

1. Serial diinisialisasi pada `115200 baud`.
2. EEPROM diinisialisasi.
3. Bus I2C diinisialisasi.
4. Sensor manager, LCD, dan Wi-Fi clock dijalankan.
5. LCD menampilkan layar `Initializing...` sampai Wi-Fi terhubung.
6. Setelah Wi-Fi terhubung, LCD menampilkan `Initializing Finish` selama beberapa detik.
7. Sistem masuk ke layar monitoring utama.

### 7.2 Loop Utama

Di dalam `loop()`:

1. Membaca command dari serial.
2. Memproses perubahan mode tampilan bila ada.
3. Memproses command kalibrasi bila command bukan mode tampilan.
4. Update sensor manager.
5. Update status Wi-Fi dan NTP.
6. Cetak log sensor ke serial secara periodik.
7. Refresh LCD secara periodik.

## 8. Cara Kerja Sensor

### 8.1 Temperature Sensor (DS18B20)

Implementasi ada di `src/sensors/temp_sensor.cpp`.

Karakteristik:

- menggunakan `DallasTemperature`
- pembacaan non-blocking
- resolusi sensor diset ke `10-bit`
- ada fallback temperature default jika sensor tidak valid

Alur kerjanya:

- sensor meminta konversi suhu
- program menunggu delay konversi sesuai config
- suhu diambil dengan `getTempCByIndex(0)`
- bila data tidak valid, sistem memakai fallback temperature

### 8.2 TDS Sensor

Implementasi ada di `src/sensors/tds_sensor.cpp`.

Karakteristik:

- menggunakan library `GravityTDS`
- pembacaan dikompensasi berdasarkan suhu air
- hasil dibersihkan dengan exponential smoothing

Rumus smoothing:

```text
filtered = (alpha * raw) + ((1 - alpha) * previous)
```

Default `alpha` saat ini adalah `0.15`.

### 8.3 pH Sensor

Implementasi ada di `src/sensors/ph_sensor.cpp`.

Karakteristik:

- pembacaan analog dengan ADC 12-bit
- mengambil beberapa sampel
- data diurutkan
- dua nilai terendah dan dua nilai tertinggi dibuang
- sisanya dirata-ratakan

Konversi tegangan ke pH:

```text
pH = (slope * voltage) + calibrationValue
```

Nilai default:

- `slope = -5.70`
- `calibrationValue = 21.34 - 0.7`

Nilai pH di-clamp ke rentang `0.0 - 14.0`.

## 9. Penjadwalan Sensor

Penjadwalan sensor dikelola oleh `SensorManager`.

Tujuannya adalah mengurangi potensi interferensi pembacaan antara TDS dan pH.

Urutan logikanya:

- suhu diperbarui secara terus-menerus
- TDS diperbarui tiap `1000 ms`
- setelah TDS dibaca, sistem menunggu quiet window `500 ms`
- pH diperbarui tiap `1500 ms` saat quiet window sudah lewat

Mode internal `SensorManager`:

- `IDLE`
- `TDS_READ`
- `PH_QUIET`
- `PH_READ`
- `CALIBRATION`

## 10. Tampilan LCD

LCD menggunakan modul I2C 20x4.

### 10.1 Layar Boot

Sebelum Wi-Fi terhubung:

```text
Hydroponic System
Initializing...
WiFi Connecting
Please Wait
```

Setelah Wi-Fi terhubung:

```text
Hydroponic System
Initializing
Finish
WiFi Connected
```

### 10.2 Layout Monitoring

Layout monitoring dibagi menjadi dua area:

- sisi kiri untuk pembacaan sensor
- sisi kanan untuk info farm dan waktu

Contoh tampilan:

```text
Temp:27.3C |Hasna's
TDS:518ppm |Sun 29 M
pH:6.21    |13:05:04
Mode:NORMAL
```

Catatan:

- `|` adalah pembatas antara area kiri dan kanan
- bagian kanan menggunakan running text bila teks terlalu panjang
- baris mode menggunakan lebar penuh LCD

### 10.3 Informasi yang Ditampilkan

Sisi kiri:

- `Temp`
- `TDS`
- `pH`
- `Mode`

Sisi kanan:

- nama farm: `Hasna's Farm`
- tanggal realtime WIB
- jam realtime WIB

## 11. Wi-Fi dan Sinkronisasi Waktu

Fitur ini diimplementasikan di `src/network/wifi_clock.cpp`.

Perilaku sistem:

- ESP32 masuk ke mode station
- mencoba connect ke Wi-Fi saat startup
- auto reconnect tiap `10000 ms` bila koneksi hilang
- setelah konek, waktu disinkronkan via NTP
- timezone menggunakan `UTC+7` untuk WIB

Server NTP default:

- `pool.ntp.org`
- `time.nist.gov`

Catatan keamanan:

- kredensial Wi-Fi saat ini disimpan langsung di source code
- ubah `WIFI_SSID` dan `WIFI_PASSWORD` di `include/config/app_config.h` sebelum deploy ke environment lain

## 12. Mode Tampilan

Mode tampilan didefinisikan di `include/models/display_mode.h`.

Daftar mode:

- `NORMAL`
- `PH_DOWN_CAL`
- `PH_UP_CAL`
- `NUTRI_A`
- `NUTRI_B`

Mode ini memengaruhi label yang tampil pada LCD, misalnya:

- `Mode:NORMAL`
- `Mode:PH ↓ CAL`
- `Mode:PH ↑ CAL`
- `Mode:NUTRI A`
- `Mode:NUTRI B`

## 13. Command Serial

### 13.1 Mengubah Mode Tampilan

Mode tampilan bisa diubah dari Serial Monitor.

Command yang didukung:

| Tujuan | Command yang diterima |
| --- | --- |
| Mode normal | `1`, `MODE1`, `NORMAL`, `MODE NORMAL` |
| Mode pH down cal | `2`, `MODE2`, `PHDOWNCAL`, `PHBAWAHCAL` |
| Mode pH up cal | `3`, `MODE3`, `PHUPCAL`, `PHATASCAL` |
| Mode nutrisi A | `4`, `MODE4`, `NUTRIA` |
| Mode nutrisi B | `5`, `MODE5`, `NUTRIB` |

Catatan:

- command tidak sensitif huruf besar/kecil
- spasi, `_`, dan `-` diabaikan untuk parsing mode

### 13.2 Kalibrasi TDS

Command serial yang bukan mode tampilan akan diteruskan ke modul kalibrasi TDS.

Yang ditangani langsung oleh aplikasi:

- `ENTER` untuk masuk mode kalibrasi
- `EXIT` untuk keluar mode kalibrasi

Command lain diteruskan ke library `GravityTDS`.

Catatan:

- sintaks detail command kalibrasi TDS mengikuti library `GravityTDS`
- EEPROM diinisialisasi pada startup agar kalibrasi dapat digunakan oleh library terkait

## 14. Konfigurasi Penting

Semua konfigurasi utama ada di `include/config/app_config.h`.

Parameter yang paling sering diubah:

| Parameter | Fungsi |
| --- | --- |
| `ADC_VREF` | Tegangan referensi ADC |
| `ADC_RANGE` | Rentang ADC ESP32 |
| `DEFAULT_WATER_TEMPERATURE_C` | Fallback suhu air |
| `PH_SAMPLE_COUNT` | Jumlah sampel pH |
| `TDS_SMOOTHING_ALPHA` | Intensitas smoothing TDS |
| `PH_SLOPE` | Slope konversi pH |
| `PH_CALIBRATION_VALUE` | Offset kalibrasi pH |
| `TDS_UPDATE_INTERVAL_MS` | Interval baca TDS |
| `PH_UPDATE_INTERVAL_MS` | Interval baca pH |
| `PH_QUIET_AFTER_TDS_MS` | Delay pH setelah TDS |
| `LCD_REFRESH_INTERVAL_MS` | Refresh LCD |
| `LCD_SCROLL_INTERVAL_MS` | Kecepatan running text |
| `LCD_INIT_FINISH_DURATION_MS` | Durasi layar finish |
| `WIFI_RECONNECT_INTERVAL_MS` | Interval reconnect Wi-Fi |

## 15. Cara Build dan Upload

### 15.1 Menggunakan PlatformIO CLI

Build:

```bash
pio run
```

Upload ke board:

```bash
pio run -t upload
```

Buka Serial Monitor:

```bash
pio device monitor -b 115200
```

Jika command `pio` belum ada di PATH, Anda bisa:

- memakai PlatformIO extension di VS Code
- atau memanggil binary PlatformIO dari environment lokal Anda

### 15.2 Workflow yang Disarankan

```bash
pio run
pio run -t upload
pio device monitor -b 115200
```

## 16. Output Serial

Saat tidak berada di mode kalibrasi, serial akan mencetak log seperti ini:

```text
Temp: 27.34 C | TDS: 518 ppm | pH: 6.21
```

Serial juga akan mencetak perubahan mode tampilan bila command mode diterima.

## 17. File yang Perlu Diperhatikan Saat Mengembangkan

Kalau ingin mengubah bagian tertentu, biasanya file yang relevan adalah:

| Kebutuhan | File utama |
| --- | --- |
| Ubah pin | `include/config/pins.h` |
| Ubah konstanta sistem | `include/config/app_config.h` |
| Ubah alur sensor | `src/control/sensor_manager.cpp` |
| Ubah layout LCD | `src/display/lcd_display.cpp` |
| Ubah koneksi Wi-Fi/NTP | `src/network/wifi_clock.cpp` |
| Ubah parsing mode tampilan | `src/main.cpp` |

## 18. Batasan dan Catatan Pengembangan

Batasan saat ini:

- mode display belum terhubung ke aktuator nyata
- kredensial Wi-Fi masih hardcoded
- belum ada test otomatis
- belum ada pemisahan config khusus environment development dan production

Saran pengembangan berikutnya:

- pindahkan kredensial Wi-Fi ke file config lokal yang tidak di-commit
- tambahkan kalibrasi pH yang lebih formal
- tambahkan kontrol relay/pompa
- tambahkan logging ke SD card atau cloud
- tambahkan test untuk parsing mode dan formatting display

## 19. Ringkasan Singkat

Repo ini adalah fondasi sistem monitoring hidroponik berbasis ESP32 yang:

- membaca TDS, pH, dan suhu
- menampilkan data ke LCD 20x4
- menjaga tampilan lokal tetap informatif
- sinkron ke WIB lewat Wi-Fi
- siap dikembangkan ke sistem kontrol hidroponik yang lebih lengkap

