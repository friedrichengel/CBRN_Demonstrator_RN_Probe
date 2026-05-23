#pragma once
#include <Arduino.h>
#include "utilities.h"

// Einziges Bedienelement ist der seitliche Hauptknopf BTN. Die Touchflaeche
// (Touch_Pin / P0.11) wird nicht mehr verwendet - der TTP223 auf unserem
// Board war nicht zuverlaessig genug fuer ein Bedienkonzept.
// PIN_BTN wird auch von power.cpp gebraucht (Wake-Source fuer System OFF).
constexpr uint8_t PIN_BTN = UserButton_Pin;   // P1.10 = Arduino-Pin 42

// In setup() einmalig aufrufen.
void inputBegin();

// In loop() in regelmaessigen Abstaenden (z.B. alle 10 ms) aufrufen.
// Polt Button + Touchflaeche und ruft die in menu.h deklarierten
// Handler (onBtnShort, onBtnDouble, onBtnLong, onBtnRepeat, onBtnLongRelease) auf.
void inputPoll();
