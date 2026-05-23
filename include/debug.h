#pragma once
#include <Arduino.h>

// Zentraler Schalter fuer die ausfuehrlichen Diagnose-Ausgaben (>>>>>>>>>> ...).
// 0 = Stille (nur die wichtigen Status-Zeilen wie [CFG], [BAT], [BLE], [EPD]).
// 1 = Volle Trace (Touch-Edges, Long/Short/Release-Events, Periodic-State).
// Auf 1 setzen wenn etwas am Input nicht funktioniert.
#define DEBUG_VERBOSE 0

#if DEBUG_VERBOSE
  #define DBG_PRINT(...)   Serial.print(__VA_ARGS__)
  #define DBG_PRINTLN(...) Serial.println(__VA_ARGS__)
  #define DBG_PRINTF(...)  Serial.printf(__VA_ARGS__)
#else
  #define DBG_PRINT(...)   do {} while (0)
  #define DBG_PRINTLN(...) do {} while (0)
  #define DBG_PRINTF(...)  do {} while (0)
#endif
