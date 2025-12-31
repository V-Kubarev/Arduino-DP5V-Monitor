# Firmware Operating Principles

---

## Pulse Counting Principle: Hardware Interrupts

Reliable counting of every pulse from the Geiger counter is the most fundamental task of a dosimeter. To solve it, the project uses the mechanism of hardware interrupts, which is the most accurate and efficient way to register fast, asynchronous events.

Problem: Unpredictability and short duration of pulses

Pulses from a Geiger counter have two features that make them difficult to count:

 * Randomness: They can occur at any moment in time.
 * Short duration: The duration of a single pulse is very short (on the order of microseconds).

If we tried to “catch” pulses by simply reading the pin state in the main `loop()` cycle (a method called polling), we would almost certainly miss most of them. At the moment a short pulse arrives, the microcontroller may be busy with other tasks: updating the display, performing calculations, or processing a button.

Solution: Using hardware interrupts

A hardware interrupt can be imagined as an “emergency phone call” for the microcontroller.

When a specified event occurs on a special interrupt pin (for example, a voltage change), the microcontroller immediately stops executing the current code in `loop()`, executes a special short handler function (ISR – Interrupt Service Routine), and then returns exactly to the point where it was interrupted.

This guarantees that not a single pulse, even the shortest one, will be missed, regardless of what the main program loop is doing at that moment.

Principle of implementation in code

1. Configuration in `setup()`

In the `setup()` function, we “tell” the microcontroller which pin to listen to and which function to call.
