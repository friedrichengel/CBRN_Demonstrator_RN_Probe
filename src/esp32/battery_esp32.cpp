#include "battery.h"
#include "esp32/board_xiao_s3.h"

// ================================================================
// Akku-Messung auf dem XIAO ESP32S3.
//
// Der XIAO hat KEINEN eingebauten Batterie-ADC - es muss ein
// Spannungsteiler 2 x 200 kOhm von BAT+ nach GND geloetet und der
// Mittelabgriff auf A0 (GPIO1) gefuehrt werden (100 nF nach GND).
// analogReadMilliVolts() rechnet die ADC-Kalibrierung des Chips
// bereits ein, deshalb keine eigene Referenzspannung noetig.
// ================================================================

void batteryBegin() {
  analogReadResolution(12);
  pinMode(BAT_ADC_PIN, INPUT);
}

static uint32_t batteryMillivolts() {
  uint32_t sum = 0;
  for (uint8_t i = 0; i < BAT_ADC_SAMPLES; ++i) {
    sum += analogReadMilliVolts(BAT_ADC_PIN);
  }
  return (uint32_t)((float)(sum / BAT_ADC_SAMPLES) * BAT_ADC_DIVIDER);
}

float batteryVolts() {
  return batteryMillivolts() / 1000.0f;
}

int batteryPercent() {
  uint32_t mv = batteryMillivolts();
  if (mv >= BAT_FULL_MV)  return 100;
  if (mv <= BAT_EMPTY_MV) return 0;
  return (int)((mv - BAT_EMPTY_MV) * 100 / (BAT_FULL_MV - BAT_EMPTY_MV));
}

bool batteryCharging() {
  // Ein echtes VBUS-Detect gibt es auf dem XIAO nicht (kein Pin des
  // Lade-ICs herausgefuehrt). Naeherung: USB-CDC-Host verbunden.
  // Laden an einem reinen Netzteil wird damit NICHT erkannt.
  return (bool)Serial;
}
