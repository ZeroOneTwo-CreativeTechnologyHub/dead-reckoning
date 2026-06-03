/**
 * shared_state.h
 * Mutex-guarded state shared between the LoRa, sniffer, and display tasks.
 */

#pragma once
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// Max unique SSIDs to track per session
#define MAX_SSIDS 64
// Max unique hashed MACs per session
#define MAX_DEVICES 512
// OLED display screens
#define SCREEN_MESH    0
#define SCREEN_PROBES  1
#define SCREEN_ABOUT   2
#define SCREEN_COUNT   3
// Screen hold duration ms
#define SCREEN_DWELL   5000

struct SSIDEntry {
  char  name[33];
  uint16_t count;
};

struct SharedState {
  SemaphoreHandle_t mutex;

  // LoRa mesh stats
  uint32_t msgs_sent;
  uint32_t msgs_received;
  uint32_t msgs_relayed;
  char     last_msg[64];
  bool     has_last_msg;

  // Probe sniffer stats
  uint32_t devices_total;    // total probe frames seen
  uint32_t devices_unique;   // unique hashed MACs
  SSIDEntry ssids[MAX_SSIDS];
  uint8_t  ssid_count;

  // Uptime
  uint32_t start_time_ms;

  // Current display screen
  uint8_t  current_screen;
  uint32_t screen_last_changed;
  bool     screen_frozen;

  void init() {
    mutex = xSemaphoreCreateMutex();
    msgs_sent      = 0;
    msgs_received  = 0;
    msgs_relayed   = 0;
    has_last_msg   = false;
    devices_total  = 0;
    devices_unique = 0;
    ssid_count     = 0;
    start_time_ms  = millis();
    current_screen = SCREEN_MESH;
    screen_last_changed = millis();
    screen_frozen  = false;
    memset(last_msg, 0, sizeof(last_msg));
    memset(ssids,    0, sizeof(ssids));
  }

  bool lock(uint32_t timeout_ms = 50) {
    return xSemaphoreTake(mutex, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
  }

  void unlock() {
    xSemaphoreGive(mutex);
  }

  // Record a probe SSID (call with mutex held)
  void recordSSID(const char* name) {
    for (int i = 0; i < ssid_count; i++) {
      if (strncmp(ssids[i].name, name, 32) == 0) {
        ssids[i].count++;
        return;
      }
    }
    if (ssid_count < MAX_SSIDS) {
      strncpy(ssids[ssid_count].name, name, 32);
      ssids[ssid_count].count = 1;
      ssid_count++;
    }
  }

  // Get most-seen SSID name (call with mutex held)
  const char* topSSID() {
    if (ssid_count == 0) return "none";
    int best = 0;
    for (int i = 1; i < ssid_count; i++) {
      if (ssids[i].count > ssids[best].count) best = i;
    }
    return ssids[best].name;
  }

  uint32_t topSSIDCount() {
    if (ssid_count == 0) return 0;
    int best = 0;
    for (int i = 1; i < ssid_count; i++) {
      if (ssids[i].count > ssids[best].count) best = i;
    }
    return ssids[best].count;
  }
};

extern SharedState sharedState;
