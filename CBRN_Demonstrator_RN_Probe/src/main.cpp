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

// Alle 5 min einen Display-Refresh ausloesen, damit der Battery-Indikator
// aktuell bleibt. Im OFF-Zustand schlaeft der Prozessor sowieso - dann
// laeuft loop() gar nicht, also kein Refresh.
static constexpr uint32_t BATTERY_REFRESH_MS = 5UL * 60UL * 1000UL;
static uint32_t lastBatteryRefresh = 0;

void setup() {
  Serial.begin(115200);
  uint32_t t = millis();
  while (!Serial && millis() - t < 3000) delay(10);
  Serial.println("\n=== CBRN-RN Strahler ===");

  // Peripherie-Rail (3V3 fuer EPD/LoRa/Flash) einschalten.
  pinMode(Power_Enable_Pin, OUTPUT);
  digitalWrite(Power_Enable_Pin, HIGH);
  delay(50);

  inputBegin();
  batteryBegin();
  loadConfig();

  display.begin();
  Serial.println("[EPD] bereit");

  bleBegin();

  updateDisplay();
  lastBatteryRefresh = millis();
  printStatus();
  Serial.printf("[BAT] %.2f V (%d%%)\n", batteryVolts(), batteryPercent());

  Serial.println("BTN kurz=Menu/Save  BTN lang=Aktion (Edit/AN-AUS/Kontakt/Off)");
  Serial.println("TCH kurz=+1/zurueck/Kontakt-verlassen  TCH lang=Autorepeat");
}

void loop() {
  inputPoll();

  // Periodischer Battery-Refresh - aber nur im Status-View (nicht
  // mitten in einer Kontakt-Vollansicht und nicht waehrend Edit).
  uint32_t now = millis();
  if (view == VIEW_NORMAL && !editMode &&
      now - lastBatteryRefresh >= BATTERY_REFRESH_MS) {
    lastBatteryRefresh = now;
    Serial.printf("[BAT] %.2f V (%d%%)\n", batteryVolts(), batteryPercent());
    updateDisplay();
  }

  delay(10);
}
