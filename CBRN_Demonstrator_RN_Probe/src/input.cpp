#include "input.h"
#include "menu.h"
#include "debug.h"

static constexpr uint32_t DEBOUNCE_MS    = 50;
static constexpr uint32_t LONG_PRESS_MS  = 800;
static constexpr uint32_t AUTOREPEAT_MS  = 250;
// Mindestdauer in ms, die das Signal stabil HIGH gewesen sein muss,
// bevor wir das naechste LOW als "neuer Press" akzeptieren. Filtert
// kurze Wackler von kapazitiven Touch-ICs aus, die sonst ein
// laufendes Long-Press-Event mehrfach starten wuerden.
static constexpr uint32_t PRESS_GUARD_MS = 100;

struct InputState {
  uint8_t  pin;
  bool     lastRaw;
  uint32_t pressTime;
  uint32_t lastRepeat;
  uint32_t releaseTime;       // wann das Signal zuletzt HIGH wurde (0 = noch nie)
  bool     longFired;
};
static InputState stBtn   = {PIN_BTN,   HIGH, 0, 0, 0, false};
static InputState stTouch = {PIN_TOUCH, HIGH, 0, 0, 0, false};

// Liest den Pin mehrfach und gibt den Mehrheitswert zurueck.
// Filtert EMI-Spikes vom EPD-Partial-Refresh aus, die sonst den
// kapazitiven Touch-IC versehentlich triggern wuerden.
static bool readDebounced(uint8_t pin, uint8_t samples = 5) {
  uint8_t low_count = 0;
  for (uint8_t i = 0; i < samples; i++) {
    if (digitalRead(pin) == LOW) low_count++;
    if (i < samples - 1) delayMicroseconds(200);
  }
  return (low_count > samples / 2) ? LOW : HIGH;
}

// Polaritaet des Touch-IC. 0 = normal active-low (Beruehrung = LOW),
// 1 = invertiert (Beruehrung = HIGH). Auf dem T-Echo mit TTP223 ist
// 0 korrekt.
#define TOUCH_INVERT 0

static void processInput(InputState& s,
                         void(*onShort)(), void(*onLong)(), void(*onRepeat)(),
                         void(*onLongRelease)() = nullptr) {
  // Touch wird mit Mehrfach-Sampling gelesen (EMI-Filter),
  // Button als zuverlaessiger mechanischer Schalter nur einmal.
  // Touch ist optional invertiert je nach TOUCH_INVERT (siehe oben).
  bool raw;
  if (s.pin == PIN_TOUCH) {
    raw = readDebounced(s.pin);
#if TOUCH_INVERT
    raw = !raw;
#endif
  } else {
    raw = digitalRead(s.pin);
  }
  uint32_t now = millis();

  // Touch-Edges nur bei DEBUG_VERBOSE loggen
  if (s.pin == PIN_TOUCH && raw != s.lastRaw) {
    DBG_PRINTF(">>>>>>>>>> TCH edge: %s @%lu (releaseTime=%lu)\n",
               raw ? "HIGH" : "LOW", now, s.releaseTime);
  }

  // HIGH -> LOW Flanke = potentieller Press-Beginn.
  // Nur akzeptieren, wenn vorher mindestens PRESS_GUARD_MS stabil HIGH war,
  // sonst handelt es sich um einen Wackler innerhalb eines laufenden Drucks.
  if (raw == LOW && s.lastRaw == HIGH) {
    bool fresh = (s.releaseTime == 0) ||
                 (now - s.releaseTime >= PRESS_GUARD_MS);
    if (fresh) {
      s.pressTime  = now;
      s.lastRepeat = now;
      s.longFired  = false;
    }
    // else: kurzer Bounce, longFired/pressTime bleiben erhalten
  }
  // Gehalten
  if (raw == LOW) {
    if (!s.longFired && (now - s.pressTime >= LONG_PRESS_MS)) {
      s.longFired = true;
      if (onLong) onLong();
    }
    if (s.longFired && onRepeat && (now - s.lastRepeat >= AUTOREPEAT_MS)) {
      s.lastRepeat = now;
      onRepeat();
      // Direkt nach dem (oft blockierenden) Callback erneut pruefen, ob
      // der Pin schon HIGH ist - dann sofort Autorepeat stoppen, statt
      // bis zur naechsten Loop-Iteration zu warten. Den Release-Callback
      // MUESSEN wir dabei selbst feuern, sonst verpassen wir ihn (der
      // HIGH-Edge-Handler unten wird durch das fruehe return uebersprungen).
      if (digitalRead(s.pin) == HIGH) {
        s.releaseTime = millis();
        s.lastRaw = HIGH;
        if (onLongRelease) onLongRelease();
        return;
      }
    }
  }
  // LOW -> HIGH Flanke = Loslassen
  if (raw == HIGH && s.lastRaw == LOW) {
    s.releaseTime = now;
    if (!s.longFired && (now - s.pressTime >= DEBOUNCE_MS)) {
      if (onShort) onShort();
    } else if (s.longFired && onLongRelease) {
      onLongRelease();
    }
  }
  s.lastRaw = raw;
}

void inputBegin() {
  pinMode(PIN_BTN,   INPUT_PULLUP);
  pinMode(PIN_TOUCH, INPUT_PULLUP);
}

void inputPoll() {
  processInput(stBtn,   onBtnShort,   onBtnLong, nullptr);
  processInput(stTouch, onTouchShort, onTouchLong, onTouchRepeat, onTouchLongRelease);
}
