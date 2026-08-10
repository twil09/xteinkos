// App — base class for a screen/activity. AppManager owns a stack of these.
#pragma once
#include <EInkDisplay.h>
#include "config.h"
#include "DuetDisplay.h"

class AppManager;

class App {
 public:
  AppManager* nav = nullptr;  // set by AppManager on push
  virtual ~App() {}

  virtual void onEnter() {}
  // Handle one button edge. Return true if the screen changed (needs redraw).
  // Return false for Back/Power to let the manager pop this app.
  virtual bool onButton(Btn b) { (void)b; return false; }
  virtual void render(DuetDisplay& d) = 0;

  // Timed tick support (for animated games like Snake). Return >0 ms to be
  // ticked; onTick() returns true when the screen changed.
  virtual uint32_t tickIntervalMs() const { return 0; }
  virtual bool onTick() { return false; }

  // Return true to block idle deep-sleep (e.g. during WiFi setup/connect).
  virtual bool keepAwake() const { return false; }

  // Refresh mode for non-full redraws (FAST is snappy; HALF is cleaner).
  virtual EInkDisplay::RefreshMode refreshMode() const {
    return EInkDisplay::FAST_REFRESH;
  }
};
