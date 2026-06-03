#include "lora_mesh.h"
#include "shared_state.h"
#include "heltec.h"
#include <LoRa.h>

// ─── LoRa parameters (868 MHz, EU868) ─────────────────────
#define LORA_FREQ       868E6
#define LORA_BW         125E3
#define LORA_SF         9
#define LORA_CR         5
#define LORA_SYNC       0x34    // Private network sync word
#define LORA_PREAMBLE   8
#define LORA_TX_POWER   17      // dBm — legal EU868 limit

// ─── Packet types ─────────────────────────────────────────
#define PKT_MSG  0x01
#define PKT_ACK  0x02

// ─── Seen-packet cache — dedup prevents relay loops ───────
#define SEEN_CACHE_SIZE 32
static uint32_t seenCache[SEEN_CACHE_SIZE];
static uint8_t  seenHead = 0;

static bool wasSeen(uint32_t hash) {
  for (int i = 0; i < SEEN_CACHE_SIZE; i++) {
    if (seenCache[i] == hash) return true;
  }
  return false;
}

static void markSeen(uint32_t hash) {
  seenCache[seenHead] = hash;
  seenHead = (seenHead + 1) % SEEN_CACHE_SIZE;
}

// ─── FNV-1a hash for dedup ────────────────────────────────
static uint32_t fnv1a(const uint8_t* data, size_t len) {
  uint32_t hash = 2166136261u;
  for (size_t i = 0; i < len; i++) {
    hash ^= data[i];
    hash *= 16777619u;
  }
  return hash;
}

// ─── Relay a packet verbatim ──────────────────────────────
static void loraRelay(const uint8_t* pkt, size_t len) {
  LoRa.beginPacket();
  LoRa.write(pkt, len);
  LoRa.endPacket();

  if (sharedState.lock()) {
    sharedState.msgs_relayed++;
    sharedState.unlock();
  }
}

// ─── Process a received packet ────────────────────────────
static void loraReceive(int packetSize) {
  if (packetSize == 0) return;

  uint8_t buf[120];
  size_t len = 0;
  while (LoRa.available() && len < 120) {
    buf[len++] = LoRa.read();
  }

  if (len < 5) return;

  uint32_t hash = fnv1a(buf, len);
  if (wasSeen(hash)) return;
  markSeen(hash);

  uint8_t type = buf[0];

  if (type == PKT_MSG) {
    char text[116] = "";
    size_t textLen = len - 5;
    if (textLen > 0 && textLen <= 115) {
      memcpy(text, buf + 5, textLen);
      text[textLen] = '\0';
    }

    if (sharedState.lock()) {
      sharedState.msgs_received++;
      strncpy(sharedState.last_msg, text, 63);
      sharedState.last_msg[63] = '\0';
      sharedState.has_last_msg = true;
      sharedState.unlock();
    }

    // Relay to extend mesh range
    loraRelay(buf, len);
  }
}

// ─── LoRa task — listen and relay only ────────────────────
void loraTask(void* params) {
  LoRa.setFrequency(LORA_FREQ);
  LoRa.setSignalBandwidth(LORA_BW);
  LoRa.setSpreadingFactor(LORA_SF);
  LoRa.setCodingRate4(LORA_CR);
  LoRa.setSyncWord(LORA_SYNC);
  LoRa.setPreambleLength(LORA_PREAMBLE);
  LoRa.setTxPower(LORA_TX_POWER);
  LoRa.receive();

  while (true) {
    int pktSize = LoRa.parsePacket();
    if (pktSize > 0) {
      loraReceive(pktSize);
      LoRa.receive();
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
