#pragma once
#include <Arduino.h>
#include "utilities.h"

// Bedienelemente. Wir nutzen die symbolischen Defines aus utilities.h,
// damit Pin-Mapping-Fehler ausgeschlossen sind (Schaltplan T-Echo:
// Hauptknopf an P1.10, Touch-IC an P0.11). Werden auch von power.cpp
// gebraucht, um den Button als Wake-Source fuer System OFF zu konfigurieren.
constexpr uint8_t PIN_BTN   = UserButton_Pin;   // P1.10 = Arduino-Pin 42
constexpr uint8_t PIN_TOUCH = Touch_Pin;        // P0.11 = Arduino-Pin 11

// In setup() einmalig aufrufen.
void inputBegin();

// In loop() in regelmaessigen Abstaenden (z.B. alle 10 ms) aufrufen.
// Polt Button + Touchflaeche und ruft die in menu.h deklarierten
// Handler (onBtnShort, onBtnLong, onTouchShort, onTouchRepeat) auf.
void inputPoll();
