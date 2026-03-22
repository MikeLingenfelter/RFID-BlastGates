/**
 * Calibration.ino — Servo Angle Calibration Tool
 * 
 * Use this sketch to find the correct SERVO_OPEN_ANGLE and
 * SERVO_CLOSED_ANGLE values for your blast gate linkage before
 * installing the main BlastGateController sketch.
 * 
 * Instructions:
 *   1. Change SERVO_PIN below to match the gate you're calibrating.
 *   2. Upload this sketch.
 *   3. Open Serial Monitor at 9600 baud.
 *   4. Type an angle (0–180) and press Enter to move the servo.
 *   5. Record the open and closed angles, then enter them in config.h.
 */

#include <Servo.h>

// ── Change this to the pin of the servo you're calibrating ──
#define SERVO_PIN 22

Servo testServo;

void setup() {
  Serial.begin(9600);
  testServo.attach(SERVO_PIN);
  testServo.write(90);
  Serial.println(F("Servo Calibration Tool"));
  Serial.println(F("Enter an angle (0–180) and press Enter:"));
}

void loop() {
  if (Serial.available()) {
    int angle = Serial.parseInt();
    if (angle >= 0 && angle <= 180) {
      testServo.write(angle);
      Serial.print(F("Moved to: "));
      Serial.println(angle);
    } else {
      Serial.println(F("Invalid angle. Enter a value between 0 and 180."));
    }
    // Flush remaining input
    while (Serial.available()) Serial.read();
  }
}
