#pragma once
#include "activities/Activity.h"

// A simple list of offline games, launched from the Home menu.
class GamesMenuActivity : public Activity {
 public:
  GamesMenuActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("Games", renderer, mappedInput) {}
  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  int selectorIndex = 0;
  int scrollTop = 0;   // first visible row (list scrolls to keep selection in view)
  int gameCount() const;
  void launch(int index);
};
