#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
#include <WiFiManager.h>
#include <ESP8266WebServer.h>

#include "config.h"

LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);
OneWire oneWire(TEMP_PIN); 
DallasTemperature sensors(&oneWire);

// Zmienne
float threshold = 30.0;
float currentTemp = 0;
unsigned long lastPushTime = 0;
unsigned long lastLcdUpdate = 0;
const int RESET_HOLD_TIME = 3000; // Czas trzymania przycisku w ms (3 sekundy)
bool isButtonHeld = false;
unsigned long buttonPressTime = 0;

void saveThreshold() {
  EEPROM.put(0, threshold);
  EEPROM.commit();
  Serial.println("Prog zapisany w EEPROM (adres 0): " + String(threshold));
}

void loadThreshold() {
  float storedValue;
  EEPROM.get(0, storedValue);
  
  // Sprawdź czy wartość jest sensowna (po wymazaniu pamięci mogą być śmieci)
  if (!isnan(storedValue) && storedValue > -50 && storedValue < 150) {
    threshold = storedValue;
    Serial.println("Prog odczytany z EEPROM: " + String(threshold));
  } else {
    Serial.println("Brak poprawnego progu w EEPROM, uzywam domyslnego: " + String(threshold));
  }
}

void setup() {
  Serial.begin(115200);
  
  EEPROM.begin(512); // Rezerwujemy 512 bajtów (standard na ESP8266)
  loadThreshold();   // Wczytaj próg z pamięci
  
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RESET_WIFI_PIN, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();
  sensors.begin();

  // 2. WiFiManager - Magia konfiguracji
  lcd.setCursor(0, 0); lcd.print("Szukam WiFi...");

  WiFiManager wm;

  bool res = wm.autoConnect("Termometr-Konfiguracja"); 

  if(!res) {
    Serial.println("Nie udalo sie polaczyc. Restart...");
    ESP.restart();
  }

  // Jeśli tu dotarł, to znaczy że WiFi działa!
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("WiFi OK!");
  lcd.setCursor(0, 1); lcd.print(WiFi.localIP());
  Serial.println("\nPolaczono z WiFi! IP: " + WiFi.localIP().toString());
  
  lcd.clear();
}

void pushData() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    
    http.begin(client, SERVER_URL);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-API-Key", API_KEY);

    JsonDocument doc;
    doc["temp"] = currentTemp;
    doc["threshold"] = threshold;

    String requestBody;
    serializeJson(doc, requestBody);

    int httpResponseCode = http.POST(requestBody);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("HTTP Response code: " + String(httpResponseCode));
      Serial.println("Response: " + response);

      JsonDocument resDoc;
      DeserializationError error = deserializeJson(resDoc, response);

      if (!error && resDoc.containsKey("threshold")) {
        float newThreshold = resDoc["threshold"];
        if (newThreshold != threshold) {
          threshold = newThreshold;
          saveThreshold(); // Zapisz nową wartość w EEPROM
          Serial.println("Zaktualizowano prog: " + String(threshold));
        }
      }
    } else {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
}

void loop() {

  // --- OBSŁUGA RESETU WIFI (Długie naciśnięcie) ---
  int buttonState = digitalRead(RESET_WIFI_PIN);

  if (buttonState == HIGH) { // Przycisk wciśnięty
    
    // Jeśli to dopiero początek wciśnięcia, zapamiętaj czas
    if (!isButtonHeld) {
      buttonPressTime = millis();
      isButtonHeld = true;
      Serial.println("Przycisk wcisniety...");
    }

    // Sprawdź, czy przycisk jest trzymany wystarczająco długo
    if (millis() - buttonPressTime >= RESET_HOLD_TIME) {
      
      // AKCJA RESETU
      Serial.println("RESETOWANIE USTAWIEN WIFI!");
      
      lcd.clear();
      lcd.setCursor(0, 0); lcd.print("RESET WiFi...");
      lcd.setCursor(0, 1); lcd.print("Puszcz przycisk");
      
      // Sygnał dźwiękowy
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);
      delay(100);
      digitalWrite(BUZZER_PIN, HIGH);
      delay(500);
      digitalWrite(BUZZER_PIN, LOW);

      // Kasowanie ustawień
      WiFiManager wm;
      wm.resetSettings();
      
      delay(1000); 
      ESP.restart(); // Restart urządzenia - najbezpieczniejsza metoda po resecie
    }

  } else {
    // Przycisk puszczony - resetuj flagę
    isButtonHeld = false;
    buttonPressTime = 0;
  }

  // --- TIMER 1: Odczyt temp + LCD (np. co 2 sekundy) ---
  if (millis() - lastLcdUpdate >= LCD_INTERVAL) {
    lastLcdUpdate = millis();

    // 1. Odczyt temperatury (robimy to raz na 2s, nie ciągle!)
    sensors.requestTemperatures();
    float newTemp = sensors.getTempCByIndex(0);
    
    // Zabezpieczenie przed błędem odczytu (-127)
    if (newTemp != -127.0) {
      currentTemp = newTemp;
    }

    // 2. Obsługa Alarmu (sprawdzamy przy każdym nowym odczycie)
    if (currentTemp >= threshold) {
      digitalWrite(BUZZER_PIN, HIGH);
    } else {
      digitalWrite(BUZZER_PIN, LOW);
    }

    // 3. Aktualizacja LCD
    lcd.setCursor(0,0);
    lcd.print("T:"); lcd.print(currentTemp, 1); lcd.print(" C    "); 
    lcd.setCursor(0,1);
    lcd.print("Pr:"); lcd.print(threshold, 1); lcd.print(" C    ");
  }

  // --- TIMER 2: Wysyłanie danych (np. co 30 sekund) ---
  if (millis() - lastPushTime >= PUSH_INTERVAL || lastPushTime == 0) {
    // Żeby buzzer nie wył w nieskończoność jak zerwie neta przy wysyłaniu:
    // digitalWrite(BUZZER_PIN, LOW); 
    
    Serial.println("Inicjowanie wysyłania danych...");
    pushData(); 
    lastPushTime = millis();
  }
}
