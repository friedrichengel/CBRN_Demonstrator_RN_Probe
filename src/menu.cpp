#include "menu.h"
#include "config.h"
#include "display_ui.h"
#include "ble_radio.h"
#include "power.h"
#include "debug.h"

MenuState menuState = MENU_STATUS;
bool      editMode  = false;
View      view      = VIEW_NORMAL;

const char* menu_label[MENU_COUNT] = {
  "Status", "Sender-Nr", "Nuklid", "Max.Dosis", "Einheit", "Aktiv",
  "Reichweite", "Kontakt", "Aus"
};

// Richtung fuer den BTN-Autorepeat im Edit-Modus. Wird beim Release einer
// Long-Press-Session umgedreht (toggle nach jedem Halten):
// 1. Halten zaehlt hoch, 2. runter, 3. hoch, ...
// Wird beim Eintritt in den Edit-Modus auf +1 zurueckgesetzt.
static int btn_dir = +1;

// Autorepeat-Beschleunigung (Treppe): alle REPEAT_STEP_TICKS Ticks waechst der
// Schritt um Faktor 10. Sequenz also +/-1, dann +/-10, dann +/-100, ...
// Beim Loslassen oder beim Eintritt in den Edit-Modus zurueckgesetzt, das
// naechste Halten startet wieder langsam.
// Nuklid- und Einheits-Auswahl machen keine Acceleration (siehe currentDelta()).
static constexpr uint8_t REPEAT_STEP_TICKS = 10;
static constexpr int     REPEAT_MAX_MAG    = 1000000;   // Schutz vor int-Ueberlauf
static uint8_t repeatCount = 0;

static int currentDelta() {
  // Diskrete Menues (Nuklid, Einheit) bleiben immer auf +/-1, da Acceleration
  // bei kleinen Zyklen (4-8 Elemente) nichts bringt und das Bedienen verschlechtert.
  if (menuState == MENU_NUKLID || menuState == MENU_EINHEIT) {
    return btn_dir;
  }
  // Treppen-Acceleration: Schrittweite x10 pro vollendeter REPEAT_STEP_TICKS-Phase.
  int mag = 1;
  for (int n = (int)repeatCount;
       n > REPEAT_STEP_TICKS && mag < REPEAT_MAX_MAG;
       n -= REPEAT_STEP_TICKS) {
    mag *= 10;
  }
  return btn_dir * mag;
}

void wertAendern(int delta) {
  switch (menuState) {
    case MENU_SENDER_NR:
      cfg.sender_nr = (uint8_t)constrain((int)cfg.sender_nr + delta, 1, 254);
      break;
    case MENU_NUKLID: {
      // Zyklisch ueber die Tabellen-Position, nicht ueber die ID - die IDs
      // sind nicht lueckenlos (1..8, dann 99 fuer Se-75).
      int idx = nuklidIndex(cfg.nuklid);
      if (idx < 0) idx = 0;
      idx = (idx + delta % NUKLID_COUNT + NUKLID_COUNT) % NUKLID_COUNT;
      cfg.nuklid = nuklid_table[idx].id;
      break;
    }
    case MENU_DOSIS:
      // Reines +/-delta ohne wertabhaengige Skalierung. Die Stufung
      // (1 -> 10 -> 100 -> ...) macht die Autorepeat-Acceleration in
      // currentDelta(). Range = ganzer uint16-Bereich.
      cfg.max_dosis_100 = (uint16_t)constrain(
        (int)cfg.max_dosis_100 + delta, 0, 65535);
      break;
    case MENU_EINHEIT:
      cfg.unit_idx = (uint8_t)(
        ((int)cfg.unit_idx + delta + UNIT_COUNT) % UNIT_COUNT);
      break;
    case MENU_AKTIV:
      cfg.aktiv = !cfg.aktiv;
      break;
    case MENU_REICHWEITE:
      cfg.long_range = !cfg.long_range;
      break;
    default: break;
  }
  // Live-Sync der GATT-Attribute, damit ein evtl. verbundener Client beim
  // naechsten Read den aktuellen Wert sieht statt seinen letzten Write.
  bleSyncChars();
}

void printStatus() {
  Serial.printf("[STATUS] Q%03d | %s | %u %s | %s | Menu:%s %s\n",
    cfg.sender_nr, nuklidName(cfg.nuklid),
    (unsigned)cfg.max_dosis_100, unit_table[cfg.unit_idx].name,
    cfg.aktiv ? "AN" : "AUS",
    menu_label[menuState], editMode ? "[EDIT]" : "");
}

static bool leaveKontaktView() {
  if (view == VIEW_KONTAKT) {
    view = VIEW_NORMAL;
    menuState = MENU_STATUS;
    updateDisplay();
    return true;
  }
  return false;
}

static void saveAndExitEdit() {
  editMode = false;
  saveConfig();
  if (cfg.aktiv) startAdvertising();
  else           stopAdvertising();
}

// Autorepeat fuer Bool-Felder unterdruecken, sonst flippen sie hin und her.
// Erstes onBtnLong wendet schon einmal an, das reicht zum Toggeln.
static bool isBoolField() {
  return menuState == MENU_AKTIV || menuState == MENU_REICHWEITE;
}

void onBtnShort() {
  DBG_PRINTLN(">>>>>>>>>> BTN short");
  if (leaveKontaktView()) return;
  if (editMode) {
    wertAendern(+1);
  } else {
    menuState = (MenuState)((menuState + 1) % MENU_COUNT);
  }
  printStatus();
  updateDisplay();
}

void onBtnDouble() {
  DBG_PRINTLN(">>>>>>>>>> BTN double");
  if (leaveKontaktView()) return;
  if (editMode) {
    saveAndExitEdit();
  } else {
    menuState = (menuState == MENU_STATUS)
      ? (MenuState)(MENU_COUNT - 1)
      : (MenuState)(menuState - 1);
  }
  printStatus();
  updateDisplay();
}

void onBtnLong() {
  DBG_PRINTLN(">>>>>>>>>> BTN long");
  if (view == VIEW_KONTAKT) {
    // jede Geste in Kontakt-Vollansicht: zurueck zu Status
    leaveKontaktView();
    return;
  }
  if (editMode) {
    // Erster Tick der Autorepeat-Session: zaehlt fuer die Acceleration.
    repeatCount++;
    DBG_PRINTF(">>>>>>>>>>   dir=%+d cnt=%u\n", btn_dir, (unsigned)repeatCount);
    wertAendern(currentDelta());
    printStatus();
    updateDisplay();
    return;
  }
  // Ausserhalb Edit: Aktion abhaengig vom Menuepunkt
  switch (menuState) {
    case MENU_STATUS:
      cfg.aktiv = !cfg.aktiv;
      saveConfig();
      bleSyncChars();
      if (cfg.aktiv) startAdvertising();
      else           stopAdvertising();
      break;
    case MENU_KONTAKT:
      view = VIEW_KONTAKT;
      break;
    case MENU_OFF:
      powerOff();      // kehrt nicht zurueck (System OFF)
      break;
    default:
      // Wertpunkt: Edit-Modus betreten, Richtung + Beschleunigungszaehler frisch starten
      editMode    = true;
      btn_dir     = +1;
      repeatCount = 0;
      break;
  }
  printStatus();
  updateDisplay();
}

void onBtnRepeat() {
  if (!editMode) return;
  if (isBoolField()) return;     // Bool nicht in Autorepeat
  repeatCount++;
  wertAendern(currentDelta());
  updateDisplay();
}

void onBtnLongRelease() {
  if (!editMode) return;
  if (isBoolField()) return;     // Bool: Richtung egal
  btn_dir     = -btn_dir;
  repeatCount = 0;               // naechste Halten-Session beginnt wieder langsam
  DBG_PRINTF(">>>>>>>>>> BTN long-release, next dir=%+d\n", btn_dir);
}
