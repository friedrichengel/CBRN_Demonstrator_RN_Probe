#pragma once
#include <Arduino.h>

// ================================================================
// Firmware-Update ueber WLAN.
//
// Ablauf: Die Handy-App schreibt 1 auf die GATT-Charakteristik
// 0x2B06. Damit ist das Update-Fenster angefordert; die Hauptschleife
// ruft otaRun() auf, sobald sie den Wunsch sieht.
//
// In otaRun() wird BLE komplett abgeschaltet, bevor WLAN startet.
// Grund: Espressif warnt ausdruecklich davor, eine ACL-Verbindung auf
// Coded PHY parallel zu WLAN zu betreiben - die Koexistenz bricht die
// WLAN-Performance massiv ein. Fuer ein Update ist das inakzeptabel.
//
// Im Fenster laufen parallel:
//   - ArduinoOTA (espota)  -> Upload direkt aus PlatformIO/Arduino IDE
//   - Webseite auf Port 80 -> .bin per Browser hochladen, ohne Toolchain
// Nach OTA_WINDOW_MS ohne Upload startet das Geraet normal neu.
// ================================================================

// Wunsch nach einem Update-Fenster setzen (aus dem BLE-Callback).
void otaRequest();

// true, wenn ein Update-Fenster angefordert wurde.
bool otaPending();

// WLAN-Zugangsdaten in NVS ablegen. nullptr = Feld unveraendert lassen.
void otaSetWifi(const char* ssid, const char* psk);

// Aktuell gespeicherte SSID (leerer String, wenn nichts gesetzt).
const char* otaGetSsid();

// Update-Fenster ausfuehren. Kehrt nur zurueck, wenn kein WLAN
// konfiguriert ist; sonst endet die Funktion mit einem Neustart.
void otaRun();
