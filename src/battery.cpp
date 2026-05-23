#include "battery.h"
#include "utilities.h"
#include <nrf.h>

// Adafruit-nRF52-BSP Default: AR_INTERNAL (0.6 V Ref, Gain 1/6) -> max 3.6 V am Pin.
// Mit 12-bit ADC: roher Wert 0..4095 entspricht 0..3.6 V.
static constexpr float ADC_VREF    = 3.6f;
static constexpr float ADC_MAX     = 4096.0f;
static constexpr float VBAT_DIV    = 2.0f;        // 2:1 Spannungsteiler
static constexpr float VBAT_EMPTY  = 3.0f;        // ~ 0 %
static constexpr float VBAT_FULL   = 4.2f;        // ~ 100 %

void batteryBegin() {
  analogReadResolution(12);
  pinMode(Adc_Pin, INPUT);
  // Default-Reference (AR_INTERNAL) ist auf nRF52 schon richtig - 3.6 V Range.
}

float batteryVolts() {
  uint16_t raw = analogRead(Adc_Pin);
  return (float)raw * (ADC_VREF / ADC_MAX) * VBAT_DIV;
}

int batteryPercent() {
  float v = batteryVolts();
  if (v >= VBAT_FULL)  return 100;
  if (v <= VBAT_EMPTY) return 0;
  return (int)((v - VBAT_EMPTY) / (VBAT_FULL - VBAT_EMPTY) * 100.0f);
}

bool batteryCharging() {
  // VBUSDETECT-Bit in NRF_POWER->USBREGSTATUS ist 1 wenn 5 V am USB anliegen.
  return (NRF_POWER->USBREGSTATUS & POWER_USBREGSTATUS_VBUSDETECT_Msk) != 0;
}
