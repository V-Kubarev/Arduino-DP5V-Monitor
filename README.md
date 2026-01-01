# A DIY dosimeter-radiometer based on Arduino and DP-5V 

# Самодельный дозиметр-радиометр на базе Arduino и ДП-5В

[Version: 0.2.0](firmware/arduino/geiger_0.2.0/geiger_0.2.0.ino)

## 🇷🇺 Описание

Данная прошивка предназначена для работы с армейским дозиметром ДП-5В и платой Arduino (Uno / Nano).

**ВОЗМОЖНО** подключение Arduino напрямую к разъёму наушников ДП-5В(без гарантий).

С этого выхода снимаются импульсы счётчика Гейгера, которые обрабатываются с помощью аппаратного прерывания.

Для уменьшения ошибок счёта, вызванных дребезгом, повторными срабатываниями и помехами, в коде реализован программный антидребезг (мёртвое время).

Прошивка выполняет расчёт текущего фона, мощности дозы и статистической погрешности, а также отображает данные на LCD-дисплее.

Проект предназначен для учебного и экспериментального использования.

## 🇺🇸 Description

This firmware is designed to work with the DP-5V military radiation survey meter and an Arduino board (Uno / Nano).

**IT IS POSSIBLE** to connect Arduino directly to the DP-5V headphone jack (no guarantees).

Geiger counter pulses are taken from this output and processed using a hardware interrupt.

To reduce counting errors caused by bounce, noise, and repeated triggers, a software dead time (anti-bounce filter) is implemented.

The firmware calculates background radiation, dose rate, and statistical error, and displays the data on an LCD.

This project is intended for educational and experimental use.

## 🇷🇺 Основные возможности

*   Подсчёт импульсов через аппаратное прерывание

*   Скользящее окно CPS (30 секунд)

*   Расчёт мощности дозы (µSv/h)

*   Подсчёт накопленной дозы (µSv)

*   Учёт времени накопления

*   Оценка статистической погрешности (Пуассон)

*   Звуковой щелчок на каждый импульс

*   Программный антидребезг импульсов
  
*   Вывод информации на стандартный дисплей **I2C LCD 16x2**.

*   Возможность **калибровки** по эталонному дозиметру для повышения точности.

## 🇺🇸 Key Features

*   Hardware interrupt pulse counting

*   30-second sliding CPS window

*   Dose rate calculation (µSv/h)

*   Accumulated dose integration

*   Measurement time tracking

*   Poisson statistical error estimation

*   Audible click for each pulse

*   Software dead time filtering

*   Information output to a standard **I2C LCD 16x2** display.

*   Option for **calibration** against a reference dosimeter to improve accuracy.

## 🇷🇺 Аппаратная часть

*   Arduino Uno / Nano (ATmega328P)

*   Армейский дозиметр ДП-5В

*   Прямое подключение к разъёму наушников (на свой страх и риск)

*   LCD 16×2 с I2C (адрес 0x27)

*   Пьезоизлучатель (опционально)

*   Кнопка управления

*   **Согласующая схема** для безопасного подключения дозиметра к Arduino (желательно).

## 🇺🇸 Hardware

Arduino Uno / Nano (ATmega328P)

*   DP-5V military radiation survey meter

*   Direct connection to headphone jack (at your own risk)

*   16×2 I2C LCD (address 0x27)

*   Piezo buzzer (optional)

*   Control button

*   **Interface circuit** for safely connecting the dosimeter to the Arduino (desirable).

## 🇷🇺 Распиновка

*   Импульсы ДП-5В: D2 (INT0)
*   Кнопка: D3
*   Пищалка: D7
*   LCD SDA: A4
*   LCD SCL: A5

> ⚠️ **ВАЖНО! Прочтите перед подключением:**
>
> 1.  **Пин `D2` используется как внешнее прерывание (INT0)**, что гарантирует надёжный подсчёт всех импульсов.
> 2.  Для пина `D2` в коде включен **внутренний подтягивающий резистор (`INPUT_PULLUP`)**. Это означает, что для регистрации импульса вход должен быть "замкнут" на землю (GND).
> 3.  **Аудиовыход дозиметра ДП-5В НЕ ЯВЛЯЕТСЯ TTL-совместимым сигналом!** Для надежной и стабильной работы прибора рекомендуется использовать простейшую схему согласования (например, на одном транзисторе). Ее задача — усилить слабый импульс и превратить его в четкий цифровой сигнал (0-5В), который Arduino гарантированно распознает.

## 🇺🇸 Pin Configuration

*   Geiger pulse input: D2 (INT0)
*   Button: D3
*   Buzzer: D7
*   LCD SDA: A4
*   LCD SCL: A5

> ⚠️ **IMPORTANT! Read before connecting:**
>
> 1.  **Pin `D2` is used as an external interrupt (INT0)**, which ensures reliable counting of all pulses.
> 2.  The code enables the **internal pull-up resistor (`INPUT_PULLUP`)** for pin `D2`. This means that to register a pulse, the input must be pulled to ground (GND).
> 3.  **The audio output of the DP-5V dosimeter is NOT a TTL-compatible signal!** For reliable and stable operation of the device, it is recommended to use a simple matching circuit (for example, one with a single transistor). Its purpose is to amplify a weak pulse and convert it into a clear digital signal (0-5V) that the Arduino is guaranteed to recognize.

## 🇷🇺 Калибровка

Коэффициент пересчёта CPS → µSv/h зависит от конкретного прибора и условий измерения.

Значение по умолчанию откалибровано по фону с использованием дозиметра ДКГ-03Д совместно с ДП-5В.

Используемый коэффициент:
CPS_TO_USVH = 0.34

> ⚠️ **ВНИМАНИЕ!** Данный коэффициент **НЕ универсален**. Его значение зависит от конкретного экземпляра счётчика Гейгера в дозиметре, его возраста, геометрии измерения и других факторов. Копирование этого значения даст лишь очень приблизительный результат.

**Как откалибровать прибор самостоятельно:**
1.  Поместите ваш прибор и эталонный (проверенный) дозиметр в условия стабильного радиационного фона, вдали от источников.
2.  Запишите показания **эталонного дозиметра** в мкЗв/ч (например, `0.15 мкЗв/ч`).
3.  Включите ваш прибор и дождитесь стабилизации показаний `CPS` (через 1-2 минуты). Запишите это значение (например, `0.51 CPS`).
4.  Рассчитайте ваш персональный коэффициент по формуле:
    **Коэффициент = (Показания_эталона, мкЗв/ч) / (Ваши_показания, CPS)**
    *Пример: 0.15 / 0.51 = 0.294*
5.  Введите полученное значение в переменную `CPS_TO_USVH` и загрузите скетч снова.

## 🇺🇸 Calibration

The CPS to µSv/h conversion factor depends on the specific tube and measurement conditions.

The default value is calibrated to background radiation using a DKG-03D reference dosimeter together with the DP-5V.

Used conversion factor:
CPS_TO_USVH = 0.34

> ⚠️ **ATTENTION!** This conversion factor is **NOT universal**. Its value depends on the specific Geiger tube in your dosimeter, its age, measurement geometry, and other factors. Simply copying this value will yield only a very approximate result.

**How to calibrate the device yourself:**
1.  Place your device and a reference (verified) dosimeter in a stable background radiation environment, away from any sources.
2.  Record the reading of the **reference dosimeter** in µSv/h (e.g., `0.15 µSv/h`).
3.  Turn on your device and wait for the `CPS` reading to stabilize (after 1-2 minutes). Record this value (e.g., `0.51 CPS`).
4.  Calculate your personal conversion factor using the formula:
    **Factor = (Reference_Reading, µSv/h) / (Your_Reading, CPS)**
    *Example: 0.15 / 0.51 = 0.294*
5.  Enter the calculated value into the `CPS_TO_USVH` variable and upload the sketch again.

## 🇷🇺 Предупреждение

Данный проект не является сертифицированным измерительным прибором и не должен использоваться для задач, связанных с безопасностью, медициной или радиационным контролем.

**Для чего подходит этот прибор:**
*   Сравнение относительных уровней радиации в разных местах.
*   Поиск и обнаружение источников ионизирующего излучения.
*   Изучение принципов работы дозиметрических приборов.
*   Эксперименты в области физики и электроники.

Вы используете данный проект и код на свой страх и риск. Автор не несёт ответственности за любые последствия, связанные со сборкой и эксплуатацией данного устройства.

## 🇺🇸 Disclaimer

This project is not a certified radiation measurement device.
Do not use it for safety-critical, medical, or regulatory applications.

**What this device is suitable for:**
*   Comparing relative radiation levels in different places.
*   Searching for and detecting sources of ionizing radiation.
*   Studying the principles of how dosimetric instruments work.
*   Experiments in physics and electronics.

You use this project and code at your own risk. The author is not responsible for any consequences related to the assembly and operation of this device.

## License

GNU General Public License v3.0

---

## Документация / Documentation

### 🇷🇺 Русский

-  [Принципы работы прошивки](docs/SOFTWARE_RU.md)
-  [Архитектура проекта](docs/architecture.md)
-  [Блок-схема алгоритма](docs/flowchart.md)
-  [Математическая модель](docs/math_model.md)
-  [Ограничения и допущения](docs/limitations.md)
-  [Список возможных улучшений](docs/TODO.md)

### us English

-  [Operating principles of firmware](docs/SOFTWARE_EN.md)
-  [Project architecture](docs/architecture.md)
-  [Firmware flowchart](docs/flowchart.md)
-  [Mathematical model](docs/math_model.md)
-  [Limitations and assumptions](docs/limitations.md)
