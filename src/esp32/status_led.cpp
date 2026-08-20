#include "esp32/status_led.h"
#include "esp32/board_xiao_s3.h"

// Muster als Liste von Phasen: {Dauer in ms, LED an?}.
// Die Liste wird zyklisch durchlaufen.
struct Phase { uint16_t ms; bool on; };

static const Phase PAT_ADV_ACTIVE[] = {{  60, true}, {2940, false}};
static const Phase PAT_ADV_OFF[]    = {{  60, true}, { 200, false}, {  60, true}, {2680, false}};
static const Phase PAT_BAT_LOW[]    = {{  60, true}, { 200, false}, {  60, true}, { 200, false},
                                       {  60, true}, {2420, false}};
static const Phase PAT_OTA[]        = {{ 100, true}, { 100, false}};

static LedMode      mode        = LED_OFF;
static const Phase* pattern     = nullptr;
static uint8_t      patternLen  = 0;
static uint8_t      phaseIdx    = 0;
static uint32_t     phaseStart  = 0;

static inline void drive(bool on) {
#if LED_STATUS_ACTIVE_HIGH
  digitalWrite(LED_STATUS_PIN, on ? HIGH : LOW);
#else
  digitalWrite(LED_STATUS_PIN, on ? LOW : HIGH);
#endif
}

void ledBegin() {
  pinMode(LED_STATUS_PIN, OUTPUT);
  drive(false);
  ledSetMode(LED_ADV_ACTIVE);
}

void ledSetMode(LedMode m) {
  if (m == mode) return;
  mode       = m;
  phaseIdx   = 0;
  phaseStart = millis();

  switch (m) {
    case LED_ADV_ACTIVE: pattern = PAT_ADV_ACTIVE; patternLen = 2; break;
    case LED_ADV_OFF:    pattern = PAT_ADV_OFF;    patternLen = 4; break;
    case LED_BAT_LOW:    pattern = PAT_BAT_LOW;    patternLen = 6; break;
    case LED_OTA:        pattern = PAT_OTA;        patternLen = 2; break;
    default:             pattern = nullptr;        patternLen = 0; break;
  }

  // Statische Zustaende sofort setzen.
  if (!pattern) drive(m == LED_ON_SOLID);
  else          drive(pattern[0].on);
}

LedMode ledGetMode() { return mode; }

void ledTask() {
  if (!pattern) return;
  uint32_t now = millis();
  if (now - phaseStart < pattern[phaseIdx].ms) return;
  phaseStart += pattern[phaseIdx].ms;
  phaseIdx = (uint8_t)((phaseIdx + 1) % patternLen);
  drive(pattern[phaseIdx].on);
}
