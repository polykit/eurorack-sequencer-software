/***

POLYKIT 8-Step Sequencer

https://polykit.rocks/sequencer

License: Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International

***/

#define DEBUG_MODE

// === Pin assignment ===
const uint8_t gateOutPin = 13;       // Digital gate output
const uint8_t clockOutPin = 10;      // Digital clock output
const uint8_t clockInPin = 11;       // Digital clock input
const uint8_t resetInPin = 12;       // Digital reset input
const uint8_t cvRatePin = A2;        // CV for tempo/rate
const uint8_t cvGateLengthPin = A0;  // CV for gate length
const uint8_t cvStepsPin = A1;       // CV for step count
const uint8_t stepOutPins[8] = {3, 5, 6, 8, 2, 4, 7, 9};

// === Sequencer Settings ===
const uint8_t maxSteps = 8;
uint8_t currentStep = 0;
uint8_t stepCount = 8;  // Dynamically set by CV

// === Timing ===
unsigned long stepInterval = 200;      // ms between steps (tempo)
unsigned long minStepInterval = 50;    // ms between steps (tempo)
unsigned long maxStepInterval = 3000;  // ms between steps (tempo)
unsigned long gateOnTime = 50;         // ms gate length
unsigned long minGateOnTime = 50;      // ms min gate length
unsigned long clockOnTime = 10;        // ms clock length
bool gateHigh = false;
unsigned long gateStartTime = 0;
unsigned long lastStepTime = 0;
unsigned long lastClockTime = 0;

// === Clock Input Tracking ===
bool lastClockState = LOW;
bool lastResetState = LOW;

// === Link mode ===
const uint8_t linkOutPin = A3;
const uint8_t linkInPin = A4;
unsigned long lastLinkTime = 0;
bool isLinked = false;
bool isFirst = true;
bool isLast = true;
bool isActive = true;
bool lastLinkInState = HIGH;  // active low

void setup() {
#ifdef DEBUG_MODE
  Serial.begin(115200);
  Serial.println("Starting POLYKIT Sequencer ...");
#endif

  pinMode(gateOutPin, OUTPUT);
  pinMode(clockOutPin, OUTPUT);
  pinMode(clockInPin, INPUT);
  pinMode(resetInPin, INPUT);
  digitalWrite(gateOutPin, LOW);
  digitalWrite(clockOutPin, LOW);

  // === Initialize step output pins ===
  for (uint8_t i = 0; i < maxSteps; i++) {
    pinMode(stepOutPins[i], OUTPUT);
    digitalWrite(stepOutPins[i], LOW);
  }

  // === Link mode: detect additional units ===
  pinMode(linkOutPin, OUTPUT);
  digitalWrite(linkOutPin, LOW);
  pinMode(linkInPin, INPUT_PULLUP);
  lastLinkTime = millis();
  while (millis() - lastLinkTime < 1000) {
    if (digitalRead(linkInPin) == LOW) {
      isLinked = true;
      isFirst = false;
    }
  }
  digitalWrite(linkOutPin, HIGH);

  delay(1000);
  pinMode(linkOutPin, INPUT_PULLUP);
  pinMode(linkInPin, OUTPUT);
  digitalWrite(linkInPin, LOW);
  delay(1000);
  lastLinkTime = millis();
  while (millis() - lastLinkTime < 1000) {
    if (digitalRead(linkOutPin) == LOW) {
      if (isLinked) {
        isLast = false;
      }
      isLinked = true;
    }
  }
  digitalWrite(linkInPin, HIGH);
  pinMode(linkOutPin, OUTPUT);
  digitalWrite(linkOutPin, HIGH);
  pinMode(linkInPin, INPUT_PULLUP);

#ifdef DEBUG_MODE
  if (isLinked) {
    Serial.println("Link mode detected.");
    if (isFirst) Serial.println("Unit is first device in chain.");
    if (isLast) Serial.println("Unit is last device in chain.");
    if (!isFirst && !isLast) Serial.println("Unit is middle device in chain.");
  } else {
    Serial.println("Link mode NOT detected.");
  }
#endif

  if (!isFirst) {
    isActive = false;
  }
}

void loop() {
  // === Read CVs ===
  int rateCV = analogRead(cvRatePin);        // 0 - 1023
  int gateCV = analogRead(cvGateLengthPin);  // 0 - 1023
  int stepsCV = analogRead(cvStepsPin);      // 0 - 1023

  // === Map CV to useful values ===
  stepInterval = map(rateCV, 1023, 0, minStepInterval, maxStepInterval);
  gateOnTime = map(gateCV, 0, 1023, minStepInterval, stepInterval);
  stepCount = map(stepsCV, 0, 1023, 1, maxSteps);

  // === Clock Input Handling ===
  bool clockState =
      digitalRead(clockInPin);  // is internally wired to clock out pin
  if (clockState == HIGH && lastClockState == LOW) {
    if (isActive) {
      advanceStep();
    } else {
      disableOutputs();
    }

    // pass active state
    if (currentStep == 0 && isLinked && isActive) {
      if (isLast) {
        // send reset to first device
        pinMode(resetInPin, OUTPUT);
        digitalWrite(resetInPin, HIGH);
        delay(50);
        digitalWrite(resetInPin, LOW);
        pinMode(resetInPin, INPUT);
      } else {
        // pass to next device
        digitalWrite(linkOutPin, LOW);
        delay(50);
        digitalWrite(linkOutPin, HIGH);
      }
      isActive = false;
    }
  }
  lastClockState = clockState;

  // === Handle Reset Input ===
  bool resetState = digitalRead(resetInPin);
  if (resetState == HIGH && lastResetState == LOW) {
    if (isLinked && isFirst) {
      isActive = true;
    }
    currentStep = 0;
  }
  lastResetState = resetState;

  // === Clock Output Handling ===
  if (millis() - lastClockTime >= stepInterval) {
    digitalWrite(clockOutPin, HIGH);
    lastClockTime = millis();
  }

  // === Handle Clock Duration ===
  if (millis() - lastClockTime >= clockOnTime) {
    digitalWrite(clockOutPin, LOW);
  }

  // === Handle Gate Duration ===
  if (gateHigh && (millis() - gateStartTime >= gateOnTime)) {
    digitalWrite(gateOutPin, LOW);
    gateHigh = false;
  }

  // === Receive active state from link in ===
  bool linkInState = digitalRead(linkInPin);
  if (!isActive && linkInState == LOW && lastLinkInState == HIGH) {
    isActive = true;
  }
  lastLinkInState = linkInState;
}

void advanceStep() {
  lastStepTime = millis();

  // Update step outputs
  updateStepOutputs();

  // Trigger gate output
  digitalWrite(gateOutPin, HIGH);
  gateStartTime = millis();
  gateHigh = true;

  currentStep = (currentStep + 1) % stepCount;
}

void updateStepOutputs() {
  for (uint8_t i = 0; i < maxSteps; i++) {
    digitalWrite(stepOutPins[i], (i == currentStep) ? HIGH : LOW);
  }
}

void disableOutputs() {
  for (uint8_t i = 0; i < maxSteps; i++) {
    digitalWrite(stepOutPins[i], LOW);
  }
}
