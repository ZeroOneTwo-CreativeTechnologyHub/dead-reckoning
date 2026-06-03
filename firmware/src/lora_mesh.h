/**
 * lora_mesh.h
 * LoRa receive and relay task — Core 0.
 *
 * This node is a passive witness — it receives and relays only.
 * It does not originate transmissions on behalf of any user.
 *
 * Uses RadioLib via heltec_unofficial library.
 */
#pragma once
#include <Arduino.h>

void loraTask(void* params);
