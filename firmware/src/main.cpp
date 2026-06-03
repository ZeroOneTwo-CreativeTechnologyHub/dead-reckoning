/**
 * Dead Reckoning
 * ─────────────────────────────────────────────────────────
 * A Critical Engineering project for the Heltec WiFi LoRa 32 V3
 *
 * Three concurrent tasks:
 *   Core 0 — LoRa receive/relay (witness and relay, no origination)
 *   Core 1 — WiFi probe sniffer (passive, hashed MACs only)
 *   Main   — OLED display cycling between mesh and sniffer stats
 *
 * Manifesto alignment:
 *   Principle 1  — technology studied, not merely consumed
 *   Principle 5  — each device engineers its user; this makes it visible
 *   Principle 7  — acts in the space between production and consumption
 *   Principle 10 — the exploit is the most desirable form of exposure
 *
 * License: GNU GPL v3
 * https://github.com/ZeroOneTwo-CreativeTechnologyHub/dead-reckoning
 */

#include <Arduino.h>
#include "heltec_unofficial.h"
#include "display.h"
#include "lora_mesh.h"
#include "probe_sniffer.h"
#include "shared_state.h"

TaskHandle_t loraTaskHandle    = NULL;
TaskHandle_t snifferTaskHandle = NULL;

void setup() {
  Serial.begin(115200);

  // Initialise Heltec V3 board (OLED + power management)
  heltec_setup();

  sharedState.init();
  displayInit();
  displaySplash();
  delay(2000);

  // LoRa mesh task on Core 0
  xTaskCreatePinnedToCore(
    loraTask, "lora_mesh", 8192, NULL, 2, &loraTaskHandle, 0
  );

  // WiFi probe sniffer on Core 1
  xTaskCreatePinnedToCore(
    snifferTask, "probe_sniffer", 8192, NULL, 1, &snifferTaskHandle, 1
  );
}

void loop() {
  heltec_loop();       // Required by ropg library (handles button, battery)
  displayUpdate();
  delay(100);
}
