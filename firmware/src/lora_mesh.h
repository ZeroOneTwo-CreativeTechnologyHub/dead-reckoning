/**
 * lora_mesh.h / lora_mesh.cpp
 *
 * Runs on Core 0. Handles LoRa packet Tx/Rx using the Heltec LoRa library.
 *
 * This node is a passive witness — it receives and relays only.
 * It does not originate transmissions on behalf of any user.
 *
 * Protocol (minimal, plaintext for v1):
 *   Each packet: [1 byte type] [4 byte node ID] [up to 115 bytes payload]
 *   Type 0x01 — MSG : text message, relay if not seen before
 *
 * Relay logic: simple flooding with a seen-packet cache (last 32 packet hashes).
 * Duplicate suppression prevents infinite loops.
 *
 * Future: swap for Meshtastic-compatible packet format.
 */

#pragma once
#include <Arduino.h>

void loraTask(void* params);
