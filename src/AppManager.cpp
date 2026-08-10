#include "AppManager.h"

void AppManager::push(App* a) {
  a->nav = this;
  stack_.push_back(a);
  a->onEnter();
  dirty_ = true;
  fullNext_ = true;  // clean full refresh when entering a new screen
  lastTick_ = millis();
  lastInput_ = millis();
}

void AppManager::pop() {
  if (stack_.size() > 1) {
    delete stack_.back();
    stack_.pop_back();
    dirty_ = true;
    fullNext_ = true;
  }
}

void AppManager::loop() {
  btn_.poll();
  App* a = top();

  Btn ev = btn_.firstPressed();
  if (ev != Btn::None) {
    lastInput_ = millis();  // activity
    bool consumed = a->onButton(ev);
    if (consumed) {
      dirty_ = true;
    } else if ((ev == Btn::Back || ev == Btn::Power) && depth() > 1) {
      pop();  // unconsumed Back/Power backs out of the app
    }
  }

  // Timed tick (animated games).
  App* cur = top();
  uint32_t iv = cur->tickIntervalMs();
  if (iv > 0 && millis() - lastTick_ >= iv) {
    lastTick_ = millis();
    if (cur->onTick()) { dirty_ = true; lastInput_ = millis(); }  // animation = activity
  }

  if (dirty_) {
    App* t = top();  // may have changed via push/pop
    disp_.clear();
    t->render(disp_);
    EInkDisplay::RefreshMode m =
        fullNext_ ? EInkDisplay::FULL_REFRESH : t->refreshMode();
    disp_.present(m);
    dirty_ = false;
    fullNext_ = false;
  }
}
