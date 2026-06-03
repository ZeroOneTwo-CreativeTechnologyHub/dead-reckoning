#include "display.h"
#include "shared_state.h"
#include "heltec.h"

#define BTN_PRG        0     // GPIO0 — PRG button on Heltec V3
#define BTN_HOLD_MS  2000    // Hold duration to clear stats

static uint32_t btnPressedAt = 0;
static bool     btnWasHeld   = false;

// ─── Init ─────────────────────────────────────────────────
void displayInit() {
  pinMode(BTN_PRG, INPUT_PULLUP);
  Heltec.display->clear();
  Heltec.display->display();
}

// ─── Splash ───────────────────────────────────────────────
void displaySplash() {
  Heltec.display->clear();
  Heltec.display->setFont(ArialMT_Plain_16);
  Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
  Heltec.display->drawString(64, 6, "DEAD");
  Heltec.display->drawString(64, 24, "RECKONING");
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(64, 46, "no carrier. no cloud.");
  Heltec.display->display();
}

// ─── Screen renderers ─────────────────────────────────────

static void drawScreenMesh() {
  Heltec.display->clear();
  if (!sharedState.lock()) return;

  uint32_t sent     = sharedState.msgs_sent;
  uint32_t recv     = sharedState.msgs_received;
  uint32_t relayed  = sharedState.msgs_relayed;
  char     last[32] = "";
  bool     hasMsg   = sharedState.has_last_msg;
  if (hasMsg) strncpy(last, sharedState.last_msg, 31);

  sharedState.unlock();

  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->drawString(0, 0,  "MESH");
  Heltec.display->drawHorizontalLine(0, 12, 128);

  char buf[32];
  snprintf(buf, sizeof(buf), "SENT     %lu", sent);
  Heltec.display->drawString(0, 15, buf);
  snprintf(buf, sizeof(buf), "RECEIVED %lu", recv);
  Heltec.display->drawString(0, 26, buf);
  snprintf(buf, sizeof(buf), "RELAYED  %lu", relayed);
  Heltec.display->drawString(0, 37, buf);

  if (hasMsg) {
    Heltec.display->drawHorizontalLine(0, 50, 128);
    Heltec.display->drawString(0, 52, last);
  }

  Heltec.display->display();
}

static void drawScreenProbes() {
  Heltec.display->clear();
  if (!sharedState.lock()) return;

  uint32_t total   = sharedState.devices_total;
  uint32_t unique  = sharedState.devices_unique;
  char     ssid[33] = "";
  uint32_t ssidCnt = sharedState.topSSIDCount();
  strncpy(ssid, sharedState.topSSID(), 32);

  sharedState.unlock();

  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->drawString(0, 0,  "ENVIRONMENT");
  Heltec.display->drawHorizontalLine(0, 12, 128);

  char buf[32];
  snprintf(buf, sizeof(buf), "PROBES   %lu", total);
  Heltec.display->drawString(0, 15, buf);
  snprintf(buf, sizeof(buf), "UNIQUE   %lu", unique);
  Heltec.display->drawString(0, 26, buf);

  Heltec.display->drawHorizontalLine(0, 39, 128);
  snprintf(buf, sizeof(buf), "TOP SSID (%lu)", ssidCnt);
  Heltec.display->drawString(0, 41, buf);

  // Truncate SSID to fit display width (approx 21 chars at font10)
  char ssidTrunc[22];
  strncpy(ssidTrunc, ssid, 21);
  ssidTrunc[21] = '\0';
  Heltec.display->drawString(0, 52, ssidTrunc);

  Heltec.display->display();
}

static void drawScreenAbout() {
  Heltec.display->clear();

  uint32_t uptimeSec = (millis() - sharedState.start_time_ms) / 1000;
  uint32_t h = uptimeSec / 3600;
  uint32_t m = (uptimeSec % 3600) / 60;
  uint32_t s = uptimeSec % 60;

  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->setTextAlignment(TEXT_ALIGN_LEFT);
  Heltec.display->drawString(0, 0,  "DEAD RECKONING");
  Heltec.display->drawHorizontalLine(0, 12, 128);
  Heltec.display->drawString(0, 15, "NO SIM. NO CARRIER.");
  Heltec.display->drawString(0, 26, "NO CLOUD. NO CONSENT.");
  Heltec.display->drawString(0, 37, "WITNESS ONLY.");

  char upbuf[20];
  snprintf(upbuf, sizeof(upbuf), "UP %02lu:%02lu:%02lu", h, m, s);
  Heltec.display->drawString(0, 50, upbuf);

  Heltec.display->display();
}

// ─── Button handling ──────────────────────────────────────
static void handleButton() {
  bool pressed = (digitalRead(BTN_PRG) == LOW);

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
    // Brief flash to confirm
    Heltec.display->clear();
    Heltec.display->setTextAlignment(TEXT_ALIGN_CENTER);
    Heltec.display->drawString(64, 26, "STATS CLEARED");
    Heltec.display->display();
    delay(800);
  }

  if (!pressed && btnPressedAt != 0) {
    // Short press released: toggle freeze
    if (!btnWasHeld) {
      if (sharedState.lock()) {
        sharedState.screen_frozen = !sharedState.screen_frozen;
        sharedState.unlock();
      }
    }
    btnPressedAt = 0;
    btnWasHeld   = false;
  }
}

// ─── Main update — called from loop() ─────────────────────
void displayUpdate() {
  handleButton();

  bool frozen = false;
  uint8_t screen = SCREEN_MESH;
  uint32_t changed = 0;

  if (sharedState.lock()) {
    frozen  = sharedState.screen_frozen;
    screen  = sharedState.current_screen;
    changed = sharedState.screen_last_changed;
    sharedState.unlock();
  }

  // Advance screen on timer if not frozen
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
