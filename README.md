# Inteligentny Termometr IoT (ESP8266)

Projekt inteligentnego termometru opartego na mikrokontrolerze **ESP8266 (Wemos D1 Mini)**. Urządzenie mierzy temperaturę, wyświetla ją na ekranie LCD, alarmuje o przekroczeniu progu (Buzzer) oraz synchronizuje dane z centralnym serwerem HTTP.

## 📁 Struktura projektu

*   `SMiW-sem5.ino` - Główny program dla mikrokontrolera ESP8266.
*   `config.h` - Konfiguracja pinów i parametrów połączenia klienta.
*   `server/` - Aplikacja serwerowa (Node.js + Express).
    *   `index.js` - Logika backendu i obsługa bazy danych.
    *   `public/` - Panel sterowania (Frontend).
    *   `database.sqlite` - Lokalna baza danych historii pomiarów.

## 🚀 Funkcjonalności

*   🌡 **Precyzyjny pomiar temperatury** (DS18B20).
*   📟 **Lokalny podgląd** na wyświetlaczu LCD 16x2.
*   📢 **Lokalny alarm** (Buzzer) po przekroczeniu progu.
*   🌐 **Centrum Monitorowania (Web Dashboard):**
    *   Wykresy historyczne (Dzień, Miesiąc, Rok).
    *   Zdalna zmiana progu alarmowego w czasie rzeczywistym.
    *   Podgląd temperatury w czasie rzeczywistym.
*   💾 **Pamięć nieulotna:** Zapamiętywanie ustawień (EEPROM na ESP, SQLite na serwerze).
*   📱 **WiFiManager:** Łatwa konfiguracja sieci Wi-Fi bez edycji kodu.

## 🛠 Wymagania sprzętowe

1.  **Mikrokontroler:** Wemos D1 Mini (ESP8266).
2.  **Sensory:** DS18B20 (z rezystorem 4.7kΩ).
3.  **HMI:** Wyświetlacz LCD 16x2 I2C.
4.  **Sygnalizacja:** Buzzer aktywny 5V.
5.  **Inne:** Przycisk resetu, płytka stykowa/prototypowa, przewody.

## 📐 Specyfikacja fizyczna

*   **Zasilanie:** 5V przez microUSB (Wemos D1 Mini).
*   **Pobór prądu:** ~70-150mA (szczytowo podczas transmisji WiFi).
*   **Wymiary obudowy (sugerowane):** 100x60x40mm.
*   **Środowisko pracy:** 0°C do 50°C (ograniczenie LCD i elektroniki).

## 🔌 Połączenia (Pinout)

| Podzespół | Pin ESP8266 (Wemos) | Opis |
| :--- | :--- | :--- |
| **DS18B20 (DATA)** | **D5** (GPIO14) | Magistrala 1-Wire |
| **Buzzer (+)** | **D7** (GPIO13) | Sterowanie alarmem |
| **Reset WiFi (Button)** | **D0** (GPIO16) | Resetowanie ustawień Wi-Fi |
| **LCD SDA** | **D2** (GPIO4) | Dane I2C |
| **LCD SCL** | **D1** (GPIO5) | Zegar I2C |

## ⚙️ Konfiguracja klienta (`config.h`)

Plik `config.h` definiuje kluczowe parametry połączenia i mapowanie pinów.

```cpp
// Adres API (endpoint dla danych pomiarowych)
#define SERVER_URL "http://TWOJ_ADRES_IP:3000/api/data"
#define API_KEY "TWOJ_TAJNY_KLUCZ"

#define LCD_ADDR 0x27
#define TEMP_PIN D5
#define BUZZER_PIN D7
#define RESET_WIFI_PIN D0

#define PUSH_INTERVAL 5000  // Częstotliwość wysyłania na serwer (ms)
#define LCD_INTERVAL 2000   // Częstotliwość odświeżania LCD (ms)
```

## 🖥️ Serwer (Node.js)

Serwer zarządza historią danych oraz pozwala na zdalną zmianę konfiguracji progu.

### Instalacja i uruchomienie

1.  Wejdź do katalogu serwera: `cd server`.
2.  Zainstaluj zależności: `npm install`.
3.  Skonfiguruj zmienne środowiskowe:
    *   Utwórz plik `.env` wewnątrz folderu `server/`.
    *   Dodaj wpis: `API_KEY=TWOJ_TAJNY_KLUCZ`.
4.  Uruchom serwer: `node index.js`.
5.  Otwórz przeglądarkę pod adresem: `http://localhost:3000`.

### Bezpieczeństwo

Komunikacja między ESP8266 a serwerem jest zabezpieczona nagłówkiem `X-API-Key`. Klucz zdefiniowany w `/server/.env` musi być identyczny z tym w `config.h` urządzenia.

## 📦 Biblioteki Arduino

Wymagane do kompilacji projektu:
*   `ArduinoJson`
*   `LiquidCrystal_I2C`
*   `DallasTemperature` & `OneWire`
*   `WiFiManager` (tzpu)