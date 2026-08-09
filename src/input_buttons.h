// ============================================================================
//  input_buttons.h — X3 button input (2 ADC ladders + digital power button)
// ----------------------------------------------------------------------------
//  Debounced, edge-triggered logical buttons with auto-repeat on Up/Down and
//  long-press detection on Power. Poll update() often; read events with
//  pressed()/held helpers.
// ============================================================================
#pragma once

#include <Arduino.h>
#include "config.h"

enum class Btn : uint8_t { None, Up, Down, Select, Back, Power };

class InputButtons {
 public:
  void begin();

  // Sample the pins; call every loop. Returns the logical button EVENT this
  // tick (a fresh press or an auto-repeat), or Btn::None.
  Btn poll();

  // The raw logical button currently held (Btn::None if nothing pressed).
  Btn current() const { return held_; }

  // True once per power long-press (>= BTN_LONGPRESS_MS).
  bool longPressPower();

  // Was any button down at begin()-time / this instant? Useful for boot checks.
  bool anyHeld() { return readLogical() != Btn::None; }

  // Blocking helper: returns true if `which` stays held for `ms` (for the
  // hold-BACK-at-boot -> portal gesture).
  bool heldFor(Btn which, uint32_t ms);

 private:
  Btn      held_        = Btn::None;   // debounced current button
  Btn      lastRaw_     = Btn::None;
  uint32_t lastChange_  = 0;
  uint32_t lastEvent_   = 0;           // for auto-repeat timing
  uint32_t pressStart_  = 0;
  bool     longFired_   = false;

  Btn readLogical();                   // instantaneous decode (no debounce)
  static Btn matchLadderA(int adc);
  static Btn matchLadderB(int adc);
};
