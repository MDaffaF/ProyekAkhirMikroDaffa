#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define SLOT 4

// === WiFi & ThingSpeak ===
const char* ssid = "Wokwi-GUEST";
const char* password = "";
String apiKey = "PPXOH2493SHE1APL";
int channelID = 2997847;

// === TRIG dan ECHO per slot ===
const int trigPin1 = 33;
const int echoPin1 = 32;

const int trigPin2 = 25;
const int echoPin2 = 26;

const int trigPin3 = 21;
const int echoPin3 = 19;

const int trigPin4 = 18;
const int echoPin4 = 5;

// === LED Hijau dan Merah per slot ===
const int ledHijau1 = 12;
const int ledMerah1 = 13;

const int ledHijau2 = 14;
const int ledMerah2 = 27;

const int ledHijau3 = 4;
const int ledMerah3 = 22;

const int ledHijau4 = 15;
const int ledMerah4 = 2;

// Array agar tetap bisa diproses pakai loop
const int trigPins[SLOT] = {trigPin1, trigPin2, trigPin3, trigPin4};
const int echoPins[SLOT] = {echoPin1, echoPin2, echoPin3, echoPin4};
const int ledHijau[SLOT] = {ledHijau1, ledHijau2, ledHijau3, ledHijau4};
const int ledMerah[SLOT] = {ledMerah1, ledMerah2, ledMerah3, ledMerah4};

// Simpan status jarak sementara
long jarakSlot[SLOT];

// LCD I2C (alamat umum 0x27), ukuran 16x2
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < SLOT; i++) {
    pinMode(trigPins[i], OUTPUT);
    pinMode(echoPins[i], INPUT);
    pinMode(ledHijau[i], OUTPUT);
    pinMode(ledMerah[i], OUTPUT);
  }

  // Koneksi WiFi
  WiFi.begin(ssid, password);
  Serial.print("Menghubungkan ke WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ WiFi Tersambung!");

  Wire.begin(16, 17);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(3, 0);
  lcd.print("Monitoring");
  lcd.setCursor(5, 1);
  lcd.print("Parkir");
  delay(1500);
  lcd.clear();
}

long bacaJarak(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  long durasi = pulseIn(echo, HIGH, 30000);
  long jarak = durasi * 0.034 / 2;
  return jarak;
}

void loop() {
  Serial.println("\n=== PEMANTAUAN SLOT PARKIR ===");
  int statusSlot[SLOT];

  // ✅ Inisialisasi URL untuk ThingSpeak
  String url = "api.thingspeak.com/update?api_key=" + apiKey;

  for (int i = 0; i < SLOT; i++) {
    jarakSlot[i] = bacaJarak(trigPins[i], echoPins[i]);
    statusSlot[i] = (jarakSlot[i] > 0 && jarakSlot[i] < 50) ? 1 : 0;

    if (statusSlot[i] == 1) {
      digitalWrite(ledHijau[i], LOW);
      digitalWrite(ledMerah[i], HIGH);
      Serial.printf("Slot %d: TERISI\n", i + 1);
    } else {
      digitalWrite(ledHijau[i], HIGH);
      digitalWrite(ledMerah[i], LOW);
      Serial.printf("Slot %d: KOSONG\n", i + 1);
    }

    url += "&field" + String(i + 1) + "=" + String(statusSlot[i]);
  }

  lcd.clear();

  lcd.setCursor(2, 0);
  lcd.print("A1 :");
  lcd.print(statusSlot[0] ? "X " : "O ");
  lcd.print("A3 :");
  lcd.print(statusSlot[2] ? "X" : "O");
  lcd.setCursor(2, 1);
  lcd.print("A2 :");
  lcd.print(statusSlot[1] ? "X " : "O ");
  lcd.print("A4 :");
  lcd.print(statusSlot[3] ? "X" : "O");

  // Kirim data ke ThingSpeak
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(url);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      Serial.print("📤 Data dikirim! Kode respons: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("❌ Gagal kirim: ");
      Serial.println(http.errorToString(httpResponseCode).c_str());
    }
    http.end();
  } else {
    Serial.println("⚠️ WiFi tidak tersambung.");
  }

  delay(16000);  // Delay untuk interval pengiriman (min 15 detik)
}
