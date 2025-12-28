/*
 * DP-5V Arduino Geiger Counter Firmware
 *
 * Copyright (C) 2025 Arduino-DP5V-Monitor
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

/*
  ============================
  Hardware configuration
  Аппаратная конфигурация
  ============================
*/

#define GEIGER_PIN 2        // Interrupt pin (D2 on Arduino Uno)
#define BUZZER_PIN 7        // Buzzer (optional)

LiquidCrystal_I2C lcd(0x27, 16, 2);

/*
  ============================
  Measurement parameters
  Параметры измерения
  ============================
*/

// Conversion factor CPS -> uSv/h
// Calibrated to background using DKG-03D
// Коэффициент CPS -> мкЗв/ч
// Откалиброван по фону прибором ДКГ-03Д
const float CPS_TO_USVH = 0.30;

// Sliding window duration (seconds)
// Длительность скользящего окна (секунды)
#define WINDOW_SECONDS 30

// Dead time for audio output debouncing (microseconds)
// Мёртвое время для подавления дребезга (мкс)
#define DEAD_TIME_US 20000

/*
  ============================
  Global variables
  Глобальные переменные
  ============================
*/

// Total accepted pulses
// Общее число принятых импульсов
volatile unsigned long pulseCount = 0;

// Time of last accepted pulse (micros)
// Время последнего принятого импульса
volatile unsigned long lastPulseTime = 0;

// Beep flag
// Флаг звукового щелчка
volatile bool beepFlag = false;

// Timing
// Тайминг
unsigned long lastSecondMillis = 0;

// Sliding window buffer (pulses per second)
// Буфер скользящего окна (импульсы за секунду)
unsigned int cpsBuffer[WINDOW_SECONDS] = {0};

// Current buffer index
// Текущий индекс буфера
unsigned int bufferIndex = 0;

// Snapshot of pulse counter for per-second calculation
// Снимок счётчика для расчёта за секунду
unsigned long lastPulseSnapshot = 0;

/*
  ============================
  Interrupt Service Routine
  Обработчик прерывания
  ============================
*/

void geigerISR() {
  unsigned long now = micros();

  // Software dead time (anti-bounce)
  // Программное мёртвое время (антидребезг)
  if (now - lastPulseTime > DEAD_TIME_US) {
    pulseCount++;
    lastPulseTime = now;
    beepFlag = true;
  }
}

/*
  ============================
  Setup
  ============================
*/

void setup() {
  pinMode(GEIGER_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(GEIGER_PIN), geigerISR, RISING);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("DP-5B Monitor");
  lcd.setCursor(0, 1);
  lcd.print("Initializing");
  delay(2000);
  lcd.clear();
}

/*
  ============================
  Main loop
  ============================
*/

void loop() {

  unsigned long nowMillis = millis();

  /*
    ============================
    Beep outside ISR
    Пищалка вне прерывания
    ============================
  */

  if (beepFlag) {
    tone(BUZZER_PIN, 3000, 5);
    beepFlag = false;
  }

  /*
    ============================
    Once per second processing
    Обработка раз в секунду
    ============================
  */

  if (nowMillis - lastSecondMillis >= 1000) {
    lastSecondMillis += 1000;

    /*
      ============================
      Safe pulse counter read
      Безопасное чтение счётчика
      ============================
    */

    unsigned long currentCount;
    noInterrupts();
    currentCount = pulseCount;
    interrupts();

    /*
      ============================
      Pulses per last second
      Импульсы за последнюю секунду
      ============================
    */

    unsigned int pulsesThisSecond = currentCount - lastPulseSnapshot;
    lastPulseSnapshot = currentCount;

    /*
      ============================
      Sliding window update
      Обновление скользящего окна
      ============================
    */

    cpsBuffer[bufferIndex] = pulsesThisSecond;
    bufferIndex = (bufferIndex + 1) % WINDOW_SECONDS;

    /*
      ============================
      Sum pulses in window
      Сумма импульсов в окне
      ============================
    */

    unsigned long windowPulses = 0;
    for (unsigned int i = 0; i < WINDOW_SECONDS; i++) {
      windowPulses += cpsBuffer[i];
    }

    /*
      ============================
      CPS and dose calculation
      Расчёт CPS и дозы
      ============================
    */

    float cps = windowPulses / (float)WINDOW_SECONDS;
    float doseRate_uSv = cps * CPS_TO_USVH;

    /*
      ============================
      Poisson statistical error
      Статистическая погрешность (Пуассон)
      ============================
    */

    float error_uSv = 0.0;
    if (windowPulses > 0) {
      error_uSv = doseRate_uSv / sqrt(windowPulses);
    }

    /*
      ============================
      Auto unit selection
      Автоматический выбор единиц
      ============================
    */

    float displayValue = doseRate_uSv;
    float displayError = error_uSv;
    const char* unit = "uSv/h";

    // Switch to nSv/h for low dose rates
    // Переход в нЗв/ч для малых доз
    if (doseRate_uSv < 0.05) {
      displayValue = doseRate_uSv * 1000.0;
      displayError = error_uSv * 1000.0;
      unit = "nSv/h";
    } 
    else if (doseRate_uSv >= 1000.0) {
      displayValue = doseRate_uSv / 1000.0;
      displayError = error_uSv / 1000.0;
      unit = "mSv/h";
    }

    /*
      ============================
      LCD output
      Вывод на дисплей
      ============================
    */

    lcd.setCursor(0, 0);
    lcd.print("CPS:");
    lcd.print(cps, 2);
    lcd.print("     ");

    lcd.setCursor(0, 1);
    lcd.print(displayValue, 2);
    lcd.print(" +-");
    lcd.print(displayError, 2);
    lcd.print(" ");
    lcd.print(unit);
    lcd.print(" ");
  }
}
