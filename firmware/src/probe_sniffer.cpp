#include "probe_sniffer.h"
#include "shared_state.h"
#include <esp_wifi.h>
#include <mbedtls/sha256.h>

// ─── 802.11 frame structures ──────────────────────────────

typedef struct {
  uint8_t  frame_ctrl[2];
  uint16_t duration;
  uint8_t  addr1[6];   // Destination
  uint8_t  addr2[6];   // Source (device MAC)
  uint8_t  addr3[6];   // BSSID
  uint16_t seq_ctrl;
  // Management frame body follows
} __attribute__((packed)) wifi_mgmt_hdr_t;

// Probe request subtype = 0x04, type = management (0x00)
#define FRAME_TYPE_MGMT    0x00
#define FRAME_SUBTYPE_PROBE_REQ 0x04

// ─── Seen-MAC hash set (open addressing, power of 2) ─────
#define MAC_HASH_BUCKETS 512  // Must be power of 2
static uint64_t macHashSet[MAC_HASH_BUCKETS];
static uint32_t macHashCount = 0;

static bool macSeen(uint64_t h) {
  uint32_t slot = (uint32_t)(h & (MAC_HASH_BUCKETS - 1));
  for (int i = 0; i < 16; i++) {
    uint32_t s = (slot + i) & (MAC_HASH_BUCKETS - 1);
    if (macHashSet[s] == 0) return false;
    if (macHashSet[s] == h) return true;
  }
  return false;
}

static bool macInsert(uint64_t h) {
  if (macHashCount >= MAC_HASH_BUCKETS - 64) return false; // Table full
  uint32_t slot = (uint32_t)(h & (MAC_HASH_BUCKETS - 1));
  for (int i = 0; i < 16; i++) {
    uint32_t s = (slot + i) & (MAC_HASH_BUCKETS - 1);
    if (macHashSet[s] == 0) {
      macHashSet[s] = h;
      macHashCount++;
      return true;
    }
    if (macHashSet[s] == h) return false; // Already present
  }
  return false;
}

// ─── Hash a MAC to 8-byte fingerprint ────────────────────
// Uses mbedTLS SHA-256 (already in ESP-IDF), takes first 8 bytes.
static uint64_t hashMAC(const uint8_t mac[6]) {
  uint8_t digest[32];
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts_ret(&ctx, 0); // 0 = SHA-256 (not SHA-224)
  mbedtls_sha256_update_ret(&ctx, mac, 6);
  mbedtls_sha256_finish_ret(&ctx, digest);
  mbedtls_sha256_free(&ctx);

  uint64_t fingerprint = 0;
  for (int i = 0; i < 8; i++) {
    fingerprint = (fingerprint << 8) | digest[i];
  }
  return fingerprint;
}

// ─── Extract SSID from probe request body ────────────────
// Probe body: sequence of tagged parameters. Tag 0 = SSID.
static void extractSSID(const uint8_t* body, int bodyLen, char* out, int outLen) {
  out[0] = '\0';
  int pos = 0;
  // Skip fixed parameters (4 bytes: timestamp=0 for probe req? Actually probe req has no fixed params)
  // Probe request body starts directly with tagged parameters
  while (pos + 2 <= bodyLen) {
    uint8_t tag    = body[pos];
    uint8_t tagLen = body[pos + 1];
    if (pos + 2 + tagLen > bodyLen) break;
    if (tag == 0) { // SSID tag
      int copyLen = (tagLen < outLen - 1) ? tagLen : outLen - 1;
      if (copyLen > 0) {
        memcpy(out, body + pos + 2, copyLen);
        out[copyLen] = '\0';
      }
      return;
    }
    pos += 2 + tagLen;
  }
}

// ─── Promiscuous callback ─────────────────────────────────
static void IRAM_ATTR probeCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
  if (type != WIFI_PKT_MGMT) return;

  const wifi_promiscuous_pkt_t* pkt = (wifi_promiscuous_pkt_t*)buf;
  const uint8_t* payload = pkt->payload;
  uint16_t payloadLen    = pkt->rx_ctrl.sig_len;

  if (payloadLen < sizeof(wifi_mgmt_hdr_t) + 2) return;

  const wifi_mgmt_hdr_t* hdr = (wifi_mgmt_hdr_t*)payload;

  // Check frame type/subtype
  uint8_t frameType    = (hdr->frame_ctrl[0] & 0x0C) >> 2;
  uint8_t frameSubtype = (hdr->frame_ctrl[0] & 0xF0) >> 4;

  if (frameType != FRAME_TYPE_MGMT) return;
  if (frameSubtype != FRAME_SUBTYPE_PROBE_REQ) return;

  const uint8_t* srcMAC = hdr->addr2;

  // Hash MAC for privacy
  uint64_t macHash = hashMAC(srcMAC);
  bool isNew = false;
  if (macInsert(macHash)) {
    isNew = true;
  }

  // Extract SSID from body
  const uint8_t* body    = payload + sizeof(wifi_mgmt_hdr_t);
  int            bodyLen = (int)payloadLen - (int)sizeof(wifi_mgmt_hdr_t);
  char ssid[33] = "";
  if (bodyLen > 0) {
    extractSSID(body, bodyLen, ssid, sizeof(ssid));
  }

  // Update shared state (best effort — skip if mutex contended)
  if (xSemaphoreTake(sharedState.mutex, 0) == pdTRUE) {
    sharedState.devices_total++;
    if (isNew) sharedState.devices_unique++;
    if (ssid[0] != '\0') {
      sharedState.recordSSID(ssid);
    }
    xSemaphoreGive(sharedState.mutex);
  }
}

// ─── Sniffer task ─────────────────────────────────────────
void snifferTask(void* params) {
  // Initialise WiFi in promiscuous mode on channel 6
  // (scanning would be better but requires channel hopping; v1 uses ch6)
  esp_wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_NULL);
  esp_wifi_start();

  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(probeCallback);

  // Hop channels 1→6→11 every 200ms to catch more probes
  const uint8_t channels[] = {1, 6, 11};
  uint8_t chanIdx = 0;

  while (true) {
    esp_wifi_set_channel(channels[chanIdx], WIFI_SECOND_CHAN_NONE);
    chanIdx = (chanIdx + 1) % 3;
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}
