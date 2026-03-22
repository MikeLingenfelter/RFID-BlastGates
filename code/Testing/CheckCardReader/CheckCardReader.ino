void isr_d0() { Serial.println("D0 fired"); }
void isr_d1() { Serial.println("D1 fired"); }

void setup() {
  Serial.begin(9600);
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(2), isr_d0, FALLING);
  attachInterrupt(digitalPinToInterrupt(3), isr_d1, FALLING);
  Serial.println("Interrupt test ready.");
}

void loop() {}