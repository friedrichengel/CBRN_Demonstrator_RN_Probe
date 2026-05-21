#pragma once
#include <Arduino.h>

// Nuklid-Liste fuer das Trainingsgeraet.
// 0 = ungueltig/leer, 1..8 sind die im UI/BLE verwendeten IDs.
enum Nuklid : uint8_t {
  CS_137 = 1, CO_60 = 2, AM_241 = 3, IR_192 = 4,
  RA_226 = 5, TH_232 = 6, U_238  = 7, PU_239 = 8
};
constexpr uint8_t NUKLID_COUNT = 8;
extern const char* nuklid_name[NUKLID_COUNT + 1];

// Persistente Konfiguration. Wird im internen LittleFS abgelegt
// und beim Boot via loadConfig() in den globalen `cfg` geladen.
struct Config {
  uint8_t  sender_nr;       // 1..254 - Strahler-Identifikation
  uint8_t  nuklid;          // 1..NUKLID_COUNT
  uint16_t max_dosis_100;   // Dosis in 0.01 µSv/h, z.B. 1000 = 10.00 µSv/h
  bool     aktiv;           // sendet BLE-Advertising?
  bool     long_range;      // false = Legacy/1M PHY (~50 m, BLE4-kompatibel)
                            // true  = Extended/Coded PHY (~150-200 m, Scanner muss BLE5 koennen)
  uint8_t  magic;           // Validitaets-Marker - siehe CFG_MAGIC in config.cpp
};

extern Config cfg;

void loadConfig();
void saveConfig();
