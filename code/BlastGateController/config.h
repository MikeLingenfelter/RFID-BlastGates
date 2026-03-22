/**
 * config.h — Blast Gate Controller Configuration
 *
 * Edit this file to match your wiring and mechanical setup.
 * See README.md for full wiring diagram and pin descriptions.
 *
 * Reader protocol: Wiegand 26-bit
 *   Each reader needs only two signal wires (DATA0 + DATA1) back to the Arduino.
 *   Suitable for cable runs up to 50+ feet — no SPI bus limitations.
 *
 * Library required:
 *   PinChangeInterrupt by NicoHood
 *   Install via: Tools → Manage Libraries → "PinChangeInterrupt"
 */

#ifndef CONFIG_H
#define CONFIG_H

// ─────────────────────────────────────────────
//  GATE COUNT
//  Set to the number of gates you have wired (max 8)
// ─────────────────────────────────────────────
#define NUM_GATES 4

// ─────────────────────────────────────────────
//  SERVO ANGLES
//  Run code/Calibration/Calibration.ino to find
//  the correct angles for your gate linkage.
// ─────────────────────────────────────────────
#define SERVO_OPEN_ANGLE    70    // Degrees — gate fully open
#define SERVO_CLOSED_ANGLE   4    // Degrees — gate fully closed
#define SERVO_MOVE_DELAY   500    // ms to wait after commanding a move

// ─────────────────────────────────────────────
//  SCAN DEBOUNCE
//  Minimum milliseconds between reads on the same
//  reader. Prevents accidental double-triggers.
// ─────────────────────────────────────────────
#define SCAN_DEBOUNCE_MS  2000

// ─────────────────────────────────────────────
//  WIEGAND 26 PIN ASSIGNMENTS
//
//  D0 and D1 are independent per reader.
//  All digital pins on the Arduino Mega are supported
//  via the PinChangeInterrupt library.
//
//  Default wiring:
//    Gate 1: D0=Pin 2,  D1=Pin 3    (also hardware INT4/INT5)
//    Gate 2: D0=Pin 18, D1=Pin 19   (also hardware INT3/INT2)
//    Gate 3: D0=Pin 20, D1=Pin 21   (also hardware INT1/INT0)
//    Gate 4: D0=Pin 14, D1=Pin 15
//
//  Array index 0 = Gate 1, index 1 = Gate 2, etc.
//  Extend to index 7 for 8-gate expansion.
// ─────────────────────────────────────────────
const uint8_t D0_PINS[8] = { 2, 18, 20, 14,  4,  6,  8, 10};
const uint8_t D1_PINS[8] = { 3, 19, 21, 15,  5,  7,  9, 11};

// ─────────────────────────────────────────────
//  SERVO SIGNAL PINS
//  One PWM-capable digital pin per gate.
// ─────────────────────────────────────────────
const uint8_t SERVO_PINS[8] = {22, 24, 26, 28, 30, 32, 34, 36};

#endif // CONFIG_H
