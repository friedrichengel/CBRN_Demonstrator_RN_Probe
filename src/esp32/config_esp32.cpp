#include "config.h"
#include <Preferences.h>

// ================================================================
// Persistente Konfiguration auf ESP32.
// Gegenstueck zu src/config.cpp (nRF52 / Adafruit InternalFS):
// dort LittleFS-Datei, hier NVS via Preferences. Struktur, Defaults,
// Validierung und CFG_MAGIC sind absichtlich identisch, damit sich
// beide Varianten gleich verhalten.
// ================================================================

const NuklidEntry nuklid_table[NUKLID_COUNT] = {
  { CS_137, "Cs-137" }, { CO_60,  "Co-60"  }, { AM_241, "Am-241" },
  { IR_192, "Ir-192" }, { RA_226, "Ra-226" }, { TH_232, "Th-232" },
  { U_238,  "U-238"  }, { PU_239, "Pu-239" }, { SE_75,  "Se-75"  },
};

int nuklidIndex(uint8_t id) {
  for (uint8_t i = 0; i < NUKLID_COUNT; ++i)
    if (nuklid_table[i].id == id) return i;
  return -1;
}

const char* nuklidName(uint8_t id) {
  int i = nuklidIndex(id);
  return i >= 0 ? nuklid_table[i].name : "?";
}

const UnitEntry unit_table[UNIT_COUNT] = {
  { "nSv/h", -3 },
  { "uSv/h",  0 },
  { "mSv/h", +3 },
  { "Sv/h" , +6 },
};

Config cfg;

static constexpr uint8_t CFG_MAGIC = 0xAD;
static constexpr const char* NVS_NS  = "strahler";
static constexpr const char* NVS_KEY = "cfg";

static Preferences prefs;

void loadConfig() {
  prefs.begin(NVS_NS, /*readOnly=*/false);
  size_t len = prefs.getBytesLength(NVS_KEY);
  if (len == sizeof(Config)) {
    prefs.getBytes(NVS_KEY, &cfg, sizeof(cfg));
  }
  prefs.end();

  // Migration wie auf dem T-Echo: Se-75 hatte frueher die ID 99.
  if (cfg.nuklid == 99) cfg.nuklid = SE_75;

  if (len != sizeof(Config) || cfg.magic != CFG_MAGIC ||
      cfg.sender_nr < 1 || cfg.sender_nr > 254 ||
      nuklidIndex(cfg.nuklid) < 0 || cfg.unit_idx >= UNIT_COUNT) {
    cfg = {1, CS_137, 10, true, false, /*unit_idx=*/1, CFG_MAGIC};
    Serial.println("[CFG] Defaults");
  } else {
    Serial.println("[CFG] Geladen");
  }
}

void saveConfig() {
  cfg.magic = CFG_MAGIC;
  prefs.begin(NVS_NS, false);
  prefs.putBytes(NVS_KEY, &cfg, sizeof(cfg));
  prefs.end();
  Serial.println("[CFG] Gespeichert");
}
