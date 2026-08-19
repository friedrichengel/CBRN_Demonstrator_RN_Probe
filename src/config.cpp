#include "config.h"
#include <InternalFileSystem.h>

// 'File' kollidiert mit SdFat's typedef (kommt transitiv via Adafruit_TinyUSB),
// daher Namespace-Alias.
namespace LFS = Adafruit_LittleFS_Namespace;

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

// Index 0..UNIT_COUNT-1 - per BLE-Write auf chrUnit oder im Menue zyklisch wechselbar.
// Exponent ist informativ (relativ zu uSv/h) und wird im Geraet nicht zum
// Skalieren benutzt - max_dosis_100 wird einfach mit dem Namen angezeigt.
const UnitEntry unit_table[UNIT_COUNT] = {
  { "nSv/h", -3 },
  { "uSv/h",  0 },
  { "mSv/h", +3 },
  { "Sv/h" , +6 },
};

Config cfg;

static LFS::File cfgFile(InternalFS);
// 0xAD seit Hinzufuegen von unit_idx (vorher 0xAC). Aeltere Configs werden
// beim ersten Boot der neuen Firmware automatisch durch Defaults ersetzt.
static constexpr uint8_t CFG_MAGIC = 0xAD;
static constexpr const char* CFG_PATH = "/strahler.bin";

void loadConfig() {
  InternalFS.begin();
  cfgFile.open(CFG_PATH, LFS::FILE_O_READ);
  if (cfgFile) { cfgFile.read(&cfg, sizeof(cfg)); cfgFile.close(); }
  // max_dosis_100 == 0 ist KEIN Invaliditaets-Indikator mehr - 0 ist ein
  // gueltiger Anzeigewert (z.B. "0 uSv/h" fuer Hintergrundstrahlung-Demo).
  if (cfg.magic != CFG_MAGIC || cfg.sender_nr < 1 || cfg.sender_nr > 254 ||
      nuklidIndex(cfg.nuklid) < 0 ||
      cfg.unit_idx >= UNIT_COUNT) {
    cfg = {1, CS_137, 10, true, false, /*unit_idx=*/1, CFG_MAGIC};
    Serial.println("[CFG] Defaults");
  } else {
    Serial.println("[CFG] Geladen");
  }
}

void saveConfig() {
  cfg.magic = CFG_MAGIC;
  InternalFS.remove(CFG_PATH);
  cfgFile.open(CFG_PATH, LFS::FILE_O_WRITE);
  if (cfgFile) { cfgFile.write((uint8_t*)&cfg, sizeof(cfg)); cfgFile.close(); }
  Serial.println("[CFG] Gespeichert");
}
