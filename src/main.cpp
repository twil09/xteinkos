// ============================================================================
//  main.cpp — xteinkOS entry point
//  Duet-styled home launcher over reader + games, on the MIT community-sdk.
// ============================================================================
#include <Arduino.h>
#include <LittleFS.h>
#include <SPI.h>
#include <esp_sleep.h>
#include <driver/gpio.h>

#include "AppManager.h"
#include "Buttons.h"
#include "DuetDisplay.h"
#include "HomeApp.h"
#include "config.h"
#include "theme.h"

static DuetDisplay display;
static Buttons     buttons;
static AppManager* manager = nullptr;

static void drawSleepScreen() {
  auto& g = display.gfx();
  display.clear();
  // Big blocky "V" (classic font scaled), then "Vix OS" and "Sleeping".
  g.setFont(NULL);
  g.setTextColor(UI_BLACK);
  g.setTextSize(12);
  int vw = 6 * 12, vh = 8 * 12;
  g.setCursor(SCREEN_W / 2 - vw / 2, SCREEN_H / 2 - vh - 10);
  g.print("V");
  g.setTextSize(1);
  duet::centerText(g, SCREEN_W / 2, SCREEN_H / 2 + 20, FONT_LARGE, "Vix OS", UI_BLACK);
  duet::centerText(g, SCREEN_W / 2, SCREEN_H / 2 + 60, FONT_BODY, "Sleeping", UI_BLACK);
  display.present(EInkDisplay::FULL_REFRESH);
}

static void deepSleep() {
  drawSleepScreen();
  // Wait for the power button to be released so we don't instantly re-wake.
  uint32_t t = millis();
  while (digitalRead(PIN_POWER) == LOW && millis() - t < 3000) delay(20);
  Serial.flush();
  gpio_pullup_en((gpio_num_t)PIN_POWER);
  gpio_pulldown_dis((gpio_num_t)PIN_POWER);
  esp_deep_sleep_enable_gpio_wakeup(1ULL << PIN_POWER, ESP_GPIO_WAKEUP_GPIO_LOW);
  esp_deep_sleep_start();  // e-ink keeps its image; wakes into setup() on press
}

void setup() {
  Serial.begin(115200);
  delay(50);
  Serial.printf("\n=== %s v%s ===\n", FW_NAME, FW_VERSION);

  buttons.begin();
  display.begin();
  // The e-ink driver inits SPI with MISO disabled; re-enable MISO (pin 7) so
  // the SD card (shared SPI bus) can be read. E-ink is unaffected (write-only).
  SPI.begin(EPD_SCLK, PIN_SD_MISO, EPD_MOSI, -1);
  if (!LittleFS.begin(true)) Serial.println("[fs] LittleFS mount failed");

  manager = new AppManager(display, buttons);
  manager->push(new HomeApp());  // draws the launcher on first loop()
}

void loop() {
  manager->loop();
  bool idle = manager->idleMs() > IDLE_SLEEP_MS && !manager->keepAwake();
  if (manager->consumeSleep() || idle) deepSleep();
  delay(15);  // pace button polling; e-ink only refreshes on change
}
