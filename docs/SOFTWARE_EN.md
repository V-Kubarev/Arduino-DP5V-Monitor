# Firmware Operation Principles

---

## Pulse Counting Principle: Hardware Interrupts

Reliable counting of every pulse from the Geiger counter is the most fundamental task of a dosimeter. To solve this, the project uses a hardware interrupt mechanism, which is the most accurate and efficient way to register fast, asynchronous events.

### Problem: Unpredictability and Short Duration of Pulses

Pulses from a Geiger counter have two characteristics that make them difficult to count:

* **Randomness:** They can occur at any moment in time.
* **Short duration:** The length of a single pulse is very small (on the order of microseconds).

If we tried to “catch” pulses by simply reading the pin state in the main `loop()` (a method called polling), we would almost certainly miss most of them. At the moment a short pulse arrives, the microcontroller may be busy with other tasks: updating the display, performing calculations, or processing a button press.

### Solution: Using Hardware Interrupts

A hardware interrupt can be imagined as an “emergency phone call” for the microcontroller.

When a specified event occurs on a dedicated interrupt pin (for example, a voltage change), the microcontroller immediately stops executing the current code in `loop()`, runs a special short handler function (ISR – Interrupt Service Routine), and then returns exactly to the point where it was interrupted.

This guarantees that not a single pulse, even the shortest one, will be missed, no matter what the main program loop is doing at that moment.

### Implementation Principle in Code

#### 1. Configuration in `setup()`

In the `setup()` function, we “tell” the microcontroller which pin to listen to and which function to call.
```
// Configure the pin as an input with a pull-up resistor to Vcc.
// In the normal state it will be HIGH (logic 1).
pinMode(GEIGER_PIN, INPUT_PULLUP);

// “Attach” an interrupt to this pin
attachInterrupt(digitalPinToInterrupt(GEIGER_PIN), geigerISR, RISING);
```

Let’s break down the `attachInterrupt` command:

* `digitalPinToInterrupt(GEIGER_PIN)`: Converts the pin number (D2) into an internal interrupt number (INT0).
* `geigerISR`: The name of the function that will be called when the interrupt triggers.
* `RISING`: The trigger condition. The interrupt will occur when the voltage on the pin changes from low (LOW) to high (HIGH). This happens when the pulse from the signal-conditioning circuit ends and the pin returns to its normal state due to the pull-up resistor.

#### 2. Interrupt Handler — `geigerISR()`

This is the actual “emergency” function. It must be as short and fast as possible. You must not use `delay()` or long operations inside it.
```
void geigerISR() {
// ... (check for software dead time) ...

 // 1. Increment the total pulse counter
 pulseCount++;

 // 2. Set a flag for sound indication in the main loop
 beepFlag = true;
}

```
All it does is increment the `pulseCount` counter and set the `beepFlag` flag so that the main `loop()` knows it needs to make a sound and blink an LED.

Variables that are used both in the interrupt and in the main loop (`pulseCount`, `beepFlag`) must be declared with the `volatile` keyword. This tells the compiler that the variable’s value can change at any time and prevents certain optimizations that could break the logic.

#### 3. Safe Reading of the Counter in `loop()`

The variable `pulseCount` has type `unsigned long` and occupies 4 bytes. Reading 4 bytes is not an atomic (instantaneous) operation. There is a tiny chance that an interrupt could occur right in the middle of reading this variable, resulting in an incorrect value.

To avoid this, before reading `pulseCount` we temporarily disable all interrupts, and immediately after reading, we re-enable them.
```
unsigned long currentCount;

noInterrupts(); // Disable interrupts
currentCount = pulseCount; // Safely read the value
interrupts(); // Immediately re-enable interrupts
```

This guarantees that the value of `pulseCount` is read correctly and without corruption.

---

## Principle of Software “Dead Time”

To ensure high pulse counting accuracy, the firmware implements a software “dead time” mechanism. Its main purpose is to filter out false triggers caused by the characteristics of the analog signal path of the DP-5V dosimeter.

### Problem: “Dirty” Signal from the Audio Output

The analog circuitry of the DP-5V, which generates audible clicks for headphones, was not designed to produce a clean digital signal. As a result, a single real decay event registered by the Geiger counter may produce not one clean pulse at the audio output, but a more complex signal:

* A main pulse followed by a series of damped oscillations or “ringing”.
* “Bounce” or several fast consecutive peaks.

These damped oscillations, which can be called the “tails” of the main pulse, have enough amplitude to trigger the Arduino’s hardware interrupt multiple times. This would cause a single real event to be counted as 2, 3, or even more pulses, artificially inflating the dose rate readings.

### Solution: Software “Dead Time” to Cut Off the “Tails”

To combat this, the code introduces a forced “pause” after registering each valid pulse.

> **Important to understand:** this is not the physical dead time of the Geiger counter itself (which is related to the recovery of the inert gas in the tube). It is purely a software filter created to clean up the input signal and cut off its “tails”.

### Filter Operating Principle

After the program registers the first pulse “edge” and counts it, it stops responding to any subsequent changes on the interrupt pin for a short, strictly defined period of time.

### Implementation in Code

This logic is fully implemented inside the interrupt handler function `geigerISR()`.

#### 1. Constant Definition

At the beginning of the code, the duration of the “insensitive period” is defined in microseconds.

```
   // 20,000 microseconds = 20 milliseconds
   #define DEAD_TIME_US 20000
```

20 ms is a time that is definitely longer than all “tails” and bounce effects, but still short enough not to miss the next real pulse at moderate radiation levels.

#### 2. Logic Inside `geigerISR()`

```
    void geigerISR() {
      // Get the current time in microseconds
      unsigned long now = micros();

      // Check whether enough time has passed since the last COUNTED pulse
      if (now - lastPulseTime > DEAD_TIME_US) {

        // --- This block executes only if > 20 ms have passed ---

        // 1. Increment the counter
        pulseCount++;

        // 2. UPDATE THE TIME OF THE LAST COUNTED PULSE
        lastPulseTime = now;

        // 3. Set a flag for sound/light
        beepFlag = true;
      }
      // If LESS than 20 ms have passed since the last counted pulse,
      // the if condition is not met, and the function simply exits,
      // completely IGNORING the current interrupt trigger.
    }
```

Thus, this simple but effective filter ensures that only the first edge of the “dirty” signal is counted, and all subsequent “tails” are ignored, ensuring counting accuracy.

---

## Sliding Window Algorithm

To obtain stable and reliable dose rate readings, the project uses a data averaging algorithm based on a sliding window. Below is a detailed description of how it works.

### Problem: The Random Nature of Radiation

Radioactive decay is a stochastic (random) process. Even with a constant radiation background, the number of particles registered by the detector per unit time constantly fluctuates. Measuring counts per second (CPS) based only on data from the last second would lead to the following problems:

* **Unstable readings:** Values on the display would constantly “jump” (for example, 0, 2, 1, 0, 3...), making it impossible to properly assess the current level.
* **Low reliability:** A one-second measurement is not statistically significant, especially at low radiation levels.

### Solution: Sliding Window Averaging

To solve this problem, data is averaged over a longer, but continuously updated time interval. The sliding window algorithm implements exactly this approach.

**Core idea:** At any given moment, we operate on an average value calculated from data over the last N seconds (in this project, N = 30). Each new second, the newest data is added to the averaging set, and the oldest data (from N+1 seconds ago) is discarded. Thus, the measurement window “slides” along the time axis, always remaining up to date.

### Implementation Principle in Code

For efficient implementation, a circular (ring) buffer is used.

#### 1. Circular Buffer

The code creates an array that stores the number of pulses registered in each of the last 30 seconds.

```
#define WINDOW_SECONDS 30
unsigned int cpsBuffer[WINDOW_SECONDS] = {0};
```

This array can be imagined as 30 memory cells, each corresponding to one second.

#### 2. Per-Second Update

Once per second, the main loop executes a block of code responsible for updating the data.

#### 3. Writing New Data

First, the number of pulses that occurred during the last second is calculated, and then this value is written into the current buffer cell. The index of the current cell is stored in the variable `bufferIndex`.
```
// Calculate pulses for the past second
unsigned int pulsesThisSecond = currentCount - lastPulseSnapshot;
// Write them to the current cell
cpsBuffer[bufferIndex] = pulsesThisSecond;
```

#### 4. Window Shift

After writing the new data, the `bufferIndex` pointer is moved to the next cell. To “wrap” the buffer, the modulo operator (%) is used.
```
bufferIndex = (bufferIndex + 1) % WINDOW_SECONDS;
```


When `bufferIndex` reaches the value 29, on the next iteration (29 + 1) % 30 results in 0. The pointer returns to the beginning of the array, and new data starts overwriting the oldest values. This is exactly how the “sliding” is implemented — old data is automatically “forgotten”.

#### 5. Average Value Calculation (CPS)

Each second, after updating the buffer, the program recalculates the sum of all values in the 30 cells.
```
unsigned long windowPulses = 0;
for (unsigned int i = 0; i < WINDOW_SECONDS; i++) {
windowPulses += cpsBuffer[i];
}
```



The resulting value `windowPulses` is the total number of pulses over the last 30 seconds. To find the average value per second (CPS), this sum is divided by the window duration.

```
float cps = windowPulses / (float)WINDOW_SECONDS;
```

This averaged CPS value is then used for all further calculations (dose rate, error, etc.).

### Advantages of This Approach

* **Stability:** The algorithm effectively smooths random fluctuations, providing the user with a stable and easy-to-read value.
* **Relevance:** Unlike simple accumulation, the method uses only fresh data. If the radiation situation changes (for example, you bring the device closer to a source), the readings will smoothly and predictably update to the new level within 30 seconds.

---

## Principle of Converting CPS to Dose Rate (µSv/h)

One of the central algorithms of the dosimeter is converting “raw” data from the Geiger counter into commonly accepted measurement units that reflect the biological impact of radiation.

### Task: From “Clicks” to Biological Impact

By itself, a Geiger counter is just an “event counter”. It registers how many times an ionizing particle passes through it and outputs the corresponding number of electrical pulses. The CPS value (Counts Per Second) is simply the “number of pulses per second”.

However, this value does not indicate the degree of biological impact on a human. For this purpose, the system unit sievert (and its derivatives, such as microsieverts per hour, µSv/h) is used, which estimates the equivalent dose of radiation.

The task of the algorithm is to convert a physical quantity (CPS) into a biologically meaningful one (µSv/h).

### Problem: Non-Universality of Conversion

There is no single universal formula for converting CPS to µSv/h. This conversion is the biggest challenge and source of error in any dosimeter. The conversion factor strongly depends on two main factors:

* **Type and sensitivity of the Geiger counter:** A larger tube will produce more pulses at the same radiation level than a smaller one. Different fill gases and cathode materials also affect sensitivity.
* **Energy of the detected radiation:** The counter has different detection efficiency for gamma photons of different energies. For example, it may “see” radiation from Cesium-137 well, but “see” radiation from Cobalt-60 worse.

Imagine trying to estimate roof damage from precipitation. Simply counting “impacts per second” (analogous to CPS) is not enough. You need to know what exactly is falling: raindrops or fist-sized hailstones (analogous to different radiation energies).

### Implementation in Code: Linear Calibration Coefficient

For simplicity, in this project, as in most consumer dosimeters, a linear calibration coefficient is used. This approach assumes that we are measuring radiation with a known or “average” energy spectrum (for example, natural background radiation).

In code, this is implemented very simply:
```
// Coefficient defining how many "µSv/h" are in one "CPS"
const float CPS_TO_USVH = 0.34;

// ...

// Dose rate calculation
float doseRate_uSv = cps * CPS_TO_USVH;

```

This formula assumes that each registered “average” pulse per second (CPS) contributes the same predefined amount to the final dose rate.

### Importance of Calibration

The number 0.34 in the code is not a random value “pulled out of thin air”. It is the calibration coefficient obtained experimentally.

The calibration process described in `README.md` is as follows:

* Two devices are taken: our homemade dosimeter and a reference professional dosimeter (in this case, the DKG-03D).
* Both devices are placed in identical conditions (for example, measuring natural background).
* Their readings are recorded: our device in CPS, and the reference device in µSv/h.
* The coefficient is calculated using a simple formula:
  > Coefficient = (Reference reading, µSv/h) / (Our device reading, CPS)

That is why, to achieve maximum accuracy, each assembled device should ideally be calibrated individually, since even two identical Geiger counters may have slightly different sensitivities.

---

## Algorithm for Calculating Statistical Error

In addition to the dose rate value itself, the device also calculates and displays the relative statistical measurement error as a percentage. This parameter shows how much the current readings can be “trusted”.

### Problem: Uncertainty of Random Events

Radioactive decay is a random (stochastic) process. This means it is impossible to predict exactly when the next decay will occur. Because of this randomness, any measurement based on counting events has inherent uncertainty.

For example, a reading of 0.24 µSv/h by itself is not very informative. It is much more useful to know whether it is 0.24 ± 0.50 µSv/h (very unreliable measurement) or 0.24 ± 0.01 µSv/h (very reliable measurement). Statistical error provides exactly this reliability estimate.

### Theoretical Basis: Poisson Statistics

Random decay processes are well described by the Poisson distribution. One of the key conclusions of this theory states:

> If N random events are registered over a certain time interval, the standard deviation (which is a measure of absolute statistical error) of this measurement is approximately equal to the square root of N (√N).

**Simple example:**

* If we count 100 pulses, the absolute error is √100 = 10 pulses. This means the “true” average value most likely lies in the range 100 ± 10 (from 90 to 110).
* If we count 10,000 pulses, the absolute error is √10,000 = 100 pulses. Range: 10,000 ± 100.

Note: the more events we register, the larger the absolute error, but the smaller it is relative to the value itself (10 relative to 100 is 10%, while 100 relative to 10,000 is only 1%). It is this relative error, in percent, that the device calculates.

### Implementation in Code

#### 1. Obtaining the Total Number of Events (N)

N is taken as the total number of pulses accumulated in the 30-second sliding window. This is the variable `windowPulses`.

#### 2. Relative Error Formula

Relative error is calculated as the ratio of the standard deviation to the measured value:

* Relative error = (√N) / N

This fraction can be mathematically simplified to 1 / √N.

To express this value as a percentage, we simply multiply the result by 100:

* Relative error (%) = (1 / √N) * 100  or  100 / √N

#### 3. Calculation Code

This formula is implemented directly in the code:
```
float errorPercent = 0.0;
// Check that there were pulses to avoid division by zero
if (windowPulses > 0) {
// 100.0 / √N
errorPercent = 100.0 / sqrt(windowPulses);
}
```

### Practical Meaning for the User

This percentage on the screen provides an intuitive estimate of the reliability of the current readings:

* **High error (e.g., > 20–30%):** Occurs at low radiation background levels, when few pulses are registered in 30 seconds. This tells the user that the current dose rate value is “noisy” and not very stable.
* **Low error (e.g., < 5%):** Occurs at high radiation background levels, when hundreds or thousands of pulses are registered in 30 seconds. This indicates that the measurement is statistically significant and the dose rate readings are reliable and stable.

---

## Dose Accumulation Algorithm

One of the key functions of the device is calculating the total accumulated dose received over the measurement time. This process is called dose accumulation and runs in parallel with measuring the current dose rate.

### Purpose and Difference from Dose Rate

It is important to understand the difference between dose rate and accumulated dose. An analogy with a car is useful here:

* **Dose rate (µSv/h):** Like a speedometer in a car. It shows how “fast” you are receiving dose at the current moment. The value can change every second depending on the radiation environment.
* **Accumulated dose (µSv):** Like an odometer. It shows the total dose you have “traveled”, i.e., received since the start of measurement (or since the last reset). This value only increases over time.

Accumulated dose is a more important parameter for assessing the overall radiation impact on the body over a certain period.

### Implementation Principle in Code

The algorithm is based on the principle of discrete integration. Since dose rate is the speed of dose acquisition, the dose itself is the integral of dose rate over time. Digitally, we approximate this process by summing small “portions” of dose received over short time intervals (in our case, one second).

#### 1. Input Data

Each second, after averaging using the sliding window, we obtain the dose rate value in the variable `doseRate_uSv`. The units of this variable are microsieverts per hour (µSv/h).

#### 2. Conversion to Dose per Second

To find out how much dose was received in one second, we need to convert the hourly rate to a per-second rate. There are 3600 seconds in an hour, so the formula is simple:

> Dose per second = (Dose rate per hour) / 3600

#### 3. Summation (Accumulation)

This small dose portion obtained per second is added to the total accumulated dose counter. This process repeats every second.
```
// doseRate_uSv has units [µSv/hour]
// To get dose per second, divide by 3600
accumulatedDose_uSv += doseRate_uSv / 3600.0;
```

At the same time, the total measurement time counter is incremented by one:
```
accumulationTime_s++;
```

#### 4. Process Continuity

A key feature is that this calculation occurs every second in the background, regardless of which screen the user has selected. Thanks to this, even if the user is on the main screen, dose accumulation does not stop, and when switching to the accumulated dose screen, the user sees the current total value.

### Resetting Measurements

When the button is held down for a long press, the `resetMeasurements()` function is called. It resets both the accumulated dose counter and the time counter, allowing a new measurement session to start from zero.
```
void resetMeasurements() {
// ... (reset other variables)
accumulatedDose_uSv = 0.0;
accumulationTime_s = 0;
}
```


---

## Principle of Mode Switching and Control

A single button is used to interact with the dosimeter. It can perform two different functions depending on the duration of the press. Display mode control and measurement reset are implemented using a simple finite-state machine that tracks the button state.

### Task: Multifunction Button

The goal was to implement two different actions using one physical button:

* **Short press:** Switch between screens (display modes).
* **Long press:** Fully reset all accumulated data.

### Implementation in Code

To solve this task, button state tracking and timing measurements of how long it is pressed are used.

#### 1. State Variables

The following global variables are used to manage modes and button state:

```
// Enumeration to store possible display modes
enum DisplayMode {
  DISPLAY_MAIN,
  DISPLAY_ACCUM
};
// Variable storing the currently active mode
DisplayMode displayMode = DISPLAY_MAIN;

// Variables for button handling
bool lastButtonState = HIGH;        // Previous button state
unsigned long buttonPressTime = 0;  // Time when the button was pressed
unsigned long lastButtonEvent = 0;  // Time of last event for debouncing
```

#### 2. Debouncing

When a button is physically pressed, its contacts may close and open several times within a few milliseconds, creating “bounce”. To prevent a single press from being interpreted as multiple presses, a software debounce is implemented.
```
#define DEBOUNCE_MS 200 // Ignore new presses for 200 ms

// ... in the main loop loop()
if (buttonState == LOW && lastButtonState == HIGH &&
nowMillis - lastButtonEvent > DEBOUNCE_MS) {


 buttonPressTime = nowMillis; // Record the press start time
 lastButtonEvent = nowMillis; // Update the last event time
}

```

This code triggers only at the very first moment of pressing (when the state changes from HIGH to LOW) and only if more than 200 ms have passed since the last event.

#### 3. Determining Press Duration

The logic for determining the action triggers when the button is released.
```
// Triggers when the state changes from LOW to HIGH
if (buttonState == HIGH && lastButtonState == LOW) {
// Calculate how long the button was held
unsigned long pressDuration = nowMillis - buttonPressTime;

  // ... (logic for choosing the action follows)
}

```

#### 4. Short and Long Press Logic

Inside the block that triggers on button release, we analyze the calculated `pressDuration`:

```
#define LONG_PRESS_MS 1500 // Threshold for long press = 1.5 seconds

// If the button was held longer than the threshold
if (pressDuration >= LONG_PRESS_MS) {
  // Perform a reset of all measurements
  resetMeasurements();
}
// If the button was held shorter than the threshold
else {
  // Switch the display screen
  displayMode = (displayMode == DISPLAY_MAIN) ? DISPLAY_ACCUM : DISPLAY_MAIN;
}
```

#### 5. Mode Switching

To change the mode, a ternary operator is used, which works as a compact form of if-else:

`displayMode = (displayMode == DISPLAY_MAIN) ? DISPLAY_ACCUM : DISPLAY_MAIN;`

This line reads as: “If the current mode (`displayMode`) is `DISPLAY_MAIN`, assign it the value `DISPLAY_ACCUM`. Otherwise, assign `DISPLAY_MAIN`.” This effectively toggles the mode to the opposite one.

### Display Output Control

At the end of the main `loop()`, there is a block that decides which information to display on the screen based on the current value of the `displayMode` variable. It works like a rendering “router”.
```
if (displayMode == DISPLAY_MAIN) {
// Code to render the main screen
// (CPS, B8, dose rate, error)
} else { // If displayMode == DISPLAY_ACCUM
// Code to render the accumulated dose screen
// (Accumulation time, accumulated dose)
}
```


---

## Principle of Displaying Data on the Screen

A standard 16x2 character LCD with an I2C interface is used to display information in the project. The display logic is organized so that all necessary information fits on the small screen, while the interface remains clean and readable.

### Task: Informative and Clean Interface

The main task is to efficiently use the limited display space (2 lines of 16 characters) to show different sets of data depending on the user-selected mode, while avoiding visual artifacts.

### Library Used

Interaction with the display is handled through the popular `LiquidCrystal_I2C` library, which greatly simplifies sending commands and text to the display over the I2C bus.
```
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

```

### Implementation in Code

#### 1. Display Routing

In the main `loop()`, inside the once-per-second processing block, there is an if-else structure that acts as a “router”. It checks the current value of the `displayMode` variable and, depending on it, calls the corresponding block of code to render the appropriate screen.
```
if (displayMode == DISPLAY_MAIN) {
// Code to render the main screen...
} else { // If displayMode == DISPLAY_ACCUM
// Code to render the accumulated dose screen...
}
```

#### 2. Cursor Control and Line Clearing

Before outputting each block of information, the cursor position on the display is manually set using the `lcd.setCursor(column, row)` command. Numbering starts from (0, 0) for the top-left corner.

**Solving the “ghost character” problem:**

When displaying values of variable length (for example, number 100, then 99), “tails” from previous longer values may remain.

* Was: `CPS:100`
* Became: `CPS:990` (the zero remained from 100)

To avoid this, a simple but effective technique is used: after printing the useful information, the line is padded with spaces to the end. This guarantees that any previous content on that line is completely erased.
```
// Example of clearing a line “tail”
lcd.print(errorPercent, 1);
lcd.print("% "); // <-- These spaces erase old characters
```

#### 3. Screen Descriptions

##### Screen 1: Main Measurements (DISPLAY_MAIN)

Screen layout:
```
1 CPS:0.7 B8:42
2 0.24uSv 12.1%
```

* **Top line:** Cursor is set to (0, 0). Static text `CPS:` is printed, followed by the `cps` value with one decimal place, then the text `B8:` and the integer value `b8Value`.
* **Bottom line:** Cursor is set to (0, 1). The dose rate `doseRate_uSv` is printed with two decimal places, the unit `uSv`, and the relative error `errorPercent` with one decimal place and the `%` sign.

##### Screen 2: Accumulated Dose (DISPLAY_ACCUM)

Screen layout:
```
1 00:15:32
2 0.0123 uSv
```

* **Top line:** Displays the total accumulation time.
  * First, the total number of seconds `accumulationTime_s` is broken down into hours, minutes, and seconds using integer division and the modulo operator (%).
  * Then each component is printed with logic that adds a leading zero if the number is less than 10 (for example, `01:05:09` instead of `1:5:9`).

```
       if (hours < 10) lcd.print("0");
       lcd.print(hours);
```

* **Bottom line:** Displays the total accumulated dose `accumulatedDose_uSv` with high precision (4 decimal places) and the unit `uSv`.






