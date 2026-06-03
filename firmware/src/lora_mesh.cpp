#include "lora_mesh.h"
#include "shared_state.h"
#include "heltec_unofficial.h"

// ─── LoRa parameters (868 MHz, EU868) ─────────────────────
#define LORA_FREQ       868.0   // MHz
#define LORA_BW         125.0   // kHz
#define LORA_SF         9
#define LORA_CR         5
#define LORA_SYNC       0x34
#define LORA_PREAMBLE   8
#define LORA_TX_POWER   17      // dBm

#define PKT_MSG  0x01

// ─── Seen-packet cache ────────────────────────────────────
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

static uint32_t fnv1a(const uint8_t* data, size_t len) {
  uint32_t hash = 2166136261u;
  for (size_t i = 0; i < len; i++) {
    hash ^= data[i];
    hash *= 16777619u;
  }
  return hash;
}

static void loraRelay(const uint8_t* pkt, size_t len) {
  radio.transmit(pkt, len);
  if (sharedState.lock()) {
    sharedState.msgs_relayed++;
    sharedState.unlock();
  }
  // Return to listen mode
  radio.startReceive();
}

static void loraReceive() {
  uint8_t buf[120];
  size_t  len = radio.getPacketLength();
  if (len == 0 || len > 120) { radio.startReceive(); return; }

  int state = radio.readData(buf, len);
  if (state != RADIOLIB_ERR_NONE || len < 5) { radio.startReceive(); return; }

  uint32_t hash = fnv1a(buf, len);
  if (wasSeen(hash)) { radio.startReceive(); return; }
  markSeen(hash);

  if (buf[0] == PKT_MSG) {
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
    loraRelay(buf, len);
    return;
  }

  radio.startReceive();
}

void loraTask(void* params) {
  // Configure radio via RadioLib (used by heltec_unofficial)
  radio.setFrequency(LORA_FREQ);
  radio.setBandwidth(LORA_BW);
  radio.setSpreadingFactor(LORA_SF);
  radio.setCodingRate(LORA_CR);
  radio.setSyncWord(LORA_SYNC);
  radio.setPreambleLength(LORA_PREAMBLE);
  radio.setOutputPower(LORA_TX_POWER);
  radio.startReceive();

  while (true) {
    // Check RadioLib receive flag (set by ISR via heltec_unofficial)
    if (receivedFlag) {
      receivedFlag = false;
      loraReceive();
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
