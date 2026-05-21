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
  "Status", "Sender-Nr", "Nuklid", "Max.Dosis", "Aktiv",
  "Reichweite", "Kontakt", "Aus"
};

// Richtung fuer den BTN-Autorepeat im Edit-Modus. Wird beim Release einer
// Long-Press-Session umgedreht (toggle nach jedem Halten):
// 1. Halten zaehlt hoch, 2. runter, 3. hoch, ...
// Wird beim Eintritt in den Edit-Modus auf +1 zurueckgesetzt.
static int btn_dir = +1;

void wertAendern(int delta) {
  switch (menuState) {
    case MENU_SENDER_NR:
      cfg.sender_nr = (uint8_t)constrain((int)cfg.sender_nr + delta, 1, 254);
      break;
    case MENU_NUKLID:
      cfg.nuklid = (uint8_t)(
        ((int)(cfg.nuklid - 1) + delta + NUKLID_COUNT) % NUKLID_COUNT + 1);
      break;
    case MENU_DOSIS: {
      int s = (cfg.max_dosis_100 < 10000)  ? 50    :
              (cfg.max_dosis_100 < 100000) ? 1000  : 10000;
      cfg.max_dosis_100 = (uint16_t)constrain(
        (int)cfg.max_dosis_100 + delta * s, 50, 600000);
      break;
    }
    case MENU_AKTIV:
      cfg.aktiv = !cfg.aktiv;
      break;
    case MENU_REICHWEITE:
      cfg.long_range = !cfg.long_range;
      break;
    default: break;
  }
}

void printStatus() {
  Serial.printf("[STATUS] Q%03d | %s | %.2f uSv/h | %s | Menu:%s %s\n",
    cfg.sender_nr, nuklid_name[cfg.nuklid], cfg.max_dosis_100 / 100.0f,
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
    // Erster Tick der Autorepeat-Session: aktuelle Richtung anwenden
    DBG_PRINTF(">>>>>>>>>>   dir=%+d\n", btn_dir);
    wertAendern(btn_dir);
    printStatus();
    updateDisplay();
    return;
  }
  // Ausserhalb Edit: Aktion abhaengig vom Menuepunkt
  switch (menuState) {
    case MENU_STATUS:
      cfg.aktiv = !cfg.aktiv;
      saveConfig();
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
      // Wertpunkt: Edit-Modus betreten, Richtung frisch starten
      editMode = true;
      btn_dir  = +1;
      break;
  }
  printStatus();
  updateDisplay();
}

void onBtnRepeat() {
  if (!editMode) return;
  if (isBoolField()) return;     // Bool nicht in Autorepeat
  wertAendern(btn_dir);
  updateDisplay();
}

void onBtnLongRelease() {
  if (!editMode) return;
  if (isBoolField()) return;     // Bool: Richtung egal
  btn_dir = -btn_dir;
  DBG_PRINTF(">>>>>>>>>> BTN long-release, next dir=%+d\n", btn_dir);
}
