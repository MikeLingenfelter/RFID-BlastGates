#define D0_PIN 2
#define D1_PIN 3

volatile uint32_t cardData = 0;
volatile uint8_t  bitCount = 0;
volatile bool     frameReady = false;

void isr_d0() {
  if (bitCount < 26) {
    cardData = (cardData << 1);
    bitCount++;
    if (bitCount == 26) frameReady = true;
  }
}

void isr_d1() {
  if (bitCount < 26) {
    cardData = (cardData << 1) | 0x1;
    bitCount++;
    if (bitCount == 26) frameReady = true;
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(D0_PIN, INPUT_PULLUP);
  pinMode(D1_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(D0_PIN), isr_d0, FALLING);
  attachInterrupt(digitalPinToInterrupt(D1_PIN), isr_d1, FALLING);
  Serial.println("Ready — tap card.");
}

void loop() {
  if (frameReady) {
    // Disable interrupts immediately to block rest of stream
    detachInterrupt(digitalPinToInterrupt(D0_PIN));
    detachInterrupt(digitalPinToInterrupt(D1_PIN));

    Serial.print("Card ID (decimal): ");
    Serial.println(cardData);
    Serial.print("Card ID (hex):     0x");
    Serial.println(cardData, HEX);

    // Reset and re-enable after 1.5 seconds
    delay(1500);
    cardData   = 0;
    bitCount   = 0;
    frameReady = false;
    attachInterrupt(digitalPinToInterrupt(D0_PIN), isr_d0, FALLING);
    attachInterrupt(digitalPinToInterrupt(D1_PIN), isr_d1, FALLING);
    Serial.println("Ready — tap card.");
  }
}