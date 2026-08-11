#pragma once
#include "activities/Activity.h"

// Lights Out: toggle a cell and its orthogonal neighbours; turn every light off.
class LightsOutActivity : public Activity {
 public:
  LightsOutActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
      : Activity("LightsOut", renderer, mappedInput) {}
  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  static constexpr int N = 5;
  bool grid[N][N] = {{false}};
  int curR = 2, curC = 2;
  int moves = 0;
  bool solved = false;

  void reset();
  void press(int r, int c);
  bool allOff() const;
};
