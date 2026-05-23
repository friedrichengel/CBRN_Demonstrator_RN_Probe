#include "config.h"
#include <InternalFileSystem.h>

// 'File' kollidiert mit SdFat's typedef (kommt transitiv via Adafruit_TinyUSB),
// daher Namespace-Alias.
namespace LFS = Adafruit_LittleFS_Namespace;

const char* nuklid_name[NUKLID_COUNT + 1] = {
  "", "Cs-137", "Co-60", "Am-241", "Ir-192",
  "Ra-226", "Th-232", "U-238", "Pu-239"
};

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
      cfg.nuklid < 1 || cfg.nuklid > NUKLID_COUNT ||
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
