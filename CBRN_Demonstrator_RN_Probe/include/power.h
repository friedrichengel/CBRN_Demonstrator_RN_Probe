#pragma once

// Geraet abschalten:
//   1. THW-Off-Screen zeichnen (E-Paper behaelt das Bild ohne Strom)
//   2. BLE-Advertising stoppen
//   3. Peripherie-Rail (Power_Enable_Pin) ausschalten
//   4. Hauptknopf als Wake-SENSE-Pin konfigurieren
//      (Touch absichtlich NICHT - zu empfindlich -> ungewolltes Aufwachen)
//   5. nRF52 in System OFF (~1 µA) versetzen
//
// Aufwachen: Hauptknopf druecken -> nRF52 fuehrt Reset durch,
// setup() startet von vorne. Kehrt also nie aus dieser Funktion zurueck.
[[noreturn]] void powerOff();
