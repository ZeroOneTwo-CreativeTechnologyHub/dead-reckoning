/**
 * Dead Reckoning
 * ─────────────────────────────────────────────────────────
 * A Critical Engineering project for the Heltec WiFi LoRa 32 V3
 *
 * Three concurrent tasks:
 *   Core 0 — LoRa mesh comms (send/receive messages via LoRa)
 *   Core 1 — WiFi probe sniffer (passive, hashed MACs only)
 *   Main    — OLED display cycling between mesh and sniffer stats
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
#include <Wire.h>
#include <SPI.h>
#include "heltec.h"
#include "display.h"
#include "lora_mesh.h"
#include "probe_sniffer.h"
#include "shared_state.h"

// ─── Task handles ─────────────────────────────────────────
TaskHandle_t loraTaskHandle   = NULL;
TaskHandle_t snifferTaskHandle = NULL;

// ─── Setup ────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  // Initialise Heltec board (OLED, LoRa, LED)
  Heltec.begin(
    true,   // OLED enable
    true,   // LoRa enable
    true,   // Serial enable
    true,   // PABOOST enable
    868E6   // 868 MHz — UK/EU band
  );

  sharedState.init();
  displayInit();

  // Splash screen
  displaySplash();
  delay(2000);

  // Spawn LoRa mesh task on Core 0
  xTaskCreatePinnedToCore(
    loraTask,
    "lora_mesh",
    8192,
    NULL,
    2,
    &loraTaskHandle,
    0
  );

  // Spawn WiFi sniffer task on Core 1
  xTaskCreatePinnedToCore(
    snifferTask,
    "probe_sniffer",
    8192,
    NULL,
    1,
    &snifferTaskHandle,
    1
  );
}

// ─── Main loop — display cycling ──────────────────────────
void loop() {
  displayUpdate();
  delay(100);
}
