/**
 * display.h / display.cpp
 * Controls the Heltec V3's built-in 0.96" SSD1306 OLED (128×64).
 *
 * Three screens rotate every SCREEN_DWELL ms:
 *   0 — MESH   : messages sent/received/relayed, last message snippet
 *   1 — PROBES : devices seen, unique count, top SSID
 *   2 — ABOUT  : project statement, press [PRG] to freeze
 *
 * The PRG button (GPIO0) freezes/unfreezes rotation.
 * Holding the button for 2s clears sniffer stats.
 *
 * Font: uses Heltec's built-in font16 for headings, font8 for body.
 * All text is drawn with Heltec.display methods directly.
 */

#pragma once
#include <Arduino.h>

void displayInit();
void displaySplash();
void displayUpdate();
