# Changelog

Все заметные изменения в этом проекте будут документироваться в этом файле.

Формат основан на [Keep a Changelog](https://keepachangelog.com/ru/1.0.0/).  
Проект не использует строгую семантическую версификацию, версии отражают этапы развития функциональности.
---

## [0.3.0] — 2026-01-02

### Added
- **Dead-time compensation for Geiger tube**
  - Implemented physical dead time correction using the non-paralyzable model.
  - Tube dead time is configurable via `TUBE_DEAD_TIME_S` parameter.
  - All dose rate and CPS calculations now use corrected (true) count rate.

- **Startup screen**
  - On power-up, the device displays its name and firmware version for 2.5 seconds.

- **Dynamic averaging window with hysteresis**
  - Adaptive averaging window length (5 s ↔ 30 s) depending on radiation level.
  - Automatic switch to fast response mode when CPS exceeds 3.0.
  - Automatic return to stable background mode when CPS drops below 1.5.
  - Built-in hysteresis prevents frequent window switching.

---

- **Компенсация мёртвого времени трубки**
  - Реализована коррекция физического мёртвого времени счётчика Гейгера по непарализуемой модели.
  - Паспортное мёртвое время трубки задаётся параметром `TUBE_DEAD_TIME_S`.
  - Все расчёты CPS и мощности дозы выполняются с использованием скорректированной частоты счёта.

- **Экран загрузки**
  - При включении прибора на 2.5 секунды отображается название устройства и версия прошивки.

- **Динамическое окно усреднения с гистерезисом**
  - Адаптивная длина окна усреднения (5 с ↔ 30 с) в зависимости от уровня радиации.
  - Автоматический переход в быстрый режим при CPS > 3.0.
  - Автоматический возврат в фоновый режим при CPS < 1.5.
  - Гистерезис предотвращает частые переключения режимов.

---

### Changed
- CPS, dose rate (µSv/h) and B-8 equivalent are now calculated using:
  - dynamic averaging window;
  - dead-time–corrected count rate.
- Improved accuracy at elevated count rates without degrading background stability.

---

- CPS, мощность дозы (µSv/h) и эквивалент Б-8 теперь рассчитываются:
  - с использованием динамического окна усреднения;
  - с учётом компенсации мёртвого времени трубки.
- Повышена точность измерений при высоких уровнях счёта без ухудшения стабильности на фоне.

---

### Unchanged
- Pulse counting logic, interrupt handling and software dead time.
- Dose accumulation method (time integration remains statistically correct).
- Hardware interface (LCD, buzzer, LED, button behavior).
- Calibration constants CPS → µSv/h and CPS → B-8.

---

- Логика подсчёта импульсов, обработка прерываний и программное мёртвое время.
- Метод накопления дозы (интеграция по времени остаётся статистически корректной).
- Аппаратный интерфейс (LCD, пищалка, светодиод, кнопка).
- Калибровочные коэффициенты CPS → µSv/h и CPS → Б-8.

---

### Notes
- Dead-time compensation becomes significant at elevated count rates and does not affect background measurements.
- Dynamic averaging window affects displayed dose rate only; accumulated dose accuracy is preserved.
- Algorithm behavior is consistent with survey meter practices (IEC 60846-1 methodology).

---

- Компенсация мёртвого времени становится значимой при повышенных уровнях счёта и не влияет на фоновые измерения.
- Динамическое окно влияет только на отображаемую мощность дозы; накопленная доза не искажается.
- Алгоритм соответствует методическим принципам контрольных дозиметрических приборов (IEC 60846-1).


---

## [0.2.2] — 2026-01-01

### Added
- **Dynamic averaging window for CPS calculation**
  - Adaptive sliding window length (5 s ↔ 30 s) depending on radiation level.
  - Automatic switch to fast response mode when CPS exceeds 3.0.
  - Automatic return to stable background mode when CPS drops below 1.5.
  - Built-in hysteresis prevents frequent window switching.
- **Added boot screen:** On power-on, the device name and firmware version are displayed for 2.5 seconds.

- **Динамическое окно усреднения для расчёта CPS**
  - Адаптивная длина окна усреднения (5 с ↔ 30 с) в зависимости от уровня радиации.
  - Автоматический переход в быстрый режим при CPS > 3.0.
  - Автоматический возврат в стабильный фоновый режим при CPS < 1.5.
  - Гистерезис предотвращает частые переключения режима.
- **Добавлен экран загрузки:** При включении прибора на 2.5 секунды отображается его название и версия прошивки.

### Changed
- CPS, мощность дозы (µSv/h), эквивалент Б-8 и статистическая погрешность теперь рассчитываются с использованием **динамической длины окна** вместо фиксированного 30-секундного окна.
- Улучшена реакция прибора на резкие изменения уровня радиации при сохранении стабильных показаний на фоне.

- CPS, dose rate (µSv/h), B-8 equivalent and statistical error are now calculated using a **dynamic averaging window** instead of a fixed 30 s window.
- Improved responsiveness to rapid radiation level changes while maintaining stable background readings.

### Unchanged
- Логика подсчёта импульсов, обработка прерываний и программное мёртвое время.
- Метод накопления дозы (интеграция по времени остаётся статистически корректной).
- Аппаратный интерфейс (LCD, пищалка, светодиод, кнопка).
- Калибровочные коэффициенты и физическая модель измерений.

- Pulse counting logic, interrupt handling and software dead time.
- Dose accumulation method (time-integrated dose remains statistically correct).
- Hardware interface (LCD, buzzer, LED, button behavior).
- Calibration constants and physical measurement model.

### Notes
- Динамическое окно влияет **только на отображаемую мощность дозы**, накопленная доза не искажается.
- Алгоритм соответствует практике контрольных дозиметрических приборов (IEC 60846-1 по методике).

- The dynamic window affects **displayed dose rate only**; accumulated dose accuracy is preserved.
- The algorithm behavior is consistent with control/survey meter practices (IEC 60846-1 methodology).


---

 ## [0.2.1] — 2025-12-28
 ### Added
- Добавлена визуальная индикация каждого регистрируемого импульса (короткая вспышка встроенного светодиода).
- Реализован неблокирующий таймер для автоматического выключения светодиода после вспышки.

### Changed
- Логика флага `beepFlag` теперь одновременно инициирует звуковой сигнал и световую вспышку.

---

## [0.2.0] — 2025-12-28
### Added
- Добавлена кнопка управления режимами отображения.
- Реализованы два режима дисплея:
  - **MAIN** — текущие измерения (CPS, Б-8, µSv/h, статистическая ошибка).
  - **ACCUM** — накопленная доза и время измерения.
- Реализовано накопление дозы (µSv) с корректной интеграцией мощности дозы.
- Добавлен счётчик общего времени измерения.
- Добавлен пересчёт CPS в условные единицы **Б-8**.
- Реализован полный сброс измерений по длинному нажатию кнопки.
- Добавлена относительная статистическая погрешность (%).

### Changed
- Изменён коэффициент пересчёта CPS → µSv/h (0.30 → 0.34).
- Статистическая ошибка теперь отображается в процентах, а не в абсолютных единицах.
- Убрано автоматическое переключение единиц (nSv / µSv / mSv) — используются фиксированные единицы.
- Обновлена логика пользовательского интерфейса для повышения читаемости.
- Код реорганизован в сторону архитектуры полноценного дозиметра.

### Removed
- Удалён автоматический выбор единиц измерения.
- Удалён вывод абсолютной ошибки ±µSv/h.
- Убрана стартовая заставка при включении.

---

## [0.1.0] — 2025-12-27
### Added
- Подсчёт импульсов счётчика Гейгера через аппаратное прерывание.
- Программное мёртвое время для подавления дребезга и ложных срабатываний.
- Скользящее окно (30 секунд) для расчёта CPS.
- Пересчёт CPS в мощность дозы (µSv/h).
- Расчёт статистической ошибки по Пуассону.
- Автоматический выбор единиц (nSv/h, µSv/h, mSv/h).
- Звуковая индикация каждого импульса.
- Отображение данных на LCD 16×2 (I2C).

### Notes
- Версия представляет собой монитор радиационного фона без накопления дозы.
- Предназначена для демонстрации и онлайн-наблюдения.

---
