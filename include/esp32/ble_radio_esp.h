#pragma once
#include <Arduino.h>

// ================================================================
// BLE-Funkmodul, ESP32-Variante (NimBLE).
// Gegenstueck zu include/ble_radio.h (nRF52 / Bluefruit).
//
// Protokoll bleibt bitgleich zur T-Echo-Firmware:
//   - Manufacturer-Data v0x02, 10 Byte, siehe README
//   - GATT-Service 0x1900 mit 0x2B00..0x2B05
// Neu hinzugekommen:
//   - 0x2B06 uint8  OTA/WLAN-Fenster starten (Schreiben von 1)
//   - 0x2B07 string WLAN-SSID
//   - 0x2B08 string WLAN-Passwort (write only)
//   - Battery Service 0x180F / 0x2A19, damit der Ladezustand ohne
//     Display in der Handy-App sichtbar ist
// ================================================================

// Sendeleistung in dBm. Wird auch im Advertisement mitgeschickt
// (Byte 8 der Manufacturer-Data), damit Empfaenger die Entfernung
// schaetzen koennen. ESP32-S3: sinnvolle Stufen bis 9 dBm.
constexpr int8_t TX_POWER = 9;

void bleBegin();
void startAdvertising();
void stopAdvertising();
void bleSyncChars();

// Ladezustand in Prozent in den Battery Service schreiben (und ggf.
// an verbundene Clients notifizieren).
void bleSetBatteryLevel(uint8_t percent);

// true, solange ein Client verbunden ist.
bool bleConnected();
