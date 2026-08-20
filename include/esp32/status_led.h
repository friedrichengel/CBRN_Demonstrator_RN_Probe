#pragma once
#include <Arduino.h>

// ================================================================
// Betriebs-LED. Die LED sitzt aussen am Gehaeuse und sagt in erster
// Linie "Geraet ist an". Die Blinkmuster geben zusaetzlich den
// Betriebszustand, ohne dass ein Display noetig ist.
//
// Alle Muster sind non-blocking - ledTask() gehoert in loop().
// ================================================================

enum LedMode : uint8_t {
  LED_OFF,          // aus (nur fuer Testzwecke)
  LED_ON_SOLID,     // Dauerlicht (~6 mA - fuer 8-h-Betrieb unkritisch)
  LED_ADV_ACTIVE,   // 1 kurzer Blitz alle 3 s  -> an, Advertising laeuft
  LED_ADV_OFF,      // Doppelblitz alle 3 s     -> an, Advertising aus
  LED_BAT_LOW,      // 3 Blitze alle 3 s        -> Akku schwach
  LED_OTA           // schnelles Blinken (5 Hz) -> WLAN-/Update-Fenster offen
};

void ledBegin();
void ledSetMode(LedMode mode);
LedMode ledGetMode();
void ledTask();
