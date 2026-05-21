#pragma once
#include <bluefruit.h>

// Sendepower in dBm. Wird auch im Advertisement mitgeschickt,
// damit Empfaenger die Entfernung schaetzen koennen.
constexpr int8_t TX_POWER = 4;

// Bluefruit-Stack starten, GATT-Service registrieren, optional Advertising starten.
void bleBegin();

// Advertising starten mit dem PHY-Modus aus cfg.long_range:
//   false -> Legacy/1M PHY  (BLE 4.x kompatibel, ~50 m)
//   true  -> Extended/Coded PHY S=8  (BLE 5, ~150-200 m, Scanner muss Coded PHY koennen)
// Holt Werte aus cfg, stoppt ggf. ein laufendes Advertising vorher.
void startAdvertising();

// Advertising stoppen.
void stopAdvertising();
