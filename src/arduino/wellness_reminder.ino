#include <Wire.h>
#include <LiquidCrystal.h>
#include <RTClib.h>
#include <DHT.h>
#include <EEPROM.h>

// ==== DHT ====
#define DHTPIN 2
#define DHTTYPE DHT22
static DHT dht(DHTPIN, DHTTYPE);

// ==== LCD paralelo 16x2 ====
// Ajuste estes pinos conforme a ligacao do seu LCD
static constexpr uint8_t LCD_RS = 12;
static constexpr uint8_t LCD_EN = 11;
static constexpr uint8_t LCD_D4 = 10;
static constexpr uint8_t LCD_D5 = 8;
static constexpr uint8_t LCD_D6 = 7;
static constexpr uint8_t LCD_D7 = 4;

static LiquidCrystal lcd(LCD_RS, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

// ==== RTC ====
static RTC_DS1307 rtc;

// ==== IO ====
static constexpr uint8_t LED_GREEN = 5;
static constexpr uint8_t LED_RED   = 3;
static constexpr uint8_t BUZZER    = 6;

static constexpr uint8_t JOY_SW    = 9;   // botao do joystick
static constexpr uint8_t LDR_PIN   = A0;

// ==== LIMITES ====
static constexpr float TEMP_MIN = 22.0F;
static constexpr float TEMP_MAX = 25.0F;
static constexpr int   LUM_MIN  = 40;   // em %
static constexpr int   LUM_MAX  = 80;   // em %

// ==== TEMPOS ====
// Para testes: 1 "minuto" = 30 segundos.
// Para uso real, troque BASE_MIN_MS para 60000UL.
static constexpr unsigned long BASE_MIN_MS    = 30000UL;
static constexpr unsigned long INTERVAL_BREAK = 2 * BASE_MIN_MS; // intervalo entre pausas
static constexpr unsigned long SENSOR_READ_MS = 2000UL;

// ==== EEPROM ====
struct MemoryData {
  uint16_t signature;
  uint16_t pauses;   // quantas pausas o usuario fez
};

static MemoryData memoryData;
static constexpr uint16_t MEM_SIGNATURE = 0xBEEF;
static constexpr int      MEM_ADDRESS   = 0;

// ==== ESTADO ====
static bool sessionActive      = false;
static unsigned long sessionStart   = 0;
static unsigned long lastBreak      = 0;
static unsigned long lastSensorRead = 0;

static float currentTemp       = 0.0F;
static int   currentLum        = 0;
static int   currentLumPercent = 0;

void setup() {
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED,   OUTPUT);
  pinMode(BUZZER,    OUTPUT);
  pinMode(JOY_SW,    INPUT_PULLUP);
  pinMode(LDR_PIN,   INPUT);

  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);
  digitalWrite(BUZZER, LOW);

  lcd.begin(16, 2);

  Wire.begin();
  rtc.begin();
  dht.begin();

  Serial.begin(9600);

  EEPROM.get(MEM_ADDRESS, memoryData);
  if (memoryData.signature != MEM_SIGNATURE) {
    memoryData.signature = MEM_SIGNATURE;
    memoryData.pauses    = 0;
    EEPROM.put(MEM_ADDRESS, memoryData);
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Wellness Reminder");
  lcd.setCursor(0, 1);
  lcd.print("Press btn p iniciar");

  for (int i = 0; i < 2; ++i) {
    digitalWrite(BUZZER, HIGH);
    delay(150);
    digitalWrite(BUZZER, LOW);
    delay(100);
  }
}

void loop() {
  bool buttonDown = (digitalRead(JOY_SW) == LOW);

  if (!sessionActive) {
    if (buttonDown) {
      while (digitalRead(JOY_SW) == LOW) {
        delay(20);
      }
      while (digitalRead(JOY_SW) == HIGH) {
        delay(20);
      }
      while (digitalRead(JOY_SW) == LOW) {
        delay(20);
      }

      sessionActive = true;
      sessionStart  = millis();
      lastBreak     = sessionStart;

      for (int i = 0; i < 3; ++i) {
        digitalWrite(BUZZER, HIGH);
        delay(100);
        digitalWrite(BUZZER, LOW);
        delay(100);
      }

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Sessao iniciada");
      lcd.setCursor(0, 1);
      lcd.print("Bons estudos");
      delay(1000);
    }
    return;
  }

  unsigned long now = millis();

  if (now - lastSensorRead >= SENSOR_READ_MS) {
    lastSensorRead = now;

    float t = dht.readTemperature();
    if (!isnan(t)) {
      currentTemp = t;
    }
    currentLum = analogRead(LDR_PIN);
    currentLumPercent = map(currentLum, 0, 1023, 0, 100);

    bool tempOk = (currentTemp >= TEMP_MIN && currentTemp <= TEMP_MAX);
    bool lumOk  = (currentLumPercent >= LUM_MIN && currentLumPercent <= LUM_MAX);

    if (tempOk && lumOk) {
      digitalWrite(LED_GREEN, HIGH);
      digitalWrite(LED_RED, LOW);
    } else {
      digitalWrite(LED_GREEN, LOW);
      digitalWrite(LED_RED, HIGH);
    }

    lcd.clear();
    lcd.setCursor(0, 0);

    DateTime rtcNow = rtc.now();
    if (rtcNow.hour() < 10) lcd.print('0');
    lcd.print(rtcNow.hour());
    lcd.print(':');
    if (rtcNow.minute() < 10) lcd.print('0');
    lcd.print(rtcNow.minute());
    lcd.print(' ');

    lcd.print((int)currentTemp);
    lcd.print((char)223);
    lcd.print('C');

    lcd.setCursor(0, 1);
    lcd.print("L:");
    lcd.print(currentLumPercent);
    lcd.print("% ");
    lcd.print((tempOk && lumOk) ? "OK" : "NOK");
  }

  if (now - lastBreak >= INTERVAL_BREAK) {
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, HIGH);

    for (int i = 0; i < 3; ++i) {
      digitalWrite(BUZZER, HIGH);
      delay(200);
      digitalWrite(BUZZER, LOW);
      delay(100);
    }

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Hora da pausa");
    lcd.setCursor(0, 1);
    lcd.print("Press btn ao voltar");

    while (digitalRead(JOY_SW) == LOW) {
      delay(20);
    }
    while (digitalRead(JOY_SW) == HIGH) {
      delay(20);
    }
    while (digitalRead(JOY_SW) == LOW) {
      delay(20);
    }

    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, HIGH);

    memoryData.pauses++;
    EEPROM.put(MEM_ADDRESS, memoryData);

    lastBreak = millis();

    for (int i = 0; i < 2; ++i) {
      digitalWrite(BUZZER, HIGH);
      delay(100);
      digitalWrite(BUZZER, LOW);
      delay(100);
    }
  }

  if (buttonDown) {
    while (digitalRead(JOY_SW) == LOW) {
      delay(20);
    }
    while (digitalRead(JOY_SW) == HIGH) {
      delay(20);
    }
    while (digitalRead(JOY_SW) == LOW) {
      delay(20);
    }

    sessionActive = false;
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, LOW);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Wellness Reminder");
    lcd.setCursor(0, 1);
    lcd.print("Press btn p iniciar");

    for (int i = 0; i < 2; ++i) {
      digitalWrite(BUZZER, HIGH);
      delay(150);
      digitalWrite(BUZZER, LOW);
      delay(100);
    }
  }
}
