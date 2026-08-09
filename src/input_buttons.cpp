#include "input_buttons.h"

static inline bool near_(int v, int center) {
  return abs(v - center) <= ADC_TOLERANCE;
}

void InputButtons::begin() {
  analogReadResolution(12);
#if defined(ESP32)
  analogSetPinAttenuation(BTN_ADC_PIN_A, ADC_11db);
  analogSetPinAttenuation(BTN_ADC_PIN_B, ADC_11db);
#endif
  pinMode(BTN_POWER_PIN, INPUT_PULLUP);
  held_ = Btn::None;
  lastRaw_ = Btn::None;
  lastChange_ = millis();
}

Btn InputButtons::matchLadderA(int adc) {
  // Idle rests near 4095 (pull-up); a press pulls to a divider value.
  if (near_(adc, ADC_A_RIGHT))   return Btn::Up;    // right paddle = Up
  if (near_(adc, ADC_A_LEFT))    return Btn::Down;  // left paddle  = Down
  if (near_(adc, ADC_A_CONFIRM)) return Btn::Select;
  if (near_(adc, ADC_A_BACK))    return Btn::Back;
  return Btn::None;
}

Btn InputButtons::matchLadderB(int adc) {
  if (near_(adc, ADC_B_UP))   return Btn::Up;    // right side button = Up
  if (near_(adc, ADC_B_DOWN)) return Btn::Down;  // left side button  = Down
  return Btn::None;
}

Btn InputButtons::readLogical() {
  // Power button first (digital, active LOW).
  if (digitalRead(BTN_POWER_PIN) == LOW) return Btn::Power;

  int a = analogRead(BTN_ADC_PIN_A);
  Btn ba = matchLadderA(a);
  if (ba != Btn::None) return ba;

  int b = analogRead(BTN_ADC_PIN_B);
  Btn bb = matchLadderB(b);
  if (bb != Btn::None) return bb;

  return Btn::None;
}

Btn InputButtons::poll() {
  uint32_t now = millis();
  Btn raw = readLogical();

  // Debounce: require the raw reading to be stable for BTN_DEBOUNCE_MS.
  if (raw != lastRaw_) {
    lastRaw_ = raw;
    lastChange_ = now;
    return Btn::None;
  }
  if (now - lastChange_ < BTN_DEBOUNCE_MS) return Btn::None;

  Btn event = Btn::None;

  if (raw != held_) {
    // State transition (edge).
    held_ = raw;
    if (raw != Btn::None) {
      event = raw;            // fresh press
      lastEvent_ = now;
      pressStart_ = now;
      longFired_ = false;
    }
  } else if (raw != Btn::None) {
    // Still held — auto-repeat for navigation buttons only.
    if ((raw == Btn::Up || raw == Btn::Down) &&
        (now - lastEvent_ >= BTN_REPEAT_MS)) {
      event = raw;
      lastEvent_ = now;
    }
  }

  return event;
}

bool InputButtons::longPressPower() {
  if (held_ == Btn::Power && !longFired_ &&
      (millis() - pressStart_ >= BTN_LONGPRESS_MS)) {
    longFired_ = true;
    return true;
  }
  return false;
}

bool InputButtons::heldFor(Btn which, uint32_t ms) {
  uint32_t start = millis();
  while (millis() - start < ms) {
    if (readLogical() != which) return false;
    delay(10);
  }
  return true;
}
