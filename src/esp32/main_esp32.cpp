// ================================================================
// CBRN-RN Trainingsgeraet - Strahler-Firmware, ESP32-Variante
// Hardware: Seeed XIAO ESP32S3 im Gehaeuse, externe 2,4-GHz-Antenne,
//           Hauptschalter in der Akkuleitung, Betriebs-LED aussen.
//
// Kein Display, kein Tastenmenue: eingestellt wird per Handy-App ueber
// den GATT-Service 0x1900 (identisch zur T-Echo-Version), Firmware
// kommt ueber WLAN (siehe esp32/ota.h).
// ================================================================

#include <Arduino.h>
#include "config.h"
#include "battery.h"
#include "esp32/board_xiao_s3.h"
#include "esp32/ble_radio_esp.h"
#include "esp32/status_led.h"
#include "esp32/ota.h"

// Versionszaehler der ESP32-Linie (die T-Echo-Firmware laeuft auf 1.x).
constexpr uint8_t VERSION_MAJOR = 2;
constexpr uint8_t VERSION_MINOR = 0;

const char* firmwareVersion() {
  static char buf[8];
  snprintf(buf, sizeof(buf), "%u.%02u", (unsigned)VERSION_MAJOR, (unsigned)VERSION_MINOR);
  return buf;
}

// Akku alle 60 s pruefen: Battery Service aktualisieren und ggf. das
// LED-Muster auf "Akku schwach" umstellen.
static constexpr uint32_t BAT_CHECK_MS = 60UL * 1000UL;
static uint32_t lastBatCheck = 0;

static void batteryCheck() {
  int pct = batteryPercent();
  bleSetBatteryLevel((uint8_t)pct);

  // Prioritaet: Akku-Warnung schlaegt die normale Betriebsanzeige,
  // das OTA-Muster bleibt unangetastet.
  if (ledGetMode() != LED_OTA) {
    if (pct <= BAT_LOW_PERCENT)      ledSetMode(LED_BAT_LOW);
    else if (cfg.aktiv)              ledSetMode(LED_ADV_ACTIVE);
    else                             ledSetMode(LED_ADV_OFF);
  }

  Serial.printf("[BAT] %.2f V (%d%%)\n", batteryVolts(), pct);
}

void setup() {
  Serial.begin(115200);
  uint32_t t = millis();
  while (!Serial && millis() - t < 2000) delay(10);
  Serial.printf("\n=== CBRN-RN Strahler v%s (ESP32-S3) ===\n", firmwareVersion());

  ledBegin();
  batteryBegin();
  loadConfig();
  bleBegin();

  Serial.printf("[CFG] Q%03d | %s | %u %s | %s | %s\n",
                cfg.sender_nr, nuklidName(cfg.nuklid),
                (unsigned)cfg.max_dosis_100, unit_table[cfg.unit_idx].name,
                cfg.aktiv ? "aktiv" : "aus",
                cfg.long_range ? "Long Range (Coded PHY)" : "Legacy 1M PHY");
  if (otaGetSsid()[0]) Serial.printf("[WLAN] SSID '%s' gespeichert\n", otaGetSsid());
  else                 Serial.println("[WLAN] keine SSID gesetzt - OTA erst nach 0x2B07/0x2B08");

  batteryCheck();
  lastBatCheck = millis();
}

void loop() {
  ledTask();

  // Update-Fenster: laeuft blockierend und endet mit einem Neustart.
  if (otaPending()) {
    otaRun();
  }

  uint32_t now = millis();
  if (now - lastBatCheck >= BAT_CHECK_MS) {
    lastBatCheck = now;
    batteryCheck();
  }

  delay(10);
}
