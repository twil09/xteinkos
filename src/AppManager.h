// AppManager — a stack of Apps with an e-ink-friendly redraw-on-change loop.
#pragma once
#include <vector>
#include "App.h"
#include "Buttons.h"
#include "DuetDisplay.h"

class AppManager {
 public:
  AppManager(DuetDisplay& d, Buttons& b) : disp_(d), btn_(b) {}

  void push(App* a);
  void pop();
  App* top() { return stack_.back(); }
  size_t depth() const { return stack_.size(); }

  void requestRedraw() { dirty_ = true; }
  void requestFull() { fullNext_ = true; dirty_ = true; }

  void loop();  // call from Arduino loop()

  // Idle/power helpers for the main loop.
  uint32_t idleMs() const { return millis() - lastInput_; }
  bool keepAwake() { return stack_.back()->keepAwake(); }
  void noteActivity() { lastInput_ = millis(); }

 private:
  std::vector<App*> stack_;
  DuetDisplay& disp_;
  Buttons& btn_;
  bool dirty_ = true;
  bool fullNext_ = true;
  uint32_t lastTick_ = 0;
  uint32_t lastInput_ = 0;
};
