# Радиационный монитор на базе ДП-5В и Arduino

Самодельный дозиметр-радиометр на базе дозиметра ДП-5В, платформы Arduino и I2C LCD 16x2. Код реализует корректный подсчёт импульсов с использованием скользящего окна для усреднения и расчётом статистической погрешности.

# Radiation Monitor based on DP-5V and Arduino

A DIY dosimeter-radiometer based on the DP-5V military dosimeter, the Arduino platform, and an I2C LCD 16x2. The code implements proper pulse counting using a sliding window for averaging and calculates the statistical error.

## 📌 Ключевые возможности

*   **Подсчёт импульсов** от счётчика Гейгера (CPS - Counts Per Second).
*   **Реальное скользящее окно** на 30 секунд для получения стабильных и точных показаний.
*   **Оценка мощности дозы** с автоматическим выбором единиц (нЗв/ч, мкЗв/ч, мЗв/ч).
*   **Расчёт Пуассоновской статистической погрешности** для оценки достоверности измерения.
*   **Звуковая индикация** каждого зарегистрированного импульса.
*   Вывод информации на стандартный дисплей **I2C LCD 16x2**.
*   Возможность **калибровки** по эталонному дозиметру для повышения точности.

## 📌 Key Features

*   **Pulse counting** from the Geiger tube (CPS - Counts Per Second).
*   **A true 30-second sliding window** to obtain stable and accurate readings.
*   **Dose rate estimation** with automatic unit selection (nSv/h, µSv/h, mSv/h).
*   **Calculation of Poisson statistical error** to assess the measurement's reliability.
*   **Audible indication** for each registered pulse.
*   Information output to a standard **I2C LCD 16x2** display.
*   Option for **calibration** against a reference dosimeter to improve accuracy.

## 🧰 Аппаратные компоненты

*   **Arduino Uno** или любая совместимая плата (Nano, Pro Mini и т.д.).
*   **Дозиметр ДП-5В** с аудиовыходом для съёма импульсов.
*   **LCD 16x2 дисплей с I2C-адаптером** (стандартный адрес `0x27`).
*   **Пьезоизлучатель (Buzzer)** для звуковой индикации (опционально).
*   **Согласующая схема** для безопасного подключения дозиметра к Arduino (резисторы, диоды, транзисторы).

## 🧰 Hardware Components

*   **Arduino Uno** or any compatible board (Nano, Pro Mini, etc.).
*   **DP-5V Dosimeter** with an audio output to capture pulses.
*   **LCD 16x2 display with an I2C adapter** (standard address `0x27`).
*   **Piezo buzzer** for audible indication (optional).
*   **Interface circuit** for safely connecting the dosimeter to the Arduino (resistors, diodes, transistors).

## 🔌 Схема подключения

| Сигнал от модуля                  | Пин Arduino |
| --------------------------------- | ----------- |
| Аудиовыход дозиметра ДП-5В        | `D2 (INT0)` |
| Общий провод (GND)                | `GND`       |
| LCD SDA                           | `A4`        |
| LCD SCL                           | `A5`        |
| Пьезоизлучатель (+)               | `D7`        |
| Пьезоизлучатель (-)               | `GND`       |

> ⚠️ **ВАЖНО! Прочтите перед подключением:**
>
> 1.  **Пин `D2` используется как внешнее прерывание (INT0)**, что гарантирует надёжный подсчёт всех импульсов.
> 2.  Для пина `D2` в коде включен **внутренний подтягивающий резистор (`INPUT_PULLUP`)**. Это означает, что для регистрации импульса вход должен быть "замкнут" на землю (GND).
> 3.  **Аудиовыход дозиметра ДП-5В НЕ ЯВЛЯЕТСЯ TTL-совместимым сигналом!** Напрямую подключать его к Arduino опасно. Используйте простейшую схему согласования:
>     *   Ограничительный резистор на входе.
>     *   Защитный диод, подключенный к GND.
>     *   Идеальный вариант — простой транзисторный ключ (формирователь импульсов), который будет выдавать чистый 0-5В сигнал.

## 🔌 Connection Diagram

| Signal from Module                | Arduino Pin |
| --------------------------------- | ----------- |
| DP-5V Dosimeter Audio Output      | `D2 (INT0)` |
| Common Ground (GND)               | `GND`       |
| LCD SDA                           | `A4`        |
| LCD SCL                           | `A5`        |
| Piezo Buzzer (+)                  | `D7`        |
| Piezo Buzzer (-)                  | `GND`       |

> ⚠️ **IMPORTANT! Read before connecting:**
>
> 1.  **Pin `D2` is used as an external interrupt (INT0)**, which ensures reliable counting of all pulses.
> 2.  The code enables the **internal pull-up resistor (`INPUT_PULLUP`)** for pin `D2`. This means that to register a pulse, the input must be pulled to ground (GND).
> 3.  **The audio output of the DP-5V dosimeter is NOT a TTL-compatible signal!** Connecting it directly to the Arduino is dangerous. Use a simple interface circuit:
>     *   A current-limiting resistor on the input.
>     *   A protection diode connected to GND.
>     *   The ideal option is a simple transistor switch (pulse shaper) that will output a clean 0-5V signal.

## ⚙️ Настройка и калибровка

Все основные параметры настраиваются в начале скетча.

#### Основные параметры

```cpp
#define WINDOW_SECONDS 30
#define DEAD_TIME_US 20000
```

*   `WINDOW_SECONDS` — длительность скользящего окна в секундах. 30 секунд — хороший баланс между скоростью реакции и стабильностью показаний.
*   `DEAD_TIME_US` — программное "мёртвое время" в микросекундах. 20000 мкс (20 мс) — оптимальное значение для большинства счётчиков в дозиметрах ДП-5В для исключения ложных срабатываний.

#### Калибровка мощности дозы

Это **ключевой параметр**, влияющий на точность показаний прибора.

```cpp
const float CPS_TO_USVH = 0.30;
```

> ⚠️ **ВНИМАНИЕ!** Данный коэффициент **НЕ универсален**. Его значение зависит от конкретного экземпляра счётчика Гейгера в дозиметре, его возраста, геометрии измерения и других факторов. Копирование этого значения даст лишь очень приблизительный результат.

**Как откалибровать прибор самостоятельно:**
1.  Поместите ваш прибор и эталонный (проверенный) дозиметр в условия стабильного радиационного фона, вдали от источников.
2.  Запишите показания **эталонного дозиметра** в мкЗв/ч (например, `0.15 мкЗв/ч`).
3.  Включите ваш прибор и дождитесь стабилизации показаний `CPS` (через 1-2 минуты). Запишите это значение (например, `0.51 CPS`).
4.  Рассчитайте ваш персональный коэффициент по формуле:
    **Коэффициент = (Показания_эталона, мкЗв/ч) / (Ваши_показания, CPS)**
    *Пример: 0.15 / 0.51 = 0.294*
5.  Введите полученное значение в переменную `CPS_TO_USVH` и загрузите скетч снова.

## ⚙️ Configuration and Calibration

All main parameters are configured at the beginning of the sketch.

#### Main Parameters

```cpp
#define WINDOW_SECONDS 30
#define DEAD_TIME_US 20000
```

*   `WINDOW_SECONDS` — the duration of the sliding window in seconds. 30 seconds is a good balance between response speed and reading stability.
*   `DEAD_TIME_US` — a software-defined "dead time" in microseconds. 20000 µs (20 ms) is an optimal value for most Geiger tubes in DP-5V dosimeters to prevent false triggers.

#### Dose Rate Calibration

This is the **key parameter** that affects the accuracy of the device's readings.

```cpp
const float CPS_TO_USVH = 0.30;
```

> ⚠️ **ATTENTION!** This conversion factor is **NOT universal**. Its value depends on the specific Geiger tube in your dosimeter, its age, measurement geometry, and other factors. Simply copying this value will yield only a very approximate result.

**How to calibrate the device yourself:**
1.  Place your device and a reference (verified) dosimeter in a stable background radiation environment, away from any sources.
2.  Record the reading of the **reference dosimeter** in µSv/h (e.g., `0.15 µSv/h`).
3.  Turn on your device and wait for the `CPS` reading to stabilize (after 1-2 minutes). Record this value (e.g., `0.51 CPS`).
4.  Calculate your personal conversion factor using the formula:
    **Factor = (Reference_Reading, µSv/h) / (Your_Reading, CPS)**
    *Example: 0.15 / 0.51 = 0.294*
5.  Enter the calculated value into the `CPS_TO_USVH` variable and upload the sketch again.

## 📊 Интерпретация показаний

Дисплей прибора выводит информацию в две строки:

```
CPS: 0.51
0.15 +- 0.04 uSv/h
```

*   **Верхняя строка**: `CPS: 0.51` — среднее количество импульсов в секунду (Counts Per Second), усреднённое за последние 30 секунд (длина скользящего окна).
*   **Нижняя строка**: `0.15 +- 0.04 uSv/h` — рассчитанная мощность дозы.
    *   `0.15` — среднее значение мощности дозы.
    *   `+- 0.04` — статистическая погрешность измерения. Чем выше уровень радиации, тем меньше будет относительная погрешность.
    *   `uSv/h` — единицы измерения (микрозиверт в час). Выбираются автоматически (nSv/h, uSv/h, mSv/h).

## 📊 Interpreting the Readings

The device's display shows information on two lines:

```
CPS: 0.51
0.15 +- 0.04 uSv/h
```

*   **Top line**: `CPS: 0.51` — the average number of counts per second, averaged over the last 30 seconds (the sliding window duration).
*   **Bottom line**: `0.15 +- 0.04 uSv/h` — the calculated dose rate.
    *   `0.15` — the average dose rate value.
    *   `+- 0.04` — the statistical error of the measurement. The higher the radiation level, the smaller the relative error will be.
    *   `uSv/h` — the unit of measurement (microsieverts per hour). It is selected automatically (nSv/h, uSv/h, mSv/h).

## ⚠️ Важные ограничения и предупреждения

Этот проект создан в образовательных и исследовательских целях.

*   `❌` **Это НЕ профессиональный или сертифицированный измерительный прибор.**
*   `❌` **НЕ используйте его для оценки реальных рисков для здоровья**, контроля облучения персонала или проверки безопасности продуктов питания.
*   `❌` **Показания прибора НЕ могут служить основанием для принятия решений о защите жизни и здоровья.**
*   **Дозиметр ДП-5В** имеет сильную **энергетическую зависимость** и **нелинейный отклик** при высоких мощностях дозы. Пересчёт его показаний в зиверты всегда является приблизительным.

**Для чего подходит этот прибор:**
*   Сравнение относительных уровней радиации в разных местах.
*   Поиск и обнаружение источников ионизирующего излучения.
*   Изучение принципов работы дозиметрических приборов.
*   Эксперименты в области физики и электроники.

## ⚠️ Important Limitations and Warnings

This project is created for educational and research purposes.

*   `❌` **This is NOT a professional or certified measuring instrument.**
*   `❌` **DO NOT use it to assess real health risks**, monitor personnel exposure, or check food safety.
*   `❌` **The device's readings CANNOT be used as a basis for making decisions about protecting life and health.**
*   The **DP-5V dosimeter** has a strong **energy dependence** and a **non-linear response** at high dose rates. Converting its readings to Sieverts is always an approximation.

**What this device is suitable for:**
*   Comparing relative radiation levels in different places.
*   Searching for and detecting sources of ionizing radiation.
*   Studying the principles of how dosimetric instruments work.
*   Experiments in physics and electronics.

## 🚀 Возможные доработки

*   Подсчёт и отображение накопленной дозы.
*   Реализация пороговой сигнализации (тревоги) при превышении заданного уровня.
*   Логирование данных в Serial порт или на SD-карту для построения графиков.

## 🚀 Possible Improvements

*   Calculate and display the accumulated dose.
*   Implement a threshold alarm for when a set level is exceeded.
*   Log data to the Serial port or an SD card for plotting graphs.

## 📜 Лицензия и ответственность

Вы используете данный проект и код на свой страх и риск. Автор не несёт ответственности за любые последствия, связанные со сборкой и эксплуатацией данного устройства.

## 📜 License and Disclaimer

You use this project and code at your own risk. The author is not responsible for any consequences related to the assembly and operation of this device.