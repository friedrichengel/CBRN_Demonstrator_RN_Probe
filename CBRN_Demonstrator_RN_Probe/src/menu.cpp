#include "menu.h"
#include "config.h"
#include "display_ui.h"
#include "ble_radio.h"
#include "power.h"
// bluefruit.h-Include nicht mehr noetig - stopAdvertising/startAdvertising
// kapseln den BLE-Zugriff jetzt.

MenuState menuState = MENU_STATUS;
bool      editMode  = false;
View      view      = VIEW_NORMAL;

const char* menu_label[MENU_COUNT] = {
  "Status", "Sender-Nr", "Nuklid", "Max.Dosis", "Aktiv",
  "Reichweite", "Kontakt", "Aus"
};

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

// forward decl - Definition steht weiter unten bei den Touch-Handlern
static void resetTouchDirection();

static bool leaveKontaktView() {
  if (view == VIEW_KONTAKT) {
    view = VIEW_NORMAL;
    menuState = MENU_STATUS;
    updateDisplay();
    return true;
  }
  return false;
}

void onBtnShort() {
  if (leaveKontaktView()) return;
  if (editMode) {
    editMode = false;
    saveConfig();
    if (cfg.aktiv) startAdvertising();
    else           stopAdvertising();
  } else {
    menuState = (MenuState)((menuState + 1) % MENU_COUNT);
  }
  printStatus();
  updateDisplay();
}

void onBtnLong() {
  if (view == VIEW_KONTAKT) return;
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
      editMode = !editMode;
      if (editMode) resetTouchDirection();    // frisch in den Edit-Modus
      break;
  }
  printStatus();
  updateDisplay();
}

void onTouchShort() {
  Serial.println(">>>>>>>>>> TCH onShort");
  if (leaveKontaktView()) return;
  if (editMode) {
    wertAendern(+1);
  } else {
    menuState = (menuState == MENU_STATUS)
      ? (MenuState)(MENU_COUNT - 1)
      : (MenuState)(menuState - 1);
  }
  printStatus();
  updateDisplay();
}

// Richtung fuer die Touch-Autorepeat-Funktion. Wird beim Release einer
// Long-Press-Session umgedreht (NICHT beim Press-Beginn), damit das
// Verhalten mit Toggle-Mode-Touch-ICs funktioniert, die nur eine Flanke
// pro physischer Beruehrung erzeugen.
static int touch_dir = +1;

void onTouchLong() {
  Serial.println(">>>>>>>>>> TCH onLong");
  if (!editMode) { Serial.println(">>>>>>>>>>   skipped (editMode=false)"); return; }
  Serial.printf(">>>>>>>>>>   dir=%+d\n", touch_dir);
  wertAendern(touch_dir);
  printStatus();
  updateDisplay();
}

void onTouchRepeat() {
  if (editMode) {
    wertAendern(touch_dir);
    updateDisplay();
  }
}

void onTouchLongRelease() {
  Serial.println(">>>>>>>>>> TCH onLongRelease");
  if (!editMode) return;
  touch_dir = -touch_dir;
  Serial.printf(">>>>>>>>>>   next dir=%+d\n", touch_dir);
}

// Wird aus onBtnLong aufgerufen, wenn der Edit-Modus betreten wird,
// damit die Touch-Richtung bei jedem Eintritt frisch mit +1 startet.
static void resetTouchDirection() {
  touch_dir = +1;
}
