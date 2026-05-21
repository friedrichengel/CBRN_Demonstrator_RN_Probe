#pragma once
#include <Arduino.h>

// Akku-Messung ueber den Spannungsteiler an Adc_Pin (P0.04).
// T-Echo hat 100k/100k -> 2:1 -> die haelfte von VBat liegt am ADC.

// Einmal in setup() aufrufen (analogReadResolution).
void batteryBegin();

// Roher Spannungswert am Akku in Volt (3.0..4.2 typ. fuer 1S LiPo).
float batteryVolts();

// Lineare Schaetzung 0..100 % aus 3.0 V .. 4.2 V.
int batteryPercent();

// true wenn USB-VBus erkannt wird (Geraet haengt am USB).
// Waehrend USB-Versorgung ist batteryPercent() nicht aussagekraeftig,
// weil dann die Versorgungsspannung am Akku-Pin als 100 % gelesen wird.
bool batteryCharging();
