#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Kredensial WiFi
const char* ssid = "Kamal";
const char* password = "wawanbudwir";

// Inisialisasi LCD (alamat I2C, jumlah kolom, jumlah baris)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// URL endpoint API Laravel untuk POST dan GET
const char* postUrl = "http://192.168.0.3:8000/api/devices_post";
const char* getUrl = "http://192.168.0.3:8000/api/devices_get/Alat1";

// Pin untuk perangkat yang terhubung
const int ledGreen = D3;
const int ledRed = D4;
const int buzzer = D5;
const int relay1 = D6;
const int relay2 = D7;

void setup() {
  Serial.begin(9600);

  // Inisialisasi pin sebagai output
  pinMode(ledGreen, OUTPUT);
  pinMode(ledRed, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);

  // Inisialisasi LCD
  lcd.begin();
  lcd.backlight(); // Menghidupkan backlight LCD

  // Set kondisi awal perangkat
  digitalWrite(ledGreen, LOW);
  digitalWrite(ledRed, LOW);
  digitalWrite(buzzer, LOW);
  digitalWrite(relay1, HIGH);
  digitalWrite(relay2, HIGH);

  // Koneksi ke WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Menghubungkan ke WiFi...");
  }
  Serial.println("Terhubung ke WiFi");

  // Kirim data perangkat ke server
  sendDeviceData();

  // Tunggu sebelum memulai loop
  delay(2000);
}

void loop() {
  // Ambil data dari server
  fetchDeviceData();

  // Tunggu sebelum pengambilan data berikutnya
  delay(5000);
}

// Fungsi untuk mengirim data alat ke server
void sendDeviceData() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    WiFiClient client;
    
    http.begin(client, postUrl);
    http.addHeader("Content-Type", "application/json");

    String jsonData = "{\"name\":\"Alat1\",\"password\":\"passwordalat1\"}";
    int httpResponseCode = http.POST(jsonData);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Respon POST: " + response);
    } else {
      Serial.print("Error mengirim POST: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
}

// Fungsi untuk mengambil data alat dari server
void fetchDeviceData() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    WiFiClient client;
    
    http.begin(client, getUrl);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Respon GET: " + response);

      // Parsing JSON
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, response);

      if (!error) {
        bool isActive = doc["is_active"];
        int daysRemaining = doc["days_remaining"];

        // Kontrol perangkat berdasarkan data yang diterima
        if (isActive) {
          digitalWrite(ledGreen, HIGH);
          digitalWrite(relay1, LOW);
          digitalWrite(relay2, LOW);
          
          // Tampilkan sisa hari di LCD
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Sisa Hari: ");
          lcd.print(daysRemaining);

          if (daysRemaining > 3) {
            digitalWrite(ledGreen, HIGH);
            digitalWrite(ledRed, LOW);
            digitalWrite(buzzer, LOW);
          } else if (daysRemaining <= 3 && daysRemaining > 0) {
            // Jika sisa hari kurang dari atau sama dengan 3, nyalakan alarm
            digitalWrite(ledRed, HIGH);
            digitalWrite(buzzer, HIGH);
            delay(1000);
            digitalWrite(ledRed, LOW);
            digitalWrite(buzzer, LOW);
            delay(1000);
            digitalWrite(ledGreen, HIGH);
          } else if (daysRemaining <= 0) {
            // Jika masa sewa habis, matikan listrik
            digitalWrite(ledGreen, LOW);
            digitalWrite(ledRed, HIGH);
            digitalWrite(buzzer, LOW);
            digitalWrite(relay1, HIGH);
            digitalWrite(relay2, HIGH);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Waktu Sewa Habis");
            delay(500);
          } else {
            // Kamar tersedia kembali
            digitalWrite(ledGreen, LOW);
            digitalWrite(ledRed, HIGH);
            digitalWrite(buzzer, LOW);
            digitalWrite(relay1, HIGH);
            digitalWrite(relay2, HIGH);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Kamar Tersedia");
          }
        } else {
          // Jika perangkat tidak aktif
          digitalWrite(ledGreen, LOW);
          digitalWrite(ledRed, HIGH);
          digitalWrite(relay1, HIGH);
          digitalWrite(relay2, HIGH);
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Tidak Tersambung");
        }
      } else {
        Serial.println("Kesalahan Parsing JSON");
      }
    } else {
      Serial.print("Error saat GET: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
}
