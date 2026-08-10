// Buttons — thin facade over the SDK InputManager mapping to logical Btn.
#pragma once
#include <InputManager.h>
#include "config.h"

class Buttons {
 public:
  void begin() { im_.begin(); }
  void poll() { im_.update(); }

  bool pressed(Btn b) const { return im_.wasPressed((uint8_t)b); }   // edge
  bool held(Btn b) const { return im_.isPressed((uint8_t)b); }

  // First fresh press this tick (in Btn index order), or Btn::None.
  Btn firstPressed() const {
    for (uint8_t i = 0; i < BTN_COUNT; ++i)
      if (im_.wasPressed(i)) return (Btn)i;
    return Btn::None;
  }

  unsigned long powerHeldMs() { return im_.getPowerButtonHeldTime(); }

 private:
  InputManager im_;
};
