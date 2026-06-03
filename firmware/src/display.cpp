#include "display.h"
#include "shared_state.h"
#include "heltec_unofficial.h"

// PRG button is handled by heltec_unofficial via heltec_loop()
// We use heltec_button_was_pressed() for short press detection
// and track hold time manually for the long-press clear

#define BTN_HOLD_MS 2000

static uint32_t btnPressedAt = 0;
static bool     btnWasHeld   = false;

void displayInit() {
  display.clear();
  display.display();
}

void displaySplash() {
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 6,  "DEAD");
  display.drawString(64, 24, "RECKONING");
  display.setFont(ArialMT_Plain_10);
  display.drawString(64, 46, "no carrier. no cloud.");
  display.display();
}

static void drawScreenMesh() {
  display.clear();
  if (!sharedState.lock()) return;

  uint32_t recv    = sharedState.msgs_received;
  uint32_t relayed = sharedState.msgs_relayed;
  char     last[32] = "";
  bool     hasMsg   = sharedState.has_last_msg;
  if (hasMsg) strncpy(last, sharedState.last_msg, 31);

  sharedState.unlock();

  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 0, "MESH");
  display.drawHorizontalLine(0, 12, 128);

  char buf[32];
  snprintf(buf, sizeof(buf), "RECEIVED %lu", recv);
  display.drawString(0, 15, buf);
  snprintf(buf, sizeof(buf), "RELAYED  %lu", relayed);
  display.drawString(0, 26, buf);

  if (hasMsg) {
    display.drawHorizontalLine(0, 39, 128);
    display.drawString(0, 42, last);
  }

  display.display();
}

static void drawScreenProbes() {
  display.clear();
  if (!sharedState.lock()) return;

  uint32_t total  = sharedState.devices_total;
  uint32_t unique = sharedState.devices_unique;
  char     ssid[33] = "";
  uint32_t ssidCnt  = sharedState.topSSIDCount();
  strncpy(ssid, sharedState.topSSID(), 32);

  sharedState.unlock();

  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 0, "ENVIRONMENT");
  display.drawHorizontalLine(0, 12, 128);

  char buf[32];
  snprintf(buf, sizeof(buf), "PROBES   %lu", total);
  display.drawString(0, 15, buf);
  snprintf(buf, sizeof(buf), "UNIQUE   %lu", unique);
  display.drawString(0, 26, buf);

  display.drawHorizontalLine(0, 39, 128);
  snprintf(buf, sizeof(buf), "TOP SSID (%lu)", ssidCnt);
  display.drawString(0, 41, buf);

  char ssidTrunc[22];
  strncpy(ssidTrunc, ssid, 21);
  ssidTrunc[21] = '\0';
  display.drawString(0, 52, ssidTrunc);

  display.display();
}

static void drawScreenAbout() {
  display.clear();

  uint32_t uptimeSec = (millis() - sharedState.start_time_ms) / 1000;
  uint32_t h = uptimeSec / 3600;
  uint32_t m = (uptimeSec % 3600) / 60;
  uint32_t s = uptimeSec % 60;

  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 0,  "DEAD RECKONING");
  display.drawHorizontalLine(0, 12, 128);
  display.drawString(0, 15, "NO SIM. NO CARRIER.");
  display.drawString(0, 26, "NO CLOUD. NO CONSENT.");
  display.drawString(0, 37, "WITNESS ONLY.");

  char upbuf[20];
  snprintf(upbuf, sizeof(upbuf), "UP %02lu:%02lu:%02lu", h, m, s);
  display.drawString(0, 50, upbuf);

  display.display();
}

static void handleButton() {
  bool pressed = heltec_button_is_pressed();

  if (pressed && btnPressedAt == 0) {
    btnPressedAt = millis();
    btnWasHeld   = false;
  }

  if (pressed && !btnWasHeld && (millis() - btnPressedAt) > BTN_HOLD_MS) {
    // Long hold: clear sniffer stats
    if (sharedState.lock()) {
      sharedState.devices_total  = 0;
      sharedState.devices_unique = 0;
      sharedState.ssid_count     = 0;
      memset(sharedState.ssids, 0, sizeof(sharedState.ssids));
      sharedState.unlock();
    }
    btnWasHeld = true;
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 26, "STATS CLEARED");
    display.display();
    delay(800);
  }

  if (!pressed && btnPressedAt != 0) {
    if (!btnWasHeld) {
      // Short press: toggle screen freeze
      if (sharedState.lock()) {
        sharedState.screen_frozen = !sharedState.screen_frozen;
        sharedState.unlock();
      }
    }
    btnPressedAt = 0;
    btnWasHeld   = false;
  }
}

void displayUpdate() {
  handleButton();

  bool     frozen  = false;
  uint8_t  screen  = SCREEN_MESH;
  uint32_t changed = 0;

  if (sharedState.lock()) {
    frozen  = sharedState.screen_frozen;
    screen  = sharedState.current_screen;
    changed = sharedState.screen_last_changed;
    sharedState.unlock();
  }

  if (!frozen && (millis() - changed) > SCREEN_DWELL) {
    if (sharedState.lock()) {
      sharedState.current_screen = (sharedState.current_screen + 1) % SCREEN_COUNT;
      sharedState.screen_last_changed = millis();
      screen = sharedState.current_screen;
      sharedState.unlock();
    }
  }

  switch (screen) {
    case SCREEN_MESH:   drawScreenMesh();   break;
    case SCREEN_PROBES: drawScreenProbes(); break;
    case SCREEN_ABOUT:  drawScreenAbout();  break;
  }
}
