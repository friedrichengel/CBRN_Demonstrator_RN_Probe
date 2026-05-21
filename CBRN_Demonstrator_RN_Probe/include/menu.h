#pragma once
#include <Arduino.h>

// Reihenfolge der Menue-Punkte beim Durchschalten.
// MENU_COUNT muss IMMER letzter Eintrag sein.
enum MenuState : uint8_t {
  MENU_STATUS = 0,
  MENU_SENDER_NR,
  MENU_NUKLID,
  MENU_DOSIS,
  MENU_AKTIV,
  MENU_REICHWEITE,
  MENU_KONTAKT,
  MENU_OFF,
  MENU_COUNT
};

// Welcher Vollbild-Screen wird gerade gezeichnet?
// VIEW_NORMAL = Status-/Menue-Ansicht
// VIEW_KONTAKT = Vollbild-Adresskarte des OV
enum View : uint8_t { VIEW_NORMAL, VIEW_KONTAKT };

extern MenuState   menuState;
extern bool        editMode;
extern View        view;
extern const char* menu_label[MENU_COUNT];

void wertAendern(int delta);
void printStatus();

// Vom Input-Modul aufgerufene Handler:
void onBtnShort();
void onBtnLong();
void onTouchShort();
void onTouchLong();
void onTouchRepeat();
void onTouchLongRelease();
