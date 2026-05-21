#include "power.h"
#include "display_ui.h"
#include "input.h"
#include "ble_radio.h"
#include "utilities.h"
#include <Arduino.h>
#include <nrf_gpio.h>
#include <nrf_soc.h>

void powerOff() {
  Serial.println("[OFF] gehe schlafen...");
  stopAdvertising();

  // Off-Screen zeichnen - blockiert, bis der EPD-Refresh durch ist.
  drawOffScreen();
  delay(100);

  // Nur der Hauptknopf als SENSE-Wake-Source - die Touchflaeche ist zu
  // empfindlich (reagiert auf Beruehrung in Hosentasche, beim Ablegen etc.)
  // und wuerde das Geraet ungewollt aufwecken.
  // PIN_BTN ist eine Arduino-Pin-Nummer, fuer die nrf_gpio_*-APIs brauchen
  // wir die nRF-Pin-Nummer aus g_ADigitalPinMap.
  uint32_t nrf_btn = g_ADigitalPinMap[PIN_BTN];
  nrf_gpio_cfg_sense_input(nrf_btn, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);

  // Peripherie-Rail aus - spart Strom waehrend des Schlafs.
  digitalWrite(Power_Enable_Pin, LOW);

  // System OFF: tiefster Schlafmodus des nRF52, SoftDevice-konform.
  // Aufwachen via SENSE = HW-Reset, setup() laeuft erneut.
  sd_power_system_off();
  while (true) { __WFE(); }   // unerreichbar
}
