#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LittleFS.h>

#include "config.h"

ESP8266WebServer server(80);
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);
OneWire oneWire(TEMP_PIN); 
DallasTemperature sensors(&oneWire);

// Piny i zmienne
float threshold = 30.0;
float currentTemp = 0;

String getContentType(String filename) {
  if (filename.endsWith(".html")) return "text/html";
  if (filename.endsWith(".css")) return "text/css";
  if (filename.endsWith(".js")) return "application/javascript"; // To naprawia Twój błąd!
  if (filename.endsWith(".ico")) return "image/x-icon";
  return "text/plain";
}

bool handleFileRead(String path) {
  if (path.endsWith("/")) path += "index.html"; // Jeśli ktoś wpisze samo IP, daj mu index
  
  String contentType = getContentType(path);
  
  if (LittleFS.exists(path)) {
    File file = LittleFS.open(path, "r");
    server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  if (!LittleFS.begin()) { Serial.println("LittleFS Error"); return; }
  
  pinMode(BUZZER_PIN, OUTPUT);

  Serial.println("--- PLIKI NA WEMOSIE ---");
  Dir dir = LittleFS.openDir("/");
  while (dir.next()) {
      Serial.print(dir.fileName());
      Serial.print(" \t");
      Serial.println(dir.fileSize());
  }
  Serial.println("------------------------");
  
  lcd.init();
  lcd.backlight();
  sensors.begin();
  
  // Informacja o łączeniu
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Laczenie WiFi...");
  
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) { 
    delay(500); 
    Serial.print(".");
  }

  // Wyświetlenie uzyskanego IP
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Polaczono!");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP()); // Wyświetla np. 192.168.0.12
  
  Serial.println("");
  Serial.println("WiFi polaczone");
  Serial.print("Adres IP: ");
  Serial.println(WiFi.localIP());

  delay(5000); // Czekaj 5 sekund, żebyś zdążył zapisać/przepisać IP
  
  server.on("/data", []() {
    String json = "{\"temp\":" + String(currentTemp) + "}";
    server.send(200, "application/json", json);
  });

  server.on("/temp", []() {
    server.send(200, "text/plain", String(currentTemp));
  });

  server.onNotFound([]() {
    if (!handleFileRead(server.uri())) {
      server.send(404, "text/plain", "404: File Not Found");
    }
  });
  
  server.begin();
  lcd.clear();
}

void loop() {
  server.handleClient();
  sensors.requestTemperatures();
  currentTemp = sensors.getTempCByIndex(0);

  // Wyświetlacz
  lcd.setCursor(0,0);
  lcd.print("T:"); lcd.print(currentTemp); lcd.print(" C  ");
  lcd.setCursor(0,1);
  lcd.print("Prog:"); lcd.print(threshold); lcd.print(" C  ");

  // Alarm
  if(currentTemp > threshold) {
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }
}
