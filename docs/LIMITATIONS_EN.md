 # Limitations and Assumptions of the Project (v0.3.0)

  This document describes the known limitations of the hardware and software parts of the DIY dosimeter-radiometer based on Arduino and DP-5V, as well
  as the assumptions made during the development of the measurement and data processing algorithms.

  ---

  ## 1. Statistical Nature of Measurements

  The measurement of ionizing radiation is based on the registration of individual events (pulses) that obey Poisson statistics.

  Consequences:
   * High relative error at a low number of pulses.
   * The necessity of time-based averaging to obtain stable readings.
   * The impossibility of obtaining an "instantaneous" accurate measurement.

  At low CPS values, the readings are indicative in nature, which is reflected in a high percentage of statistical error.

  ---

  ## 2. Handling "Dead Time" and Count Rate Limitation

  The firmware implements two mechanisms related to "dead time," each with its own goals and limitations.

  ### 2.1. Software Dead Time (Anti-Bounce)

  This mechanism serves to filter electrical noise ("bouncing" or "tails") from the analog path of the DP-5V.

   * Characteristics:
       * Type: Software filter.
       * Value: 20 ms (DEAD_TIME_US).
       * Goal: To ensure that one real event is counted as one pulse.
   * Critical Limitation: The set value of 20 ms imposes a hard ceiling on the maximum registerable count rate. The firmware physically cannot register
     more than 1 / 0.020 s = 50 CPS. Any radiation level generating more than 50 pulses/sec will be measured incorrectly.

  ### 2.2. Physical Dead Time Compensation

  This algorithm mathematically corrects the measured count rate to compensate for pulses missed by the Geiger tube itself at high dose rates.

   * Limitations:
       * The accuracy of the compensation depends entirely on the correctness of the specified TUBE_DEAD_TIME_S constant, which must match the
         datasheet value of the tube being used.
       * The algorithm only corrects the data that has already passed through the software dead time filter (i.e., no more than 50 CPS).

  ---

  ## 3. Measurement Range Limitations

  ### 3.1. Lower Limit

  At low background radiation, the number of pulses registered during the averaging window is small, which leads to a high statistical error (>30%).

  ### 3.2. Upper Limit

  The upper measurement limit is constrained by several factors:
   1. Software Filter: As stated above, the current implementation cannot measure more than 50 CPS. This is the main limiting factor.
   2. Compensation Accuracy: Even if the throughput of the software filter is increased, the accuracy at high levels will depend on the correctness of
      the TUBE_DEAD_TIME_S constant.
   3. Physical Tube Limit: At extreme radiation levels, any Geiger counter enters a saturation mode ("locks up"), where readings become completely
      unreliable.

  ---

 ## 4. Calibration Limitations

  The conversion factor from CPS to dose rate (CPS_TO_USVH):
   * Is an approximation and is highly dependent on the energy of the measured radiation.
   * Should be determined individually for each counter instance.
   * The calibration performed is not metrologically certified.

  ---

  ## 5. Accumulated Dose Calculation Limitations

  The accuracy of the accumulated dose directly depends on the accuracy of the dose rate measurement at each moment in time. All errors and limitations
  inherent in the dose rate measurement accumulate over time in the accumulated dose value.

  ---

  ## 6. Hardware and External Factors

  Measurement results can be affected by power supply instability, electrical and radio-frequency interference, and the temperature drift of
  components.

  ---

  ## 7. User Interface Limitations

   * The 16x2 LCD display limits the amount of information that can be displayed simultaneously.
   * There is no non-volatile memory to store the accumulated dose after the power is turned off.
   * There is no interface for transferring data to a PC (logging).

  ---

  ## 8. Area of Acceptable Use

  Important: This project is not a professional or certified measuring instrument.

   * Acceptable Use: Educational and demonstration purposes, experimental and research tasks, indicative monitoring and searching for sources of
     ionizing radiation.
   * Unacceptable Use: Radiation safety of personnel, medical and sanitary measurements, making critical decisions about protecting life and health.

  ---

  ## 9. Final Remarks

  The listed limitations are deliberate and correspond to the project's goals: simplicity of implementation, open architecture, and reproducibility.

  For use in critical applications, the following are required:
   - Hardware modernization;
   - Professional calibration;
   - Metrological certification.
