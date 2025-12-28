#include <Wire.h>
#include <LiquidCrystal_I2C.h>

/*
  ============================
  Hardware configuration
  Аппаратная конфигурация
  ============================
*/

#define GEIGER_PIN 2        // Geiger counter input (interrupt)
#define BUZZER_PIN 7        // Buzzer (optional)
#define BUTTON_PIN 3        // Mode / reset button

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

// Sliding window duration (seconds)
// Длительность скользящего окна (сек)
#define WINDOW_SECONDS 30

// Software dead time / debounce (µs)
// Программное мёртвое время (мкс)
#define DEAD_TIME_US 20000

/*
  ============================
  Button timing
  Тайминги кнопки
  ============================
*/

#define DEBOUNCE_MS   200
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
unsigned int cpsBuffer[WINDOW_SECONDS] = {0};
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

  attachInterrupt(digitalPinToInterrupt(GEIGER_PIN), geigerISR, RISING);

  lcd.init();
  lcd.backlight();
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
    Button handling
    Обработка кнопки
    ============================
  */

  bool buttonState = digitalRead(BUTTON_PIN);

  // Button press detected
  // Обнаружено нажатие
  if (buttonState == LOW && lastButtonState == HIGH &&
      nowMillis - lastButtonEvent > DEBOUNCE_MS) {
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

    // Update sliding window
    cpsBuffer[bufferIndex] = pulsesThisSecond;
    bufferIndex = (bufferIndex + 1) % WINDOW_SECONDS;

    // Sum window pulses
    unsigned long windowPulses = 0;
    for (unsigned int i = 0; i < WINDOW_SECONDS; i++) {
      windowPulses += cpsBuffer[i];
    }

    // CPS and dose rate
    float cps = windowPulses / (float)WINDOW_SECONDS;
    float doseRate_uSv = cps * CPS_TO_USVH;

    // Accumulate dose ALWAYS
    // Накопление дозы ВСЕГДА
    accumulatedDose_uSv += doseRate_uSv / 3600.0;

    // Accumulate time ALWAYS
    // Время накопления ВСЕГДА
    accumulationTime_s++;

    // B-8 equivalent
    unsigned long b8Value = (unsigned long)(cps * CPS_TO_B8);

    // Poisson statistical error (%)
    float errorPercent = 0.0;
    if (windowPulses > 0) {
      errorPercent = 100.0 / sqrt(windowPulses);
    }

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