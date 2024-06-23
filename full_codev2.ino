#include <ESP32Servo.h>
#include <WiFi.h>
#include <UniversalTelegramBot.h>
#include <WiFiClientSecure.h>

// Definisikan pin untuk sensor ultrasonik
const int trigPin = 23;
const int echoPin = 22;

// Definisikan pin untuk sensor inframerah (IR)
const int irPin = 15;

// Definisikan pin untuk motor servo
const int servoPin = 4;

// Membuat objek servo
Servo myServo;

// Informasi jaringan WiFi
const char* ssid = "nama_wifi";
const char* password = "pass_wifi";

// Token dan ID bot Telegram
const char* telegramToken = "isi_tokenbot_telegram";
const char* chat_id = "isi_chatid_telegram";

// Membuat objek bot Telegram
WiFiClientSecure client;
UniversalTelegramBot bot(telegramToken, client);

unsigned long lastNotificationTime = 0;
const unsigned long notificationInterval = 20000; // 20 detik

unsigned long lastCheckTime = 0;
const unsigned long checkInterval = 1000; // 1 detik

bool paketDiDalam = false;

void setup() {
  // Memulai komunikasi serial dengan baud rate 115200
  Serial.begin(115200);
  
  // Mengatur pin untuk sensor ultrasonik
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  // Mengatur pin untuk sensor inframerah (IR)
  pinMode(irPin, INPUT);
  
  // Menghubungkan servo ke pin yang telah ditentukan
  myServo.attach(servoPin);
  myServo.write(0); // Inisialisasi servo pada posisi 0 derajat

  // Menghubungkan ke jaringan WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  
  // Mengatur waktu untuk koneksi SSL
  client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
}

void loop() {
  // Mengirim sinyal trigger untuk sensor ultrasonik
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Membaca sinyal echo dari sensor ultrasonik
  long duration = pulseIn(echoPin, HIGH);

  // Menghitung jarak dalam cm
  long distance = duration * 0.034 / 2;

  // Menampilkan jarak di Serial Monitor
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // Jika jarak kurang dari 10 cm dan belum mengirim notifikasi dalam interval waktu tertentu, kirim notifikasi ke bot Telegram
  if (distance < 10 && (millis() - lastNotificationTime > notificationInterval)) {
    bot.sendMessage(chat_id, "Misi Paket. Apakah Anda ingin membuka kotak paket? Ketik /buka untuk membuka kotak.", "");
    lastNotificationTime = millis();
  }

  // Membaca nilai dari sensor IR
  int irValue = digitalRead(irPin);

  // Jika sensor IR mendeteksi objek (nilai LOW), kirim notifikasi ke bot Telegram
  if (irValue == LOW && !paketDiDalam) {
    bot.sendMessage(chat_id, "Paket sudah di dalam", "");
    paketDiDalam = true;
  } else if (irValue == HIGH) {
    paketDiDalam = false;
  }

  // Memeriksa pesan dari bot Telegram setiap detik
  if (millis() - lastCheckTime > checkInterval) {
    lastCheckTime = millis();
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while (numNewMessages) {
      Serial.println("Got response");
      for (int i = 0; i < numNewMessages; i++) {
        String chat_id = String(bot.messages[i].chat_id);
        String text = bot.messages[i].text;

        if (text == "/buka") {
          myServo.write(130);
          bot.sendMessage(chat_id, "Kotak paket dibuka. Ketik /tutup untuk menutup kotak.", "");
        } else if (text == "/tutup") {
          myServo.write(0);
          bot.sendMessage(chat_id, "Kotak paket ditutup.", "");
        }
      }
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
  }

  delay(100); // Mengurangi delay untuk mempercepat loop
}