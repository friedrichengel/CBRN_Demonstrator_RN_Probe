#pragma once
#include <bluefruit.h>

// Sendepower in dBm. Wird auch im Advertisement mitgeschickt,
// damit Empfaenger die Entfernung schaetzen koennen.
constexpr int8_t TX_POWER = 8;

// Bluefruit-Stack starten, GATT-Service registrieren, optional Advertising starten.
void bleBegin();

// Advertising starten mit dem PHY-Modus aus cfg.long_range:
//   false -> Legacy/1M PHY  (BLE 4.x kompatibel, ~50 m)
//   true  -> Extended/Coded PHY S=8  (BLE 5, ~150-200 m, Scanner muss Coded PHY koennen)
// Holt Werte aus cfg, stoppt ggf. ein laufendes Advertising vorher.
void startAdvertising();

// Advertising stoppen.
void stopAdvertising();

// Schreibt die aktuellen cfg-Werte zurueck in die GATT-Charakteristika, damit
// GATT-Reads den am Geraet (via Taster) eingestellten Wert sehen und nicht
// nur den zuletzt per BLE geschriebenen. Aufzurufen nach jeder Menue-aenderung.
void bleSyncChars();
