#include "input.h"
#include "menu.h"
#include "debug.h"

// Timing-Konstanten der Bedienlogik.
static constexpr uint32_t DEBOUNCE_MS      = 50;
static constexpr uint32_t LONG_PRESS_MS    = 800;
static constexpr uint32_t AUTOREPEAT_MS    = 250;
// Zeitfenster fuer Doppelklick-Erkennung: zweiter Druck muss innerhalb
// dieser Zeit nach Loslassen des ersten beginnen.
static constexpr uint32_t DOUBLE_TAP_MS    = 350;
// Mindestdauer fuer HIGH zwischen zwei LOW-Phasen, damit ein neuer Press
// gezaehlt wird (filtert Bouncing innerhalb eines Druckes).
static constexpr uint32_t PRESS_GUARD_MS   = 30;

struct InputState {
  uint8_t  pin;
  bool     lastRaw;
  uint32_t pressTime;
  uint32_t lastRepeat;
  uint32_t releaseTime;     // wann das Signal zuletzt HIGH wurde (0 = noch nie)
  bool     longFired;
  // Doppelklick-Tracking:
  bool     pendingShort;    // ein Kurzdruck wartet auf moegliches Doppel
  uint32_t pendingShortAt;  // wann der wartende Kurzdruck registriert wurde
  bool     inDoubleTap;     // aktueller Druck ist die zweite Haelfte
};
static InputState stBtn = {PIN_BTN, HIGH, 0, 0, 0, false, false, 0, false};

static void processInput(InputState& s,
                         void(*onShort)(),
                         void(*onLong)(),
                         void(*onRepeat)(),
                         void(*onLongRelease)() = nullptr,
                         void(*onDouble)() = nullptr) {
  bool raw = digitalRead(s.pin);
  uint32_t now = millis();

  // Wenn ein Kurzdruck wartet und das Doppelklick-Fenster abgelaufen ist:
  // jetzt erst feuern (kein Doppel mehr moeglich).
  if (s.pendingShort && (now - s.pendingShortAt >= DOUBLE_TAP_MS)) {
    s.pendingShort = false;
    if (onShort) onShort();
  }

  // HIGH -> LOW: potentieller Press-Beginn
  if (raw == LOW && s.lastRaw == HIGH) {
    bool fresh = (s.releaseTime == 0) ||
                 (now - s.releaseTime >= PRESS_GUARD_MS);
    if (fresh) {
      s.pressTime  = now;
      s.lastRepeat = now;
      s.longFired  = false;
      // Schon ein Kurzdruck im Doppelklick-Fenster? -> dieser Druck koennte
      // die zweite Haelfte sein. Aber nur wenn der ZWEITE Druck auch wieder
      // kurz ist - das entscheidet sich beim Release.
      if (s.pendingShort && (now - s.pendingShortAt < DOUBLE_TAP_MS)) {
        s.inDoubleTap = true;
      } else {
        s.inDoubleTap = false;
      }
    }
  }
  // Gehalten
  if (raw == LOW) {
    if (!s.longFired && (now - s.pressTime >= LONG_PRESS_MS)) {
      s.longFired = true;
      // Wird der zweite Druck zu einem Long-Press, ist es kein Doppelklick.
      // Den noch ausstehenden ersten Kurzdruck jetzt nachholen.
      if (s.inDoubleTap) {
        s.inDoubleTap = false;
        if (s.pendingShort) {
          s.pendingShort = false;
          if (onShort) onShort();
        }
      }
      if (onLong) onLong();
    }
    if (s.longFired && onRepeat && (now - s.lastRepeat >= AUTOREPEAT_MS)) {
      s.lastRepeat = now;
      onRepeat();
      // Falls der User waehrend des oft blockierenden Callbacks losgelassen
      // hat: sofort stoppen und Release-Handler aufrufen.
      if (digitalRead(s.pin) == HIGH) {
        s.releaseTime = millis();
        s.lastRaw     = HIGH;
        if (onLongRelease) onLongRelease();
        return;
      }
    }
  }
  // LOW -> HIGH: Loslassen
  if (raw == HIGH && s.lastRaw == LOW) {
    s.releaseTime = now;
    if (s.longFired) {
      if (onLongRelease) onLongRelease();
    } else if (s.inDoubleTap) {
      // zweiter Druck war kurz -> Doppelklick bestaetigt
      s.inDoubleTap  = false;
      s.pendingShort = false;
      if (onDouble) onDouble();
    } else if (now - s.pressTime >= DEBOUNCE_MS) {
      // Kurzdruck, koennte noch zu einem Doppel werden -> erstmal vormerken
      s.pendingShort   = true;
      s.pendingShortAt = now;
    }
  }
  s.lastRaw = raw;
}

void inputBegin() {
  pinMode(PIN_BTN, INPUT_PULLUP);
}

void inputPoll() {
  processInput(stBtn,
               onBtnShort,
               onBtnLong,
               onBtnRepeat,
               onBtnLongRelease,
               onBtnDouble);
}
