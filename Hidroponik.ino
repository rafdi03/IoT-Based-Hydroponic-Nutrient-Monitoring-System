// =================================================================
// PUSTAKA (LIBRARIES)
// =================================================================
#include <Wire.h>
#include "RTClib.h"
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <HTTPClient.h>  // DITAMBAHKAN

// =================================================================
// WIFI + TELEGRAM
// =================================================================
const char* ssid     = "Rumah Kita";
const char* password = "EKAGUNAPUTRA03";

// Google Script Deployment ID - GANTI DENGAN ID ANDA
const char* scriptID = "AKfycbzb1r_g_HqGW4TWA0WEUNv2mQnwqg4rVtQRSO4keKCpkslR9Tb2vxpoIoG3TPltpw_y";

WiFiClientSecure client;
UniversalTelegramBot bot("8364850043:AAHxd2adOV_M1el793fztbaQqg-qxMFqEPM", client);
String chatID = "1222959013";

// Waktu pengiriman
unsigned long lastSend12Jam = 0;
unsigned long lastSendSpreadsheet = 0;
bool initialTelegramSent = false;
bool initialSpreadsheetSent = false;
unsigned long setupStartTime = 0;

// interval
const unsigned long interval12Jam = 43200000;  // 12 jam
const unsigned long interval4Jam  = 14400000;  // 4 jam

// =================================================================
// INISIALISASI OBJEK
// =================================================================
RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo servo360;

// =================================================================
// DEFINISI PIN
// =================================================================
#define TdsSensorPin 4
const int trigPin = 13;
const int echoPin = 12;
const int relayPins[] = {1, 0, 15, 7};
const int numRelays = 4;
#define SERVO_PIN 19

// =================================================================
// PENGATURAN SENSOR TDS
// =================================================================
#define VREF 3.3
#define SCOUNT 30
int analogBuffer[SCOUNT];
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0;
float averageVoltage = 0;
float tdsValue = 0;
float temperature = 28.0;
float cal_factor = 0.88;

// =================================================================
// VARIABEL GLOBAL
// =================================================================
long duration;
float distanceCm;

unsigned long tdsSampleTimepoint = 0;
unsigned long printTimepoint = 0;
unsigned long jsnReadTimepoint = 0;
unsigned long lcdUpdateTimepoint = 0;

unsigned long relay24Timepoint = 0;
unsigned long servoTimepoint = 0;
bool relay24Active = false;
bool servoActive = false;
bool tdsConditionTriggered = false;

// =================================================================
// FUNGSI FILTER MEDIAN
// =================================================================
int getMedianNum(int bArray[], int iFilterLen) {
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++) bTab[i] = bArray[i];
  for (int j = 0; j < iFilterLen - 1; j++) {
    for (int i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        int bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if (iFilterLen % 2 > 0)
    return bTab[(iFilterLen - 1) / 2];
  else
    return (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
}

// =================================================================
// BACA TDS INSTANT
// =================================================================
void readTDS_Instant() {
  for (int i = 0; i < SCOUNT; i++) analogBufferTemp[i] = analogBuffer[i];
  int medianValue = getMedianNum(analogBufferTemp, SCOUNT);
  averageVoltage = medianValue * VREF / 4095.0;

  if (averageVoltage > 0.1) {
    float compCoeff = 1.0 + 0.02 * (temperature - 25.0);
    float compVolt = averageVoltage / compCoeff;
    tdsValue = (133.42 * pow(compVolt, 3) - 255.86 * pow(compVolt, 2) + 857.39 * compVolt) * cal_factor;
    if (tdsValue < 0) tdsValue = 0;
  } else {
    tdsValue = 0;
  }

  // =================================================================
  // KOREKSI TDS AGAR SESUAI DENGAN TDS METER (OFFSET -250)
  // =================================================================
  tdsValue -= 250;
  if (tdsValue < 0) tdsValue = 0;  // Jangan sampai minus
}

// =================================================================
// BACA JSN INSTANT
// =================================================================
void readJSN_Instant() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distanceCm = duration * 0.034 / 2;
}

// =================================================================
// KIRIM PESAN AWAL TELEGRAM
// =================================================================
void kirimPesanAwal() {
  String pesan = "🔔 *Sistem Monitoring Menyala*\n";
  pesan += "Alat sudah aktif dan berjalan.\n";
  pesan += "\n📌 *Informasi Awal:*\n";
  pesan += "• TDS: " + String(tdsValue, 1) + " ppm\n";
  pesan += "• Jarak: " + String(distanceCm, 1) + " cm\n";
  pesan += "\nNama: wulan\nBahasa: id\n";

  bot.sendMessage(chatID, pesan, "Markdown");
  Serial.println("Pesan awal Telegram terkirim");
}

// =================================================================
// KIRIM TELEGRAM SETIAP 12 JAM
// =================================================================
void kirimTelegram12Jam() {
  String pesan = "⏰ *Laporan 12 Jam*\n";
  pesan += "TDS: " + String(tdsValue, 1) + " ppm\n";
  pesan += "Jarak: " + String(distanceCm, 1) + " cm\n";
  pesan += "Waktu: " + String(rtc.now().timestamp()) + "\n";
  bot.sendMessage(chatID, pesan, "Markdown");
  Serial.println("Telegram 12 jam terkirim");
}

// =================================================================
// KIRIM SPREADSHEET (VERSI YANG SUDAH DIPERBAIKI)
// =================================================================
void kirimSpreadsheet() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, skipping spreadsheet send.");
    return;
  }

  DateTime now = rtc.now();
  
  // Buat URL lengkap dengan data aktual dari sensor
  String url = "https://script.google.com/macros/s/";
  url += scriptID;
  url += "/exec?";
  url += "tds=" + String(tdsValue, 1);    // Nilai TDS dengan 1 desimal
  url += "&jsn=" + String(distanceCm, 1); // Nilai jarak dengan 1 desimal
  url += "&timestamp=" + String(now.unixtime()); // Gunakan Unix timestamp dari RTC

  Serial.println("Mengirim ke Spreadsheet:");
  Serial.println("TDS: " + String(tdsValue, 1) + " ppm");
  Serial.println("Jarak: " + String(distanceCm, 1) + " cm");
  Serial.println("Waktu RTC: " + now.timestamp());
  Serial.println("URL: " + url);

  // Gunakan HTTPClient dengan WiFiClientSecure
  HTTPClient http;
  WiFiClientSecure client_secure;
  client_secure.setInsecure();
  
  http.begin(client_secure, url);

  int httpCode = http.GET();
  
  if (httpCode > 0) {
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);

    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println("Berhasil dikirim ke Spreadsheet. Response: " + payload);
    } else {
      String payload = http.getString();
      Serial.println("Response: " + payload);
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}

// =================================================================
// FUNGSI UNTUK MENGECEK RTC
// =================================================================
void cekRTC() {
  DateTime now = rtc.now();
  Serial.println("=== INFO RTC ===");
  Serial.printf("Tanggal: %02d/%02d/%04d\n", now.day(), now.month(), now.year());
  Serial.printf("Waktu: %02d:%02d:%02d\n", now.hour(), now.minute(), now.second());
  Serial.printf("Timestamp: %s\n", now.timestamp().c_str());
  Serial.println("================");
}

// =================================================================
// SETUP
// =================================================================
void setup() {
  Serial.begin(115200);
  delay(500);
  
  setupStartTime = millis(); // Catat waktu mulai setup

  WiFi.begin(ssid, password);
  Serial.println("Menyambungkan ke WiFi...");

  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected!");
  Serial.println(WiFi.localIP());

  client.setInsecure();

  Wire.begin(2, 3);

  // === KALIBRASI RTC ===
  rtc.begin();
  
  // Set RTC manual ke tanggal hari ini jam 20:10
  // Format: DateTime(tahun, bulan, hari, jam, menit, detik)
  DateTime manualTime(2025, 11, 16, 9, 10, 0); // Ganti dengan tanggal hari ini
  rtc.adjust(manualTime);
  
  Serial.println("RTC dikalibrasi ke 20:10");
  cekRTC();

  lcd.init();
  lcd.backlight();

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  for (int i = 0; i < numRelays; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], LOW);
  }

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  servo360.attach(SERVO_PIN);
  servo360.write(90);

  // Baca sensor multiple times untuk stabilisasi
  for(int i = 0; i < 10; i++) {
    readJSN_Instant();
    readTDS_Instant();
    delay(100);
  }
  
  Serial.println("Sensor sudah dibaca multiple times untuk stabilisasi");

  // Set timer untuk interval reguler
  lastSend12Jam = millis();
  lastSendSpreadsheet = millis();
}

// =================================================================
// LOOP
// =================================================================
void loop() {
  unsigned long currentTime = millis();
  
  // === JSN ===
  if (currentTime - jsnReadTimepoint > 1000) {
    jsnReadTimepoint = currentTime;
    readJSN_Instant();
  }

  // === TDS ===
  if (currentTime - tdsSampleTimepoint > 40) {
    tdsSampleTimepoint = currentTime;
    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);
    analogBufferIndex = (analogBufferIndex + 1) % SCOUNT;
    readTDS_Instant();
  }

  // === PENGIRIMAN AWAL ===
  if (!initialTelegramSent && currentTime - setupStartTime > 5000) {
    // Kirim Telegram setelah 5 detik
    kirimPesanAwal();
    initialTelegramSent = true;
    lastSend12Jam = currentTime; // Reset timer Telegram
    Serial.println("Telegram awal terkirim setelah delay 5 detik");
  }

  if (initialTelegramSent && !initialSpreadsheetSent && currentTime - setupStartTime > 7000) {
    // Kirim Spreadsheet 2 detik setelah Telegram (5+2=7 detik)
    kirimSpreadsheet();
    initialSpreadsheetSent = true;
    lastSendSpreadsheet = currentTime; // Reset timer Spreadsheet
    Serial.println("Spreadsheet awal terkirim setelah delay 2 detik dari Telegram");
  }

  // === TELEGRAM 12 JAM (INTERVAL TERPISAH) ===
  if (initialTelegramSent && currentTime - lastSend12Jam > interval12Jam) {
    kirimTelegram12Jam();
    lastSend12Jam = currentTime;
    
    // Set timer untuk kirim Spreadsheet 2 detik setelah Telegram periodik
    lastSendSpreadsheet = currentTime;
    Serial.println("Telegram periodik terkirim, menunggu 2 detik untuk Spreadsheet");
  }

  // === SPREADSHEET 4 JAM (INTERVAL TERPISAH) ===
  if (initialSpreadsheetSent && currentTime - lastSendSpreadsheet > interval4Jam) {
    // Pastikan tidak mengirim bersamaan dengan Telegram (beri jeda 2 detik)
    if (currentTime - lastSend12Jam > 2000) {
      kirimSpreadsheet();
      lastSendSpreadsheet = currentTime;
      Serial.println("Spreadsheet periodik terkirim");
    }
  }

  // === KONDISI TDS & SERVO (DENGAN DELAY 5 DETIK) ===
  // Tunggu 5 detik setelah startup sebelum cek kondisi TDS
  if (currentTime - setupStartTime > 5000) {
    if (tdsValue < 700 && !relay24Active && !servoActive && !tdsConditionTriggered) {
      digitalWrite(relayPins[3], HIGH);
      relay24Active = true;
      relay24Timepoint = currentTime;
      tdsConditionTriggered = true;
      Serial.println("Solenoid AKTIF - TDS < 700 ppm");
    }

    if (relay24Active && (currentTime - relay24Timepoint >= 1000)) {
      digitalWrite(relayPins[3], LOW);
      relay24Active = false;
      Serial.println("Solenoid NON-AKTIF setelah 1 detik");

      servo360.write(70);
      servoActive = true;
      servoTimepoint = currentTime;
      Serial.println("Servo AKTIF - posisi 70 derajat");
    }

    if (servoActive && (currentTime - servoTimepoint >= 10000)) {
      servo360.write(90);
      servoActive = false;
      Serial.println("Servo NON-AKTIF - kembali ke posisi 90 derajat");
    }

    if (tdsValue > 720 && !relay24Active && !servoActive) {
      tdsConditionTriggered = false;
      Serial.println("Kondisi TDS reset - TDS > 720 ppm");
    }
  }

  // === LOGIKA JSN ===
  if (distanceCm <= 35)
    digitalWrite(relayPins[0], LOW);
  else
    digitalWrite(relayPins[0], HIGH);

  // === LCD ===
  if (currentTime - lcdUpdateTimepoint > 1000) {
    lcdUpdateTimepoint = currentTime;
    DateTime now = rtc.now();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.printf("%02d:%02d:%02d %04d", now.hour(), now.minute(), now.second(), now.year());
    lcd.setCursor(0, 1);
    lcd.printf("J:%.0f T:%.0f", distanceCm, tdsValue);
  }

  // === SERIAL ===
  if (currentTime - printTimepoint > 2000) {
    printTimepoint = currentTime;
    Serial.printf("TDS: %.2f ppm | Jarak: %.2f cm | Waktu: %lu ms\n", tdsValue, distanceCm, currentTime);
    
    // Debug info pengiriman
    Serial.printf("Next Telegram: %lu menit | Next Spreadsheet: %lu menit\n", 
                 (interval12Jam - (currentTime - lastSend12Jam)) / 60000,
                 (interval4Jam - (currentTime - lastSendSpreadsheet)) / 60000);
    
    // Debug info solenoid
    if (currentTime - setupStartTime <= 5000) {
      Serial.println("Solenoid: MENUNGGU 5 DETIK UNTUK STABILISASI");
    } else {
      Serial.printf("Solenoid: %s | Servo: %s | TDS Trigger: %s\n",
                   relay24Active ? "AKTIF" : "NON-AKTIF",
                   servoActive ? "AKTIF" : "NON-AKTIF",
                   tdsConditionTriggered ? "TRIGGERED" : "RESET");
    }
  }
}