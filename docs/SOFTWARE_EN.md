 Firmware Operating Principles
  ---
  Pulse Counting Principle: Hardware Interrupts
  Reliably counting every pulse from the Geiger counter is the most fundamental task of the dosimeter. To solve this, the project uses the hardware
  interrupt mechanism, which is the most accurate and efficient way to register fast, asynchronous events.
  Problem: Unpredictability and Brevity of Pulses

  Pulses from a Geiger counter have two characteristics that make counting them a complex task:

   - Randomness: They can arrive at any moment in time.
   - Brevity: The duration of a single pulse is very short (on the order of microseconds).

  If we were to try to "catch" pulses by simply reading the pin state in the main loop() (a method called "polling"), we would almost certainly miss
  most of them. At the moment a short pulse arrives, the microcontroller could be busy with other tasks: updating the screen, performing mathematical
  calculations, or handling a button press.
  Solution: Using Hardware Interrupts

  A hardware interrupt can be thought of as an "emergency phone call" for the microcontroller.

  When a specified event occurs on a special interrupt pin (e.g., a voltage change), the microcontroller immediately stops the execution of the current
  code in loop(), executes a special, short handler function (ISR - Interrupt Service Routine), and then returns to the exact place where it was
  interrupted.
  This guarantees that not a single pulse, not even the shortest one, will be missed, no matter what the main program loop is doing at that moment.

  Implementation in Code

   - Configuration in `setup()`

  In the setup() function, we "tell" the microcontroller which pin to listen to and which function to call.

   1  // Configure the pin as an input with a pull-up resistor to the power supply.
   2  // In its normal state, it will be HIGH (logical 1).
   3  pinMode(GEIGER_PIN, INPUT_PULLUP);
   4
   5  // "Attach" the interrupt to this pin
   6  attachInterrupt(digitalPinToInterrupt(GEIGER_PIN), geigerISR, RISING);
  Let's break down the attachInterrupt command:

   - digitalPinToInterrupt(GEIGER_PIN): Converts the pin number (D2) into the internal interrupt number (INT0).
   - geigerISR: The name of the function that will be called when the interrupt is triggered.
   - RISING: The trigger condition. The interrupt will occur the moment the voltage on the pin changes from LOW to HIGH. This happens when the pulse
     from the interface circuit ends, and the pin returns to its normal state thanks to the pull-up resistor.

   - The Interrupt Handler — `geigerISR()`

  This is the "emergency" function. It must be as short and fast as possible. You cannot use delay() or long operations within it.

   1  void geigerISR() {
   2  // ... (check for software dead time) ...
   3
   4  // 1. Increment the total pulse counter
   5  pulseCount++;
   6
   7  // 2. Set the flag for audio indication in the main loop
   8  beepFlag = true;
   9  }
  All it does is increment the pulseCount counter and set the beepFlag so that the main loop() knows to make a sound and flash the LED.
  Variables that are used in both the interrupt and the main loop (pulseCount, beepFlag) must be declared with the volatile keyword. This tells the
  compiler that the variable's value can change at any time and prevents it from applying certain optimizations that could break the logic.

   - Safe Counter Reading in `loop()`

  The pulseCount variable is of type unsigned long and occupies 4 bytes. Reading 4 bytes is not an atomic (instantaneous) operation. There is a tiny
  chance that an interrupt could occur right in the middle of reading this variable, which would lead to retrieving an incorrect value.
  To avoid this, we temporarily disable all interrupts before reading pulseCount and re-enable them immediately after.

   1  unsigned long currentCount;
   2
   3  noInterrupts(); // Disable interrupts
   4  currentCount = pulseCount; // Safely read the value
   5  interrupts(); // Immediately re-enable interrupts
  This ensures that the pulseCount value is read correctly and without corruption.
  ---
  Principle of Software "Dead Time"
  To ensure high accuracy in pulse counting, a software "dead time" mechanism is implemented in the firmware. Its main task is to filter out false
  triggers caused by the characteristics of the DP-5V dosimeter's analog path.
  Problem: "Dirty" Signal from the Audio Output

  The analog circuit of the DP-5V, which generates the audible clicks for the headphones, was not designed to produce a clean digital signal. As a
  result, a single real decay event registered by the Geiger counter can produce a more complex signal at the audio output, not just one clean pulse:

   - A main pulse followed by a series of decaying oscillations or "ringing."
   - "Bouncing" or several fast, consecutive peaks.

  These decaying oscillations, which can be called the "tails" of the main pulse, have sufficient amplitude to trigger the Arduino's hardware interrupt
  multiple times. This would lead to a single real event being counted as 2, 3, or even more pulses, which would artificially inflate the dose rate
  readings.
  Solution: Software "Dead Time" to Cut Off the "Tails"

  To combat this, a forced "pause" is introduced in the code after each valid pulse is registered.

  > Important: This is not the physical dead time of the Geiger counter itself (which is related to the process of gas recovery in the tube). It is
  exclusively a software filter, created to clean the input signal and cut off its "tails."

  How the filter works:
  After the program registers the first "front" of a pulse and counts it, it stops reacting to any subsequent changes on the interrupt pin for a short,
  strictly defined period.
  Implementation in Code

  This logic is fully implemented inside the geigerISR() interrupt handler function.

   - Defining the constant:
  At the beginning of the code, the duration of the "insensitivity period" is set in microseconds.

   1  // 20,000 microseconds = 20 milliseconds
   2  #define DEAD_TIME_US 20000
  20 ms is a time deliberately chosen to be longer than the duration of all "tails" and bouncing, but short enough not to miss the next real pulse at
  moderate radiation levels.

   - Logic inside `geigerISR()`:

    1  void geigerISR() {
    2  // Get the current time in microseconds
    3  unsigned long now = micros();
    4
    5  // Check if enough time has passed since the last COUNTED pulse
    6  if (now - lastPulseTime > DEAD_TIME_US) {
    7
    8  // --- This block executes only if > 20 ms have passed ---
    9
   10  // 1. Increment the counter
   11  pulseCount++;
   12
   13  // 2. UPDATE THE TIME OF THE LAST COUNTED PULSE
   14  lastPulseTime = now;
   15
   16  // 3. Set the flag for sound/light
   17  beepFlag = true;
   18  }
   19  // If LESS than 20 ms have passed since the last counted pulse,
   20  // the if condition is not met, and the function simply exits,
   21  // completely IGNORING the current interrupt trigger.
   22  }
  Thus, this simple but effective filter ensures that only the first front of a "dirty" signal is counted, and all its subsequent "tails" are ignored,
  providing counting accuracy.
  ---
  Sliding Window Algorithm
  To obtain stable and reliable dose rate readings, the project uses a data averaging algorithm based on the sliding window method. A detailed
  description of its operation is provided below.
  Problem: Random Nature of Radiation

  Radioactive decay is a stochastic (random) process. Even with a constant radiation background, the number of particles detected by the detector per
  unit of time constantly fluctuates. Measuring the number of counts per second (CPS) based only on the data from the last second would lead to the
  following problems:

   - Instability of readings: The values on the display would constantly "jump" (e.g., 0, 2, 1, 0, 3...), making an adequate assessment of the current
     level impossible.
   - Low reliability: A one-second measurement is not statistically significant, especially at low radiation levels.

  Solution: Averaging over a Sliding Window

  To solve this problem, data is averaged over a longer, but constantly updated, period. The sliding window algorithm implements exactly this approach.
  The main idea is: At any given moment, we operate with an average value calculated from the data of the last N seconds (in this project, N = 30).
  Every new second, the most recent data is added to the averaging set, and the oldest data (from N+1 seconds ago) is discarded. Thus, the measurement
  window "slides" along the timeline, always staying current.
  Implementation in Code

  To efficiently implement the algorithm, a circular (or cyclic) buffer is used.

   - Circular Buffer

  An array is created in the code to store the number of pulses registered for each of the last 30 seconds.

   1  #define WINDOW_SECONDS 30
   2  unsigned int cpsBuffer[WINDOW_SECONDS] = {0};
  This array can be imagined as 30 memory cells, each for its own second.

   - Per-Second Update

  Once per second, the main loop executes a block of code responsible for updating the data.

   - Writing New Data

  First, the number of pulses that occurred in the last second is calculated, and then this value is written to the current cell of the buffer. The
  pointer to the current cell is stored in the bufferIndex variable.

   1  // Calculate pulses in the last second
   2  unsigned int pulsesThisSecond = currentCount - lastPulseSnapshot;
   3  // Write them to the current cell
   4  cpsBuffer[bufferIndex] = pulsesThisSecond;

   - Sliding the Window

  After writing new data, the bufferIndex pointer is moved to the next cell. The modulo operator (%) is used to "wrap" the buffer.

   1 bufferIndex = (bufferIndex + 1) % WINDOW_SECONDS;
  When bufferIndex reaches a value of 29, on the next iteration, (29 + 1) % 30 results in 0. The pointer returns to the beginning of the array, and the
  new data begins to overwrite the oldest values. This is how the "sliding" is implemented—old data is automatically "forgotten."

   - Calculating the Average (CPS)

  Every second, after updating the buffer, the program recalculates the sum of all values in the 30 cells.

   1  unsigned long windowPulses = 0;
   2  for (unsigned int i = 0; i < WINDOW_SECONDS; i++) {
   3  windowPulses += cpsBuffer[i];
   4  }
  The resulting value windowPulses is the total number of pulses in the last 30 seconds. To find the average value per second (CPS), this sum is
  divided by the window duration.
   - Stability: The algorithm effectively smooths out random fluctuations, providing the user with a stable and easily readable value.
  The Geiger counter itself is just an "event counter." It registers how many times an ionizing particle has passed through it and outputs a
  corresponding number of electrical pulses. The value it measures, CPS (Counts Per Second), is simply the "number of pulses per second."
  However, this value does not indicate the degree of biological effect on a person. For this, the SI unit Sievert (and its derivatives, such as
  microsieverts per hour, µSv/h) is used, which estimates the equivalent dose rate of radiation.
  The task of the algorithm is to convert the physical quantity (CPS) into a biologically significant one (µSv/h).

  Problem: Non-Universal Conversion

  There is no single, universal formula for converting CPS to µSv/h. This conversion is the greatest complexity and source of error in any dosimeter.
  The conversion factor strongly depends on two main factors:

   - Type and sensitivity of the Geiger counter: A large counter will produce more pulses than a small one at the same radiation level. Different
     filling gases and cathode materials also affect sensitivity.
   - Energy of the detected radiation: The counter has different registration efficiencies for gamma photons of different energies. For example, it
     might be good at "seeing" radiation from Cesium-137 but worse at "seeing" radiation from Cobalt-60.

  Imagine you are trying to estimate the damage to a roof from precipitation. Simply counting "hits per second" (analogous to CPS) is not enough. You
  need to know what is falling: raindrops or hailstones the size of a fist (analogous to different radiation energies).
  Implementation in Code: Linear Calibration Factor

  For simplification, this project, like most consumer dosimeters, uses a linear calibration factor. This approach assumes that we are measuring
  radiation with a known or "averaged" energy spectrum (e.g., natural background radiation).
  This is implemented very simply in the code:
   1  // The factor that determines how many "µSv/h" are in one "CPS"
   2  const float CPS_TO_USVH = 0.34;
   3
   4  // ...
   5
   6  // Dose rate calculation
   7  float doseRate_uSv = cps * CPS_TO_USVH;
  This formula assumes that each registered "average" pulse per second (CPS) makes an equal, predetermined contribution to the final dose rate.
  Importance of Calibration

  The number 0.34 in the code is not a random value taken out of thin air. It is the calibration factor, obtained experimentally.

  The calibration process, described in the README.md, is as follows:

   - Two devices are taken: our DIY dosimeter and a reference, professional dosimeter (in this case, a DKG-03D).
   - Both devices are placed in the same conditions (e.g., measuring natural background).
   - Their readings are recorded: for our device in CPS, and for the reference one in µSv/h.
   - The factor is calculated using a simple formula:
  > Factor = (Reference_Reading, µSv/h) / (Our_Device_Reading, CPS)

  This is why, to achieve maximum accuracy, each assembled device should ideally be calibrated individually, as even two identical Geiger counters can
  have slightly different sensitivities.
  ---
  Statistical Error Calculation Algorithm
  In addition to the dose rate value itself, the device also calculates and displays the relative statistical error of the measurement as a percentage.
  This parameter shows how much the current readings can be "trusted."
  Problem: Uncertainty of Random Events

  Radioactive decay is a random (stochastic) process. This means it is impossible to predict exactly when the next decay will occur. Because of this
  randomness, any measurement based on counting events has an inherent uncertainty.
  For example, a reading of 0.24 uSv/h is not very informative by itself. It is much more useful to know if it is 0.24 ± 0.50 uSv/h (a very unreliable
  measurement) or 0.24 ± 0.01 uSv/h (a very reliable measurement). The statistical error provides exactly this assessment of reliability.
  Theoretical Basis: Poisson Statistics

  Random decay processes are well described by the Poisson distribution. One of the key conclusions of this theory states:

  > If N random events have been registered over a certain period, the standard deviation (which is the measure of absolute statistical error) of this
  measurement is approximately equal to the square root of N (√N).

  A simple example:
   - If we count 100 pulses, the absolute error will be √100 = 10 pulses. This means the "true" average value most likely lies in the range of 100 ± 10
     (from 90 to 110).
   - If we count 10,000 pulses, the absolute error will be √10000 = 100 pulses. The range is 10,000 ± 100.

  Note: the more events we register, the larger the absolute error, but the smaller it is in relation to the value itself (10 is 10% of 100, while 100
  is only 1% of 10,000). It is this relative error as a percentage that the device calculates.
  Implementation in Code

   - Getting the Total Number of Events (N)

  As N, the total number of pulses accumulated in the sliding window over 30 seconds is used. This is the windowPulses variable.

   - Relative Error Formula

  The relative error is calculated as the ratio of the standard deviation to the measured value:
   - Relative Error = (√N) / N

  This fraction can be mathematically simplified to 1 / √N.

  To express this value as a percentage, we simply multiply the result by 100.
   - Relative Error (%) = (1 / √N) * 100 or 100 / √N

   - Calculation Code

  This is the exact formula implemented in the code:

   1  float errorPercent = 0.0;
   2  // Check if there were any pulses to avoid division by zero
   3  if (windowPulses > 0) {
   4  //           100.0      /      √N
   5  errorPercent = 100.0 / sqrt(windowPulses);
   6  }
  Practical Meaning for the User

  This percentage on the screen provides an intuitive assessment of the reliability of the current readings:

   - High error (e.g., > 20-30%): Occurs at low radiation background when few pulses are registered in 30 seconds. This tells the user that the current
     dose rate value is "noisy" and not very stable.
   - Low error (e.g., < 5%): Occurs at high radiation background when hundreds or thousands of pulses are registered in 30 seconds. This indicates that
     the measurement is statistically significant, and the dose rate readings are reliable and stable.
  ---
  Dose Accumulation Algorithm
  One of the key functions of the device is to calculate the total dose received during the measurement time. This process is called dose accumulation
  and runs in parallel with the measurement of the current dose rate.
  Purpose and Differences from Dose Rate

  It is important to understand the difference between dose rate and accumulated dose. An analogy with a car is useful here:

   - Dose Rate (µSv/h): This is like the speedometer in a car. It shows how "fast" you are receiving a dose at the current moment. The value can change
     every second depending on the radiation environment.
   - Accumulated Dose (µSv): This is like the odometer (total mileage counter). It shows the total "distance" you have traveled, i.e., the total dose
     you have received since the beginning of the measurement (or since the last reset). This value only increases over time.

  The accumulated dose is a more important parameter for assessing the overall impact of radiation on the body over a specific period.

  Implementation in Code

  The algorithm is based on the principle of discrete integration. Since the dose rate is the speed at which the dose is received, the dose itself is
  the integral of the rate over time. In a digital form, we approximate this process by summing up small "portions" of the dose received over short
  time intervals (in our case, one second).

   - Source Data

  Every second, after averaging over the sliding window, we get the dose rate value in the doseRate_uSv variable. The units of this variable are
  microsieverts per hour (µSv/h).

   - Conversion to Dose per Second

  To find out how much dose was received in one second, the hourly rate must be converted to a per-second rate. There are 3600 seconds in an hour, so
  the formula is simple:
  > Dose per second = (Dose rate per hour) / 3600

   - Summation (Accumulation)

  This small portion of the dose received per second is added to the total accumulated dose counter. This process is repeated every second.

   1  // doseRate_uSv has units of [µSv/hour]
   2  // To get the dose for one second, we divide by 3600
   3  accumulatedDose_uSv += doseRate_uSv / 3600.0;
  Simultaneously, the total measurement time counter is incremented by one:
   1  accumulationTime_s++;

   - Continuity of the Process

  A key feature is that this calculation occurs every second in the background, regardless of which screen is selected by the user. Thanks to this,
  even if the user is on the main screen, dose accumulation does not stop, and when switching to the accumulated dose screen, they will see the current
  total value.
  Resetting Measurements

  A long press of the button calls the resetMeasurements() function. It resets both the accumulated dose counter and the time counter, allowing a new
  measurement session to be started from scratch.
   1  void resetMeasurements() {
   2  // ... (resetting other variables)
   3  accumulatedDose_uSv = 0.0;
   4  accumulationTime_s = 0;
   5  }
  ---
  Principle of Mode Switching and Control
  To interact with the dosimeter, a single button is used, which is capable of performing two different functions depending on the duration of the
  press. The control of display modes and measurement reset is implemented using a simple state machine that tracks the button's state.
  Task: A Multi-function Button

  The goal was to implement two different actions with a single physical button:
   - Short press: Switch between screens (display modes).
   - Long press: A full reset of all accumulated data.

  Implementation in Code

  To solve this task, the state of the button is tracked, and the time during which it was pressed is measured.

   - State Variables

  The following global variables are used to manage modes and the button state:

    1  // Enumeration to store possible screen modes
    2  enum DisplayMode {
    3  DISPLAY_MAIN,
    4  DISPLAY_ACCUM
    5  };
    6  // Variable that stores the current active mode
    7  DisplayMode displayMode = DISPLAY_MAIN;
    8
    9  // Variables for handling presses
   10  bool lastButtonState = HIGH;      // Previous button state
   11  unsigned long buttonPressTime = 0;  // Time when the button was pressed
   12  unsigned long lastButtonEvent = 0;  // Time of the last event for debouncing

   - Debouncing

  When a button is physically pressed, its contacts can bounce several times for a few milliseconds, creating "noise." To prevent a single press from
  being perceived as multiple presses, a software protection mechanism is implemented.

   1  #define DEBOUNCE_MS 200 // Ignore new presses for 200 ms
   2
   3  // ... in the main loop()
   4  if (buttonState == LOW && lastButtonState == HIGH &&
   5  nowMillis - lastButtonEvent > DEBOUNCE_MS) {
   6
   7  buttonPressTime = nowMillis; // Record the start time of the press
   8  lastButtonEvent = nowMillis; // Update the time of the last event
   9  }
  This code triggers only at the very first moment of a press (when the state changes from HIGH to LOW) and only if more than 200 ms have passed since
  the last event.

   - Determining Press Duration

  The logic for determining the action triggers when the button is released.

   1  // Triggers when the state changes from LOW to HIGH
   2  if (buttonState == HIGH && lastButtonState == LOW) {
   3   // Calculate how long the button was held down
   4   unsigned long pressDuration = nowMillis - buttonPressTime;
   5
   6   // ... (the logic for selecting the action follows)
   7  }

   - Short and Long Press Logic

  Inside the block that triggers on button release, we analyze the calculated pressDuration:

    1  #define LONG_PRESS_MS 1500 // Threshold for a long press = 1.5 seconds
    2
    3  // If the button was held down longer than the threshold
    4  if (pressDuration >= LONG_PRESS_MS) {
    5   // Perform a reset of all measurements
    6   resetMeasurements();
    7  }
    8  // If the button was held down for a shorter time
    9  else {
   10   // Perform a screen switch
   11   displayMode = (displayMode == DISPLAY_MAIN) ? DISPLAY_ACCUM : DISPLAY_MAIN;
   12  }

   - Mode Switching

  A ternary operator is used to change the mode, which works as a compact form of if-else:
  displayMode = (displayMode == DISPLAY_MAIN) ? DISPLAY_ACCUM : DISPLAY_MAIN;
  This line reads as: "If the current mode (displayMode) is DISPLAY_MAIN, then assign it the value DISPLAY_ACCUM. Otherwise, assign DISPLAY_MAIN." This
  effectively toggles the mode to the opposite one.
  Controlling the Display Output

  At the end of the main loop(), there is a block that decides exactly what information to display on the screen based on the current value of the
  displayMode variable. It works as a "router" for rendering.

   1  if (displayMode == DISPLAY_MAIN) {
   2   // Code to draw the main screen
   3   // (CPS, B8, dose rate, error)
   4  } else { // If displayMode == DISPLAY_ACCUM
   5   // Code to draw the accumulated dose screen
   6   // (Accumulation time, accumulated dose)
   7  }
  ---
  Principle of Displaying Data on Screen
  To display information, the project uses a standard 16x2 character LCD with an I2C interface. The data output logic is organized in such a way that
  all necessary information fits on the small screen, and the interface remains clean and readable.
  Task: An Informative and Clean Interface

  The main task is to effectively use the limited space of the display (2 rows of 16 characters) to show different sets of data depending on the
  user-selected mode, while avoiding visual artifacts.
  Library Used

  Interaction with the display is carried out through the popular LiquidCrystal_I2C library, which greatly simplifies sending commands and text to the
  display via the I2C bus.
   1  #include <LiquidCrystal_I2C.h>
   2  LiquidCrystal_I2C lcd(0x27, 16, 2);
  Implementation in Code

   - Display Routing

  In the main loop(), inside the per-second processing block, there is an if-else construct that acts as a "router." It checks the current value of the
  displayMode variable and, depending on it, calls the corresponding block of code to draw one screen or another.

   1  if (displayMode == DISPLAY_MAIN) {
   2   // Code to draw the main screen...
   3  } else { // If displayMode == DISPLAY_ACCUM
   4   // Code to draw the accumulated dose screen...
   5  }

   - Cursor Control and Line Clearing

  Before outputting each block of information, the cursor position on the display is set manually using the lcd.setCursor(column, row) command.
  Numbering starts from (0, 0) for the top-left corner.
  Solving the "ghost characters" problem:
  When displaying values of variable length on an LCD (e.g., the number 100, then 99), "tails" from previous, longer values can remain.
   - Was: CPS:100
   - Became: CPS:990 (the zero remained from the number 100)

  To avoid this, a simple but effective technique is used in the code: after outputting useful information, the line is padded with spaces to the end.
  This ensures that any previous entry on that line is completely overwritten.

   1  // Example of clearing the "tail" of a line
   2  lcd.print(errorPercent, 1);
   3  lcd.print("%   "); // <-- These spaces overwrite old characters

   - Screen Descriptions

  ##### Screen 1: Main Measurements (DISPLAY_MAIN)
  Screen view:
   1 CPS:0.7 B8:42
   2 0.24uSv 12.1%
   - Top row: The cursor is set to (0, 0). The static text CPS: is displayed, then the cps value with one decimal place, then the text  B8: and the
     integer value b8Value.
   - Bottom row: The cursor is set to (0, 1). The dose rate doseRate_uSv is displayed with two decimal places, the unit uSv, and the relative error
     errorPercent with one decimal place and a % sign.

  ##### Screen 2: Accumulated Dose (DISPLAY_ACCUM)
  Screen view:

   1 00:15:32
   2 0.0123 uSv
   - Top row: Displays the total accumulation time.
       - First, the total number of seconds accumulationTime_s is broken down into hours, minutes, and seconds using integer division and the modulo
         operator (%).
       - Then, each component is displayed on the screen using logic that adds a leading zero if the number is less than 10 (e.g., 01:05:09 instead of
         1:5:9).

   1      if (hours < 10) lcd.print("0");
   2      lcd.print(hours);
   - Bottom row: Displays the total accumulated dose accumulatedDose_uSv with high precision (4 decimal places) and the unit uSv.