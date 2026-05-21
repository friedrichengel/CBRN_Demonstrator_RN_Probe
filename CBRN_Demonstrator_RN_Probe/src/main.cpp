// ================================================================
// CBRN-RN Trainingsgeraet - Strahler-Firmware
// Hardware: LilyGO T-Echo (nRF52840)
//
// Diese Datei enthaelt nur Setup und Hauptschleife. Die eigentliche
// Logik ist auf Module verteilt - siehe README.md fuer den Ueberblick.
// ================================================================

#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include "utilities.h"
#include "config.h"
#include "display_ui.h"
#include "ble_radio.h"
#include "input.h"
#include "menu.h"
#include "battery.h"
#include "debug.h"

// =============================================================
// Firmware-Version. Format auf dem Display: "X.YY"
// (Major.Minor, Minor zweistellig). Beide Werte hier editieren.
// =============================================================
constexpr uint8_t VERSION_MAJOR = 1;
constexpr uint8_t VERSION_MINOR = 0;

// Globaler Versions-String, von display_ui.cpp via extern verwendet.
const char* firmwareVersion() {
  static char buf[8];
  snprintf(buf, sizeof(buf), "%u.%02u",
           (unsigned)VERSION_MAJOR, (unsigned)VERSION_MINOR);
  return buf;
}

// Alle 5 min einen Display-Refresh ausloesen, damit der Battery-Indikator
// aktuell bleibt. Im OFF-Zustand schlaeft der Prozessor sowieso - dann
// laeuft loop() gar nicht, also kein Refresh.
static constexpr uint32_t BATTERY_REFRESH_MS = 5UL * 60UL * 1000UL;
static uint32_t lastBatteryRefresh = 0;

// Letzter bekannter USB-Status. Wechsel -> sofortiger Display-Refresh,
// sonst zeigt der Battery-Indikator bis zum naechsten 5-min-Tick weiter
// den alten Zustand (insbesondere "USB" obwohl Kabel weg ist).
static bool lastUsbState = false;

void setup() {
  Serial.begin(115200);
  uint32_t t = millis();
  while (!Serial && millis() - t < 3000) delay(10);
  Serial.printf("\n=== CBRN-RN Strahler v%s ===\n", firmwareVersion());

  // Peripherie-Rail (3V3 fuer EPD/LoRa/Flash) einschalten.
  pinMode(Power_Enable_Pin, OUTPUT);
  digitalWrite(Power_Enable_Pin, HIGH);
  delay(100);

  inputBegin();
  batteryBegin();
  loadConfig();

  display.begin();
  Serial.println("[EPD] bereit");

  bleBegin();

  updateDisplay();
  lastBatteryRefresh = millis();
  lastUsbState = batteryCharging();
  printStatus();
  Serial.printf("[BAT] %.2f V (%d%%)  USB=%s\n",
                batteryVolts(), batteryPercent(),
                lastUsbState ? "ja" : "nein");

  Serial.println("BTN kurz=Menu vorwaerts/+1   doppel=Menu rueckwaerts/Save+Exit");
  Serial.println("BTN lang=Aktion / Autorepeat im Edit");
}

void loop() {
  inputPoll();

  uint32_t now = millis();

  // USB-Stecker-Wechsel sofort als Display-Refresh, damit der Indikator
  // nicht 5 min auf altem Wert haengen bleibt.
  bool nowUsb = batteryCharging();
  if (nowUsb != lastUsbState) {
    lastUsbState = nowUsb;
    Serial.printf("[USB] %s\n", nowUsb ? "verbunden" : "getrennt");
    if (view == VIEW_NORMAL && !editMode) {
      lastBatteryRefresh = now;
      updateDisplay();
    }
  }

  // Periodischer Battery-Refresh - aber nur im Status-View (nicht
  // mitten in einer Kontakt-Vollansicht und nicht waehrend Edit).
  if (view == VIEW_NORMAL && !editMode &&
      now - lastBatteryRefresh >= BATTERY_REFRESH_MS) {
    lastBatteryRefresh = now;
    Serial.printf("[BAT] %.2f V (%d%%)  USB=%s\n",
                  batteryVolts(), batteryPercent(),
                  nowUsb ? "ja" : "nein");
    updateDisplay();
  }

  delay(10);
}
