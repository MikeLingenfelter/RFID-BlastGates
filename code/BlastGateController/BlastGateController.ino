/**
 * Dust Collector Blast Gate Controller
 *
 * Hardware:  Arduino Mega 2560
 * Readers:   Wiegand 26-bit access control RFID readers (one per gate)
 * Actuators: Servo motors (one per gate)
 * Library:   PinChangeInterrupt by NicoHood
 *
 * Wiegand notes (learned during testing):
 *   - Some readers stream data continuously while a card is in the field.
 *     ISRs stop accepting bits once 26 are captured, then interrupts are
 *     detached immediately to block the rest of the stream.
 *   - millis() must NOT be called inside an ISR on AVR — Timer0 interrupts
 *     are disabled while an ISR runs, causing millis() to stall. All timing
 *     is handled in loop() using a newBit flag set by the ISR.
 *   - Interrupts are re-enabled after SCAN_DEBOUNCE_MS to prevent a
 *     second trigger from the same card presentation.
 *
 * Behavior:
 *   - Scan RFID card at any machine to toggle that gate open/closed.
 *   - Only one gate may be open at a time.
 *   - All gates default to closed on power-up.
 */

#include <Servo.h>
#include <PinChangeInterrupt.h>
#include "config.h"

// ─────────────────────────────────────────────
//  WIEGAND STATE (one struct per reader)
// ─────────────────────────────────────────────

struct WiegandReader {
  volatile uint32_t data;        // bits shifted in so far
  volatile uint8_t  count;       // number of bits received
  volatile bool     frameReady;  // true once 26 bits captured
  volatile bool     newBit;      // set by ISR so loop() can timestamp it
  bool     interruptsAttached;   // tracks whether interrupts are live
  uint32_t lastBitMs;            // millis() of last bit — set in loop()
  uint32_t detachMs;             // millis() when interrupts were detached
};

static WiegandReader wg[8];

// ── ISR macros ────────────────────────────────────────────────────────
// ISRs are kept minimal — no millis(), no Serial, just bit capture.
// Bits are ignored once count reaches 26 (handles streaming readers).

#define WG_D0(N) void isr_g##N##_d0() {          \
  if (wg[N].count < 26) {                         \
    wg[N].data = (wg[N].data << 1);               \
    wg[N].count++;                                 \
    wg[N].newBit = true;                           \
    if (wg[N].count == 26) wg[N].frameReady = true;\
  }                                                \
}

#define WG_D1(N) void isr_g##N##_d1() {           \
  if (wg[N].count < 26) {                          \
    wg[N].data = (wg[N].data << 1) | 0x01;        \
    wg[N].count++;                                  \
    wg[N].newBit = true;                            \
    if (wg[N].count == 26) wg[N].frameReady = true;\
  }                                                 \
}

WG_D0(0) WG_D1(0)
WG_D0(1) WG_D1(1)
WG_D0(2) WG_D1(2)
WG_D0(3) WG_D1(3)
WG_D0(4) WG_D1(4)
WG_D0(5) WG_D1(5)
WG_D0(6) WG_D1(6)
WG_D0(7) WG_D1(7)

// Lookup tables for attaching/detaching interrupts by gate index
typedef void (*voidFn)();
static const voidFn ISR_D0[8] = {
  isr_g0_d0, isr_g1_d0, isr_g2_d0, isr_g3_d0,
  isr_g4_d0, isr_g5_d0, isr_g6_d0, isr_g7_d0
};
static const voidFn ISR_D1[8] = {
  isr_g0_d1, isr_g1_d1, isr_g2_d1, isr_g3_d1,
  isr_g4_d1, isr_g5_d1, isr_g6_d1, isr_g7_d1
};

// ─────────────────────────────────────────────
//  GATE STATE
// ─────────────────────────────────────────────

Servo    gateServo[8];
bool     gateOpen[8];

// ─────────────────────────────────────────────
//  INTERRUPT HELPERS
// ─────────────────────────────────────────────

void attachReaderInterrupts(int i) {
  if (digitalPinToInterrupt(D0_PINS[i]) != NOT_AN_INTERRUPT) {
    attachInterrupt(digitalPinToInterrupt(D0_PINS[i]), ISR_D0[i], FALLING);
  } else {
    attachPCINT(digitalPinToPCINT(D0_PINS[i]), ISR_D0[i], FALLING);
  }
  if (digitalPinToInterrupt(D1_PINS[i]) != NOT_AN_INTERRUPT) {
    attachInterrupt(digitalPinToInterrupt(D1_PINS[i]), ISR_D1[i], FALLING);
  } else {
    attachPCINT(digitalPinToPCINT(D1_PINS[i]), ISR_D1[i], FALLING);
  }
  wg[i].interruptsAttached = true;
}

void detachReaderInterrupts(int i) {
  detachPCINT(digitalPinToPCINT(D0_PINS[i]));
  detachPCINT(digitalPinToPCINT(D1_PINS[i]));
  wg[i].interruptsAttached = false;
  wg[i].detachMs = millis();
}

// ─────────────────────────────────────────────
//  SETUP
// ─────────────────────────────────────────────

void setup() {
  Serial.begin(9600);

  for (int i = 0; i < NUM_GATES; i++) {
    // Wiegand state
    wg[i].data              = 0;
    wg[i].count             = 0;
    wg[i].frameReady        = false;
    wg[i].newBit            = false;
    wg[i].lastBitMs         = 0;
    wg[i].detachMs          = 0;
    wg[i].interruptsAttached = false;

    pinMode(D0_PINS[i], INPUT_PULLUP);
    pinMode(D1_PINS[i], INPUT_PULLUP);
    attachReaderInterrupts(i);

    // Servo — default closed
    gateServo[i].attach(SERVO_PINS[i]);
    gateOpen[i] = false;
    closeGate(i);
  }

  Serial.println(F("================================="));
  Serial.println(F(" Dust Collector Gate Controller "));
  Serial.print  (F("  Wiegand 26-bit  |  "));
  Serial.print  (NUM_GATES);
  Serial.println(F(" gates — all closed"));
  Serial.println(F("================================="));
}

// ─────────────────────────────────────────────
//  MAIN LOOP
// ─────────────────────────────────────────────

void loop() {
  uint32_t now = millis();

  for (int i = 0; i < NUM_GATES; i++) {

    // ── Timestamp new bits safely in loop() (not in ISR) ──────────────
    if (wg[i].newBit) {
      wg[i].lastBitMs = now;
      wg[i].newBit    = false;
    }

    // ── Re-enable interrupts after debounce period ─────────────────────
    if (!wg[i].interruptsAttached) {
      if (now - wg[i].detachMs >= SCAN_DEBOUNCE_MS) {
        wg[i].data       = 0;
        wg[i].count      = 0;
        wg[i].frameReady = false;
        attachReaderInterrupts(i);
      }
      continue;  // skip read check while interrupts are detached
    }

    // ── Check for a complete 26-bit frame ─────────────────────────────
    if (wg[i].frameReady) {
      // Detach immediately — blocks rest of stream from streaming readers
      detachReaderInterrupts(i);

      uint32_t cardId = wg[i].data;

      Serial.print(F("Card 0x"));
      Serial.print(cardId, HEX);
      Serial.print(F(" at reader "));
      Serial.println(i + 1);

      toggleGate(i);
    }
  }
}

// ─────────────────────────────────────────────
//  GATE LOGIC
// ─────────────────────────────────────────────

void toggleGate(int gateIndex) {
  if (gateOpen[gateIndex]) {
    closeGate(gateIndex);
    logGateEvent(gateIndex, "CLOSED (manual)");
  } else {
    // Enforce single-gate policy — close any open gate first
    for (int i = 0; i < NUM_GATES; i++) {
      if (i != gateIndex && gateOpen[i]) {
        closeGate(i);
        logGateEvent(i, "CLOSED (auto)");
      }
    }
    openGate(gateIndex);
    logGateEvent(gateIndex, "OPENED");
  }
}

void openGate(int gateIndex) {
  gateServo[gateIndex].write(SERVO_OPEN_ANGLE);
  gateOpen[gateIndex] = true;
  delay(SERVO_MOVE_DELAY);
}

void closeGate(int gateIndex) {
  gateServo[gateIndex].write(SERVO_CLOSED_ANGLE);
  gateOpen[gateIndex] = false;
  delay(SERVO_MOVE_DELAY);
}

// ─────────────────────────────────────────────
//  SERIAL LOGGING
// ─────────────────────────────────────────────

void logGateEvent(int gateIndex, const char* event) {
  Serial.print(F("Gate "));
  Serial.print(gateIndex + 1);
  Serial.print(F(": "));
  Serial.println(event);
  printGateStatus();
}

void printGateStatus() {
  Serial.print(F("  Status → "));
  for (int i = 0; i < NUM_GATES; i++) {
    Serial.print(F("G"));
    Serial.print(i + 1);
    Serial.print(F(":"));
    Serial.print(gateOpen[i] ? F("OPEN") : F("closed"));
    if (i < NUM_GATES - 1) Serial.print(F("  "));
  }
  Serial.println();
}
