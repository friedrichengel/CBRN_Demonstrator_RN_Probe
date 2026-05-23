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

// Einheiten-Tabelle fuer die angezeigte Dosisleistung.
// Index = unit_idx in Config (per BLE 0..UNIT_COUNT-1 schreibbar).
// Die zwei Felder (Name, Exponent-Offset relativ zu uSv/h) dienen nur der
// Anzeige - die gespeicherte Zahl max_dosis_100 wird NICHT umgerechnet.
constexpr uint8_t UNIT_COUNT = 4;
struct UnitEntry { const char* name; int8_t exp_vs_uSv; };
extern const UnitEntry unit_table[UNIT_COUNT];

// Persistente Konfiguration. Wird im internen LittleFS abgelegt
// und beim Boot via loadConfig() in den globalen `cfg` geladen.
struct Config {
  uint8_t  sender_nr;       // 1..254 - Strahler-Identifikation
  uint8_t  nuklid;          // 1..NUKLID_COUNT
  uint16_t max_dosis_100;   // Anzeigewert (Ganzzahl) in <Einheit aus unit_idx>.
                            // Name historisch - frueher 0.01 * uSv/h. Heute reine Ganzzahl,
                            // direkt so wie auf dem Display und in nRF Connect zu sehen.
  bool     aktiv;           // sendet BLE-Advertising?
  bool     long_range;      // false = Legacy/1M PHY (~50 m, BLE4-kompatibel)
                            // true  = Extended/Coded PHY (~150-200 m, Scanner muss BLE5 koennen)
  uint8_t  unit_idx;        // 0..UNIT_COUNT-1, Default 1 = uSv/h
  uint8_t  magic;           // Validitaets-Marker - siehe CFG_MAGIC in config.cpp
};

extern Config cfg;

void loadConfig();
void saveConfig();
