# IoT-Based Hydroponic Nutrient Monitoring System

Sistem monitoring nutrisi hidroponik berbasis IoT yang menggunakan ESP32 untuk membaca parameter penting seperti kadar TDS (Total Dissolved Solids), ketinggian air, dan pengendalian relay/servo secara otomatis. Data juga dikirim ke Telegram dan Google Sheets untuk pemantauan jarak jauh.

## Fitur Utama

- Monitoring kadar TDS menggunakan sensor TDS
- Monitoring ketinggian air dengan sensor ultrasonik JSN-SR04
- Kontrol relay otomatis untuk pengaturan nutrisi
- Kontrol servo untuk membuka/menutup aliran sesuai kondisi
- Notifikasi otomatis melalui Telegram
- Penyimpanan data ke Google Sheets via Google Apps Script
- Tampilan data pada LCD 16x2
- RTC (Real-Time Clock) untuk pencatatan waktu yang akurat

## Komponen Hardware

- ESP32 DevKit
- Sensor TDS
- Sensor ultrasonik JSN-SR04
- RTC DS3231
- LCD I2C 16x2
- Servo 360°
- Relay 4 channel
- Power supply dan perangkat pendukung hidroponik

## Pin Configuration

Berikut pin yang digunakan pada file `Hidroponik.ino`:

- TDS Sensor: GPIO 4
- Trigger Ultrasonik: GPIO 13
- Echo Ultrasonik: GPIO 12
- Relay: GPIO 1, 0, 15, 7
- Servo: GPIO 19
- I2C LCD: SDA = GPIO 2, SCL = GPIO 3

## Cara Menjalankan

1. Buka file `Hidroponik.ino` pada Arduino IDE.
2. Pastikan board ESP32 sudah terinstall.
3. Pilih board dan port COM yang sesuai.
4. Kompilasi dan upload ke ESP32.
5. Sesuaikan konfigurasi WiFi, Telegram, dan script Google Sheets pada kode.

## Konfigurasi Penting

Sebelum digunakan, ubah variabel berikut pada file `Hidroponik.ino`:

- `ssid` dan `password` untuk koneksi WiFi
- `scriptID` untuk Google Apps Script deployment
- `bot("TOKEN", client)` untuk token Telegram bot
- `chatID` untuk ID chat Telegram tujuan
- `rtc.adjust(manualTime);` sesuaikan dengan tanggal/waktu saat ini

> Catatan: Data sensitif seperti WiFi, token Telegram, dan chat ID saat ini masih bersifat hardcoded. Disarankan untuk memindahkannya ke file konfigurasi atau environment variable di tahap pengembangan selanjutnya.

## Struktur Project

- `Hidroponik.ino` — source code utama sistem
- `README.md` — dokumentasi proyek

## Tujuan Proyek

Proyek ini dibuat untuk mempermudah pengontrolan dan pemantauan nutrisi tanaman hidroponik secara otomatis dan real-time, sehingga kondisi air dan nutrisi dapat dipastikan tetap optimal tanpa pengawasan manual terus-menerus.

## Lisensi

Proyek ini dibuat untuk kebutuhan riset dan pengembangan sistem monitoring hidroponik. Silakan sesuaikan lisensi sesuai kebutuhan penggunaan Anda.
