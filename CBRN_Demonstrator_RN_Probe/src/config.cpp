#include "config.h"
#include <InternalFileSystem.h>

// 'File' kollidiert mit SdFat's typedef (kommt transitiv via Adafruit_TinyUSB),
// daher Namespace-Alias.
namespace LFS = Adafruit_LittleFS_Namespace;

const char* nuklid_name[NUKLID_COUNT + 1] = {
  "", "Cs-137", "Co-60", "Am-241", "Ir-192",
  "Ra-226", "Th-232", "U-238", "Pu-239"
};

Config cfg;

static LFS::File cfgFile(InternalFS);
// 0xAC seit Hinzufuegen des long_range-Feldes; alte 0xAB-Configs werden
// beim ersten Boot der neuen Firmware automatisch durch Defaults ersetzt.
static constexpr uint8_t CFG_MAGIC = 0xAC;
static constexpr const char* CFG_PATH = "/strahler.bin";

void loadConfig() {
  InternalFS.begin();
  cfgFile.open(CFG_PATH, LFS::FILE_O_READ);
  if (cfgFile) { cfgFile.read(&cfg, sizeof(cfg)); cfgFile.close(); }
  if (cfg.magic != CFG_MAGIC || cfg.sender_nr < 1 || cfg.sender_nr > 254 ||
      cfg.nuklid < 1 || cfg.nuklid > NUKLID_COUNT || cfg.max_dosis_100 == 0) {
    cfg = {1, CS_137, 1000, true, false, CFG_MAGIC};
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
