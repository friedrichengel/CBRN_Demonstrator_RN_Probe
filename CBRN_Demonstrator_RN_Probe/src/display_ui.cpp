#include "display_ui.h"
#include "config.h"
#include "menu.h"
#include "battery.h"
#include "thw_logo.h"
#include "utilities.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>

// Firmware-Version - in main.cpp definiert.
extern const char* firmwareVersion();

// Eigene SPI-Instanz auf NRF_SPIM0 mit T-Echo-EPD-Pinout.
// (NRF_SPIM0 ist die gleiche Instanz, die auch LilyGos Original-Demo nutzt.)
static SPIClass epdSPI(NRF_SPIM0, ePaper_Miso, ePaper_Sclk, ePaper_Mosi);

Adafruit_SSD1681 display(200, 200,
  ePaper_Dc, ePaper_Rst, ePaper_Cs, /*SRAM_CS=*/-1, ePaper_Busy, &epdSPI);

// Partial-Refresh reduziert das Flackern, kann aber nach vielen
// Aktualisierungen Ghosting verursachen. Full-Refresh wird ausgeloest
// durch *zwei* unabhaengige Trigger - was zuerst eintritt:
//   - 10 Partials seit dem letzten Full       (Interaktions-basiert)
//   - 5 Minuten seit dem letzten Full         (Zeit-basiert, fuer Standby)
// Sobald einer der beiden Trigger feuert, werden BEIDE Zaehler resettet.
// Erster Refresh nach Boot ist immer full.
static constexpr uint16_t MAX_PARTIAL_BEFORE_FULL  = 10;
static constexpr uint32_t MAX_TIME_BEFORE_FULL_MS  = 5UL * 60UL * 1000UL;
static uint16_t partialCount    = MAX_PARTIAL_BEFORE_FULL;  // -> erstes Mal full
static uint32_t lastFullRefresh = 0;

static void refreshEpd() {
  uint32_t now = millis();
  bool needFull = (partialCount >= MAX_PARTIAL_BEFORE_FULL) ||
                  (now - lastFullRefresh >= MAX_TIME_BEFORE_FULL_MS);
  if (needFull) {
    display.display();                          // full - raeumt Ghosting weg
    partialCount    = 0;
    lastFullRefresh = now;
  } else {
    display.displayPartial(0, 0, 199, 199);     // partial - kein Flackern
    partialCount++;
  }
}

// --------------------------------------------------------------
// Off-Screen: THW-Logo (Bitmap aus thw_logo.h), darunter "THW OFRK"
// und "(ausgeschaltet)". E-Paper behaelt das Bild ohne Strom.
// --------------------------------------------------------------
void drawOffScreen() {
  display.clearBuffer();

  // Logo zentriert oben
  int16_t logo_x = (200 - THW_LOGO_W) / 2;
  int16_t logo_y = 5;
  display.drawBitmap(logo_x, logo_y, THW_LOGO,
                     THW_LOGO_W, THW_LOGO_H, EPD_BLACK);

  display.setTextColor(EPD_BLACK);
  display.setFont(&FreeSansBold12pt7b);
  int16_t x1, y1; uint16_t tw, th;
  display.getTextBounds("THW OFRK", 0, 0, &x1, &y1, &tw, &th);
  display.setCursor(100 - tw / 2, 160);
  display.print("THW OFRK");

  display.setFont(&FreeSans9pt7b);
  display.getTextBounds("(ausgeschaltet)", 0, 0, &x1, &y1, &tw, &th);
  display.setCursor(100 - tw / 2, 190);
  display.print("(ausgeschaltet)");

  display.display();
}

// --------------------------------------------------------------
// Kleines Battery-Icon: 14x7 px Rahmen + 2x3 Nubsi + Fuellung
// proportional zum Ladestand 0..100 %.
// Wenn `charging=true` wird statt der Fuellung ein Blitz-Symbol
// im Icon gezeichnet (USB-Versorgung haengt dran).
// --------------------------------------------------------------
static void drawBatteryIcon(int16_t x, int16_t y, int percent, bool charging) {
  display.drawRect(x, y, 14, 7, EPD_BLACK);
  display.fillRect(x + 14, y + 2, 2, 3, EPD_BLACK);
  if (charging) {
    // Stilisierter Blitz: oberes "\" + unteres "/" als zwei Dreiecke
    display.fillTriangle(x + 8, y + 1,  x + 4, y + 4,  x + 7, y + 4, EPD_BLACK);
    display.fillTriangle(x + 9, y + 2,  x + 11, y + 5,  x + 6, y + 5, EPD_BLACK);
  } else {
    int barW = (12 * constrain(percent, 0, 100)) / 100;
    if (barW > 0) display.fillRect(x + 1, y + 1, barW, 5, EPD_BLACK);
  }
}

// --------------------------------------------------------------
// Kontakt-Screen: OV-Adresse, Telefon, E-Mail
// (Strasse statt Straße, weil die Default-Schrift kein ß rendert.)
// --------------------------------------------------------------
static void drawKontaktScreen() {
  display.clearBuffer();
  display.setTextColor(EPD_BLACK);

  display.setFont(&FreeSansBold12pt7b);
  display.setCursor(5, 18); display.print("THW OV Berlin");
  display.setCursor(5, 36); display.print("Friedrichshain-");
  display.setCursor(5, 54); display.print("Kreuzberg");
  display.drawLine(0, 62, 200, 62, EPD_BLACK);

  // Default-Font 6x8 fuer den Adressblock - kompakt, alles passt.
  display.setFont();
  display.setTextSize(1);
  int16_t y = 74;
  display.setCursor(5, y); display.print("General-Pape-Strasse 30"); y += 11;
  display.setCursor(5, y); display.print("12101 Berlin");            y += 16;
  display.setCursor(5, y); display.print("Tel: +49 30 6650899-0");   y += 11;
  display.setCursor(5, y); display.print("Fax: +49 30 6650899-18");  y += 16;
  display.setCursor(5, y); display.print("ov-berlin-friedrichs-");   y += 11;
  display.setCursor(5, y); display.print("hain-kreuzberg@thw.de");

  display.drawLine(0, 182, 200, 182, EPD_BLACK);
  display.setCursor(5, 190);
  display.print("BTN = zurueck");

  refreshEpd();
}

// --------------------------------------------------------------
// Status-Screen: Sender-Nr, Aktiv-Flag, Nuklid, Dosis, Menue-Hinweis
// --------------------------------------------------------------
static void drawStatusScreen() {
  display.clearBuffer();
  display.setTextColor(EPD_BLACK);

  // Battery-Indikator oben rechts (sehr klein, Default-Font 6x8).
  // Wird alle 5 min in loop() ueber updateDisplay() neu eingelesen,
  // bzw. sofort bei USB-Stecken/Ziehen.
  bool charging = batteryCharging();
  int  pct      = batteryPercent();
  drawBatteryIcon(150, 6, pct, charging);
  display.setFont();
  display.setTextSize(1);
  display.setCursor(170, 7);
  if (charging) {
    display.print("USB");
  } else {
    display.printf("%3d%%", pct);
  }

  display.setFont(&FreeSansBold12pt7b);
  display.setCursor(5, 30);
  display.printf("Q%03d", cfg.sender_nr);

  // Status mittig rechts auf der Sender-Zeile: "[AN]"/"[AUS]" + "/LR"
  display.setFont(&FreeSans9pt7b);
  display.setCursor(115, 30);
  if (cfg.long_range) {
    display.print(cfg.aktiv ? "[AN/LR]" : "[AUS/LR]");
  } else {
    display.print(cfg.aktiv ? "[AN]" : "[AUS]");
  }

  display.drawLine(0, 38, 200, 38, EPD_BLACK);

  display.setFont(&FreeSansBold12pt7b);
  display.setCursor(5, 68);
  display.print(nuklid_name[cfg.nuklid]);

  display.setFont(&FreeSans9pt7b);
  display.setCursor(5, 95);
  display.print("Max. Dosisleistung:");

  display.setFont(&FreeSansBold12pt7b);
  display.setCursor(5, 128);
  display.printf("%.2f uSv/h", cfg.max_dosis_100 / 100.0f);

  display.drawLine(0, 158, 200, 158, EPD_BLACK);

  display.setFont(&FreeSans9pt7b);
  display.setCursor(5, 178);
  if (menuState == MENU_STATUS) {
    display.print("kurz/lang/2x BTN");
  } else if (editMode) {
    display.printf("[EDIT] %s", menu_label[menuState]);
  } else {
    display.printf("> %s", menu_label[menuState]);
  }

  // Firmware-Version unten rechts, klein im Default-Font 6x8
  display.setFont();
  display.setTextSize(1);
  const char* ver = firmwareVersion();
  uint16_t vw = (uint16_t)strlen(ver) * 6 + 6;     // +6 fuer das 'v'
  display.setCursor(200 - vw - 2, 192);
  display.printf("v%s", ver);

  refreshEpd();
}

void updateDisplay() {
  if (view == VIEW_KONTAKT) drawKontaktScreen();
  else                       drawStatusScreen();
}
