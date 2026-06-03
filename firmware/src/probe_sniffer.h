/**
 * probe_sniffer.h / probe_sniffer.cpp
 *
 * Runs on Core 1. Puts the ESP32-S3 WiFi radio into promiscuous (monitor) mode
 * and captures 802.11 Probe Request management frames.
 *
 * Privacy design:
 *   - Only probe request frames are processed (type=0, subtype=4)
 *   - Source MAC is SHA-256 hashed (truncated to 6 bytes) before storage
 *   - Raw MACs are never stored, logged, or transmitted
 *   - Hashed MACs are stored only in RAM; cleared on reset or long button hold
 *   - SSID strings from probe frames are stored (these are public network names
 *     the device is actively broadcasting — not private data)
 *   - Randomised/locally-administered MACs are flagged but still counted
 *
 * Legal basis (UK):
 *   Passive capture of unencrypted broadcast management frames for the purpose
 *   of aggregate statistical analysis. No personal data stored per ICO guidance
 *   on MAC address hashing (see project README for full legal notes).
 */

#pragma once
#include <Arduino.h>

void snifferTask(void* params);
