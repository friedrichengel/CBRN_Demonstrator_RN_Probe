#pragma once
#include <Adafruit_EPD.h>

// Globales Display-Objekt - in display_ui.cpp definiert.
// Wird auch direkt von main.cpp fuer display.begin() benoetigt.
extern Adafruit_SSD1681 display;

// Zeichnet je nach `view`/`menuState` den passenden Vollbild-Inhalt
// und triggert den E-Paper-Refresh.
void updateDisplay();

// Zeichnet den THW-Logo-/Off-Screen. Wird beim Ausschalten verwendet -
// E-Paper behaelt das Bild ohne Strom.
void drawOffScreen();
