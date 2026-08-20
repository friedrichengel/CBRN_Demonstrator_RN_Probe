#pragma once
#include <Arduino.h>

// ================================================================
// Board-Definition: Seeed XIAO ESP32S3 im Handgehaeuse
// (headless - keine Anzeige, Bedienung per Handy-App via BLE)
//
// Pin-Namen laut Arduino-Variante XIAO_ESP32S3:
//   D0 = GPIO1 = A0, D1 = GPIO2, D2 = GPIO3 (Strapping - meiden),
//   D3 = GPIO4, D8..D10 = GPIO7..9 (SPI), LED_BUILTIN = GPIO21
// ================================================================

// Externe Betriebs-LED (Panel-LED an der Gehaeusewand), gegen GND
// geschaltet, mit Vorwiderstand. Aktiv HIGH.
#define LED_STATUS_PIN        D1        // GPIO2
#define LED_STATUS_ACTIVE_HIGH 1

// Onboard-LED des XIAO (GPIO21, aktiv LOW) - nur fuer Debug.
#define LED_ONBOARD_PIN       LED_BUILTIN
#define LED_ONBOARD_ACTIVE_HIGH 0

// Akku-Messung: Spannungsteiler 2 x 200 kOhm von BAT+ nach GND,
// Mittelabgriff auf A0. Teilerfaktor also 2,0.
#define BAT_ADC_PIN           A0        // GPIO1
#define BAT_ADC_DIVIDER       2.0f
#define BAT_ADC_SAMPLES       16

// Grenzen fuer die Prozentschaetzung (1S LiPo, linear).
#define BAT_EMPTY_MV          3300
#define BAT_FULL_MV           4200

// Ab hier signalisiert die LED "Akku schwach".
#define BAT_LOW_PERCENT       15
