# A DIY dosimeter-radiometer based on Arduino and DP-5V 

# Самодельный дозиметр-радиометр на базе Arduino и ДП-5В

Version: 0.2.0

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

*   Прямое подключение к разъёму наушников

*   LCD 16×2 с I2C (адрес 0x27)

*   Пьезоизлучатель (опционально)

*   Кнопка управления

*   **Согласующая схема** для безопасного подключения дозиметра к Arduino (опционально).

## 🇺🇸 Hardware

Arduino Uno / Nano (ATmega328P)

*   DP-5V military radiation survey meter

*   Direct connection to headphone jack

*   16×2 I2C LCD (address 0x27)

*   Piezo buzzer (optional)

*   Control button

*   **Interface circuit** for safely connecting the dosimeter to the Arduino (optional).

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
> 3.  **Аудиовыход дозиметра ДП-5В НЕ ЯВЛЯЕТСЯ TTL-совместимым сигналом!** Напрямую подключать его к Arduino опасно. Используйте простейшую схему согласования:
>     *   Ограничительный резистор на входе.
>     *   Защитный диод, подключенный к GND.
>     *   Идеальный вариант — простой транзисторный ключ (формирователь импульсов), который будет выдавать чистый 0-5В сигнал.

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
> 3.  **The audio output of the DP-5V dosimeter is NOT a TTL-compatible signal!** Connecting it directly to the Arduino is dangerous. Use a simple interface circuit:
>     *   A current-limiting resistor on the input.
>     *   A protection diode connected to GND.
>     *   The ideal option is a simple transistor switch (pulse shaper) that will output a clean 0-5V signal.

## 🇷🇺 Калибровка

Коэффициент пересчёта CPS → µSv/h зависит от конкретного прибора и условий измерения.

Значение по умолчанию откалибровано по фону с использованием дозиметра ДКГ-03Д совместно с ДП-5В.

Используемый коэффициент:
CPS_TO_USVH = 0.34

## 🇺🇸 Calibration

The CPS to µSv/h conversion factor depends on the specific tube and measurement conditions.

The default value is calibrated to background radiation using a DKG-03D reference dosimeter together with the DP-5V.

Used conversion factor:
CPS_TO_USVH = 0.34

## 🇷🇺 Предупреждение

Данный проект не является сертифицированным измерительным прибором и не должен использоваться для задач, связанных с безопасностью, медициной или радиационным контролем.

## 🇺🇸 Disclaimer

This project is not a certified radiation measurement device.
Do not use it for safety-critical, medical, or regulatory applications.

## License

GNU General Public License v3.0



## Документация / Documentation

### 🇷🇺 Русский

-  [Архитектура проекта](docs/architecture.md)
-  [Блок-схема алгоритма](docs/flowchart.md)
-  [Математическая модель](docs/math_model.md)
-  [Ограничения и допущения](docs/limitations.md)

### us English

-  [Project architecture](docs/architecture.md)
-  [Firmware flowchart](docs/flowchart.md)
-  [Mathematical model](docs/math_model.md)
-  [Limitations and assumptions](docs/limitations.md)
