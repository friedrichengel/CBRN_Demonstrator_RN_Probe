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
// PROBE-MODUS: auf 1 setzen wenn der echte Touch-Pin gesucht
// werden soll. Macht aus dem Geraet einen einfachen Pin-Scanner,
// der bei jeder Aenderung an einer Liste von Kandidaten-Pins
// eine Serial-Zeile schreibt. Beruehre das Touchfeld -> der
// reagierende Pin meldet sich.
// Danach wieder auf 0 setzen!
// =============================================================
#define RUN_TOUCH_PROBE 0

#if RUN_TOUCH_PROBE
#include <nrf_gpio.h>

// Liste der Kandidaten - alle plausiblen T-Echo-Pins ausser BTN und den
// kritischen System-Pins (XTAL, Reset). P1.10 (BTN) als Sanity-Check drin.
static const uint32_t kandidaten[] = {
  // P0
   2,  3,  4,  5,  6,  7,  8,  9, 10, 11,
                  13, 14, 15, 16, 17,
  19, 20, 21, 22, 23, 24, 25, 26, 27,
              28, 29, 30, 31,
  // P1  (32 + N)
  32+0,  32+1,  32+2,  32+3,  32+4,  32+5,  32+6,  32+7,
  32+8,  32+9,  32+10, 32+11, 32+12, 32+13, 32+14, 32+15,
};
static constexpr size_t N_KAND = sizeof(kandidaten) / sizeof(kandidaten[0]);
static bool last_state[N_KAND];

void setup() {
  Serial.begin(115200);
  uint32_t t = millis();
  while (!Serial && millis() - t < 3000) delay(10);
  Serial.println("\n=== TOUCH-PROBE v3 ===");

  // Peripherie-Rail einschalten - sonst hat der Touch-IC keinen Strom
  nrf_gpio_cfg_output(12);                          // P0.12 = Power_Enable
  nrf_gpio_pin_clear(12);
  delay(200);
  nrf_gpio_pin_set(12);                             // jetzt HIGH = Rail an
  Serial.println("Power_Enable HIGH - 800 ms warten...");
  delay(800);

  // Kandidaten direkt via nrf_gpio konfigurieren - umgeht jeden BSP-Variant
  // der eventuell schon Pins als OUTPUT konfiguriert hat.
  for (size_t i = 0; i < N_KAND; i++) {
    nrf_gpio_cfg_input(kandidaten[i], NRF_GPIO_PIN_PULLUP);
  }
  delay(50);
  for (size_t i = 0; i < N_KAND; i++) {
    last_state[i] = nrf_gpio_pin_read(kandidaten[i]);
  }

  Serial.println("\n-- Initial state --");
  for (size_t i = 0; i < N_KAND; i++) {
    uint32_t p = kandidaten[i];
    Serial.printf("  P%lu.%02lu = %s\n",
                  (unsigned long)(p / 32), (unsigned long)(p % 32),
                  last_state[i] ? "HIGH" : "LOW");
  }
  Serial.println("\n>>> 1) Erst BTN druecken (Sanity, sollte P1.10 melden)");
  Serial.println(">>> 2) Dann Touchfeld antippen\n");
}

void loop() {
  for (size_t i = 0; i < N_KAND; i++) {
    bool now = nrf_gpio_pin_read(kandidaten[i]);
    if (now != last_state[i]) {
      uint32_t p = kandidaten[i];
      Serial.printf("  P%lu.%02lu : %s -> %s @ %lu ms\n",
                    (unsigned long)(p / 32), (unsigned long)(p % 32),
                    last_state[i] ? "HIGH" : "LOW",
                    now           ? "HIGH" : "LOW",
                    millis());
      last_state[i] = now;
    }
  }
  delay(5);
}
#else
// ===== Normaler App-Betrieb unten =====

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

  // Peripherie-Rail (3V3 fuer EPD/LoRa/Flash/TTP223) einschalten.
  // WICHTIG zur Reihenfolge: der TTP223 kalibriert beim Power-on seine
  // Ruhe-Kapazitaet. Waehrend dieser ~500 ms KEINE pinMode(PIN_TOUCH, ...)
  // - unsere ~13 kOhm Pull-Up wuerde sonst in die Baseline einrechnen
  // und der IC bleibt anschliessend stuck-LOW.
  pinMode(Power_Enable_Pin, OUTPUT);
  digitalWrite(Power_Enable_Pin, HIGH);

  Serial.println("[TTP223] Kalibrierungsphase 1500 ms (Touch-Pin floatet)...");
  delay(1500);

  // JETZT erst die Input-Pins. Touch-IC hat seine Baseline gemessen,
  // unser Pull-Up stoert die Logic nicht mehr (CMOS Push-Pull-Output
  // ueberschreibt den schwachen Pull-Up locker).
  inputBegin();
  batteryBegin();
  loadConfig();
  Serial.printf("[TTP223] Initial-State von P0.11 = %s\n",
                digitalRead(PIN_TOUCH) ? "HIGH (idle, korrekt)" : "LOW (BEREITS GETRIGGERT!)");

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

  uint32_t now = millis();

  // Periodischer Battery-Refresh - aber nur im Status-View (nicht
  // mitten in einer Kontakt-Vollansicht und nicht waehrend Edit).
  if (view == VIEW_NORMAL && !editMode &&
      now - lastBatteryRefresh >= BATTERY_REFRESH_MS) {
    lastBatteryRefresh = now;
    Serial.printf("[BAT] %.2f V (%d%%)\n", batteryVolts(), batteryPercent());
    updateDisplay();
  }

  // Periodische Touch-Zustands-Anzeige - nur bei DEBUG_VERBOSE
  static uint32_t lastTouchPoll = 0;
  if (now - lastTouchPoll >= 2000) {
    lastTouchPoll = now;
    DBG_PRINTF(">>>>>>>>>> TCH state @%lu: %s\n",
               now, digitalRead(PIN_TOUCH) ? "HIGH" : "LOW");
  }

  delay(10);
}
#endif  // RUN_TOUCH_PROBE
