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

#define GEIGER_PIN 2  // Geiger counter input (interrupt)
#define BUZZER_PIN 7  // Buzzer (optional)
#define BUTTON_PIN 3  // Mode / reset button

LiquidCrystal_I2C lcd(0x27, 16, 2);

/*
============================
Measurement parameters
Параметры измерений
============================
*/

// CPS -> µSv/h conversion factor (background calibrated)
// Коэффициент CPS -> мкЗв/ч (калиброван по фону ДКГ-03Д)
const float CPS_TO_USVH = 0.34;

// B-8 control source conversion
// Пересчёт CPS в условные единицы Б-8
const float CPS_TO_B8 = 60.0;

// Physical dead time of the Geiger tube (in SECONDS!)
// !!! ЗАМЕНИТЕ НА ПАСПОРТНОЕ ЗНАЧЕНИЕ ВАШЕЙ ТРУБКИ !!! (например, 200 мкс = 0.0002 с)
// Физическое мертвое время трубки (в СЕКУНДАХ!)
const float TUBE_DEAD_TIME_S = 0.0002;

// Maximum sliding window duration for the buffer (seconds)
// Максимальная длительность скользящего окна для буфера (сек)
#define WINDOW_SECONDS 30

// Software dead time / debounce (µs)
// Программное мёртвое время (мкс)
#define DEAD_TIME_US 20000

/*
============================
Dynamic Window Parameters
Параметры динамического окна
============================
*/
#define SLOW_WINDOW_SECONDS 30  // Slow window for background stability / Длинное окно для стабильности на фоне
#define FAST_WINDOW_SECONDS 5   // Fast window for quick reaction / Короткое окно для быстрой реакции
#define CPS_THRESHOLD_FAST 3.0  // CPS threshold to switch to fast mode / Порог CPS для перехода в быстрый режим
#define CPS_THRESHOLD_SLOW 1.5  // CPS threshold to switch back to slow mode / Порог CPS для возврата в медленный режим


/*
============================
Button timing
Тайминги кнопки
============================
*/
#define DEBOUNCE_MS 200
#define LONG_PRESS_MS 1500

/*
============================
Global variables
Глобальные переменные
============================
*/

volatile unsigned long pulseCount = 0;
volatile unsigned long lastPulseTime = 0;
volatile bool beepFlag = false;

unsigned long lastSecondMillis = 0;

// Sliding window buffer
unsigned int cpsBuffer[WINDOW_SECONDS] = { 0 };
unsigned int bufferIndex = 0;
unsigned long lastPulseSnapshot = 0;

// Accumulated dose (µSv)
// Накопленная доза (мкЗв)
float accumulatedDose_uSv = 0.0;

// Accumulation time (seconds)
// Время накопления (секунды)
unsigned long accumulationTime_s = 0;

// Button handling
bool lastButtonState = HIGH;
unsigned long buttonPressTime = 0;
unsigned long lastButtonEvent = 0;

// LED flash variables
// Переменные для вспышки светодиода
const int ledPin = LED_BUILTIN;
const unsigned long flashDuration = 20;
unsigned long ledOnTime = 0;
bool ledIsOn = false;

// Dynamic Window State
// Состояние динамического окна
unsigned int currentWindowDuration = SLOW_WINDOW_SECONDS;

// Display mode
// Режим отображения
enum DisplayMode {
  DISPLAY_MAIN,
  DISPLAY_ACCUM
};

DisplayMode displayMode = DISPLAY_MAIN;

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
Reset all measurements
Сброс всех измерений
============================
*/

void resetMeasurements() {
  noInterrupts();
  pulseCount = 0;
  lastPulseSnapshot = 0;
  interrupts();

  for (unsigned int i = 0; i < WINDOW_SECONDS; i++) {
    cpsBuffer[i] = 0;
  }

  bufferIndex = 0;
  accumulatedDose_uSv = 0.0;
  accumulationTime_s = 0;
}

/*
============================
Setup
============================
*/

void setup() {
  pinMode(GEIGER_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(GEIGER_PIN), geigerISR, RISING);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("DP-5 Monitor");
  lcd.setCursor(0, 1);
  lcd.print("ver. 0.3.0");
  delay(2500);
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
Beep & LED flash trigger
Триггер звука и светодиода
============================
*/

  if (beepFlag) {
    digitalWrite(ledPin, HIGH);
    ledIsOn = true;
    ledOnTime = millis();
    tone(BUZZER_PIN, 3000, 5);
    beepFlag = false;
  }

  // LED turn off logic (timer-based)
  // Логика выключения светодиода (срабатывает по таймеру)
  if (ledIsOn && (millis() - ledOnTime >= flashDuration)) {
    digitalWrite(ledPin, LOW);
    ledIsOn = false;
  }

  /*
  ============================
  Button handling
  Обработка кнопки
  ============================
  */

  bool buttonState = digitalRead(BUTTON_PIN);

  // Button press detected
  // Обнаружено нажатие
  if (buttonState == LOW && lastButtonState == HIGH && nowMillis - lastButtonEvent > DEBOUNCE_MS) {
    buttonPressTime = nowMillis;
    lastButtonEvent = nowMillis;
  }

  // Button release detected
  // Обнаружено отпускание
  if (buttonState == HIGH && lastButtonState == LOW) {
    unsigned long pressDuration = nowMillis - buttonPressTime;

    // Long press -> reset all measurements
    // Долгое нажатие -> сброс всех измерений
    if (pressDuration >= LONG_PRESS_MS) {
      resetMeasurements();
    }
    // Short press -> switch display mode
    // Короткое нажатие -> смена экрана
    else {
      displayMode = (displayMode == DISPLAY_MAIN) ? DISPLAY_ACCUM : DISPLAY_MAIN;
    }
  }

  lastButtonState = buttonState;

  /*
  ============================
  Once per second processing
  Обработка раз в секунду
  ============================
  */

  if (nowMillis - lastSecondMillis >= 1000) {
    lastSecondMillis += 1000;

    // Safe pulse counter read
    unsigned long currentCount;
    noInterrupts();
    currentCount = pulseCount;
    interrupts();

    // Pulses in last second
    unsigned int pulsesThisSecond = currentCount - lastPulseSnapshot;
    lastPulseSnapshot = currentCount;

    // Update sliding window buffer
    // Обновляем буфер скользящего окна
    cpsBuffer[bufferIndex] = pulsesThisSecond;
    bufferIndex = (bufferIndex + 1) % WINDOW_SECONDS;


    /*
    ============================
    Dynamic window logic
    Логика динамического окна
    ============================
    */

    // First, calculate a preliminary CPS based on the *current* window to make a decision
    // Сначала рассчитываем "предварительный" CPS по *текущему* окну, чтобы принять решение
    unsigned long pre_windowPulses = 0;
    for (unsigned int i = 0; i < currentWindowDuration; i++) {
      int readIndex = (bufferIndex - 1 - i + WINDOW_SECONDS) % WINDOW_SECONDS;
      pre_windowPulses += cpsBuffer[readIndex];
    }
    float previous_cps = (float)pre_windowPulses / (float)currentWindowDuration;

    // If average CPS exceeds the threshold, switch to fast mode
    // Если средний CPS превысил порог, переключаемся в быстрый режим
    if (previous_cps > CPS_THRESHOLD_FAST) {
      currentWindowDuration = FAST_WINDOW_SECONDS;
    }
    // If average CPS drops below the threshold, switch back to slow mode
    // Если средний CPS упал ниже порога, возвращаемся в медленный режим
    else if (previous_cps < CPS_THRESHOLD_SLOW) {
      currentWindowDuration = SLOW_WINDOW_SECONDS;
    }
    // If CPS is between thresholds, the mode does not change, creating a hysteresis
    // (Если CPS между порогами, режим не меняется, создавая гистерезис)


    /*
    ============================
    Sum pulses in DYNAMIC window
    Сумма импульсов в ДИНАМИЧЕСКОМ окне
    ============================
    */

    // Recalculate the sum, but now for the window of the desired length
    // Пересчитываем сумму, но уже для окна нужной нам длины
    unsigned long windowPulses = 0;
    for (unsigned int i = 0; i < currentWindowDuration; i++) {
      // Go "backwards" through the circular buffer from the current position
      // Идем "назад" по кольцевому буферу от текущей позиции
      int readIndex = (bufferIndex - 1 - i + WINDOW_SECONDS) % WINDOW_SECONDS;
      windowPulses += cpsBuffer[readIndex];
    }

    /*
    ============================
    CPS, CPM (B-8) and dose calculation
    Расчёт CPS, CPM (Б-8) и дозы
    ============================
    */

    // The average is now calculated over the dynamic window
    // Теперь среднее считается по динамическому окну
    float measured_cps = (float)windowPulses / (float)currentWindowDuration;

    // --- Dead Time Compensation / Компенсация мертвого времени ---
    float true_cps = measured_cps / (1.0 - measured_cps * TUBE_DEAD_TIME_S);

    // Use the corrected 'true_cps' for all subsequent calculations
    // Используем скорректированный 'true_cps' для всех последующих расчетов
    float doseRate_uSv = true_cps * CPS_TO_USVH;

    // Accumulate dose ALWAYS
    // Накопление дозы ВСЕГДА
    accumulatedDose_uSv += doseRate_uSv / 3600.0;

    // Accumulate time ALWAYS
    // Время накопления ВСЕГДА
    accumulationTime_s++;

    // B-8 equivalent
    unsigned long b8Value = (unsigned long)(true_cps * CPS_TO_B8);

    // Poisson statistical error (%) is calculated from RAW pulses, so it does not change
    // Статистическая погрешность (%) считается по "сырым" импульсам, поэтому не меняется
    float errorPercent = 0.0;
    if (windowPulses > 0) {
      errorPercent = 100.0 / sqrt(windowPulses);
    }

    // The main 'cps' variable for display can be the corrected one
    // Основную переменную 'cps' для отображения можно сделать скорректированной
    float cps = true_cps;

    /*
    ============================
    LCD output
    Вывод на дисплей
    ============================
    */

    if (displayMode == DISPLAY_MAIN) {

      lcd.setCursor(0, 0);
      lcd.print("CPS:");
      lcd.print(cps, 1);
      lcd.print(" B8:");
      lcd.print(b8Value);
      lcd.print("   ");

      lcd.setCursor(0, 1);
      lcd.print(doseRate_uSv, 2);
      lcd.print("uSv ");
      lcd.print(errorPercent, 1);
      lcd.print("%   ");

    } else {

      // Format accumulation time HH:MM:SS
      // Форматирование времени накопления ЧЧ:ММ:СС
      unsigned long hours = accumulationTime_s / 3600;
      unsigned long minutes = (accumulationTime_s % 3600) / 60;
      unsigned long seconds = accumulationTime_s % 60;

      lcd.setCursor(0, 0);
      if (hours < 10) lcd.print("0");
      lcd.print(hours);
      lcd.print(":");
      if (minutes < 10) lcd.print("0");
      lcd.print(minutes);
      lcd.print(":");
      if (seconds < 10) lcd.print("0");
      lcd.print(seconds);
      lcd.print("        ");

      lcd.setCursor(0, 1);
      lcd.print(accumulatedDose_uSv, 4);
      lcd.print(" uSv       ");
    }
  }
}