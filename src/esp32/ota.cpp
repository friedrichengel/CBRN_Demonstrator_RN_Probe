#include "esp32/ota.h"
#include "esp32/status_led.h"
#include "config.h"

#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <NimBLEDevice.h>

static constexpr const char* NVS_NS       = "wifi";
static constexpr uint32_t WIFI_TIMEOUT_MS = 30000;
static constexpr uint32_t OTA_WINDOW_MS   = 10UL * 60UL * 1000UL;

static bool  pending = false;
static char  ssidBuf[33] = {0};
static char  pskBuf[65]  = {0};
static bool  credsLoaded = false;

static WebServer server(80);

// ---------------------------------------------------------------- Credentials
static void loadCreds() {
  if (credsLoaded) return;
  Preferences p;
  p.begin(NVS_NS, true);
  p.getString("ssid", ssidBuf, sizeof(ssidBuf));
  p.getString("psk",  pskBuf,  sizeof(pskBuf));
  p.end();
  credsLoaded = true;
}

void otaSetWifi(const char* ssid, const char* psk) {
  loadCreds();
  Preferences p;
  p.begin(NVS_NS, false);
  if (ssid) { snprintf(ssidBuf, sizeof(ssidBuf), "%s", ssid); p.putString("ssid", ssidBuf); }
  if (psk)  { snprintf(pskBuf,  sizeof(pskBuf),  "%s", psk);  p.putString("psk",  pskBuf);  }
  p.end();
}

const char* otaGetSsid() { loadCreds(); return ssidBuf; }

void otaRequest() { pending = true; }
bool otaPending() { return pending; }

// ---------------------------------------------------------------- Web-Upload
static const char UPLOAD_PAGE[] PROGMEM =
  "<!doctype html><html lang=de><meta charset=utf-8>"
  "<meta name=viewport content='width=device-width,initial-scale=1'>"
  "<title>Strahler Firmware-Update</title>"
  "<style>body{font-family:system-ui,sans-serif;margin:2rem;max-width:32rem}"
  "input,button{font-size:1rem;padding:.5rem;margin:.3rem 0}</style>"
  "<h2>Firmware-Update</h2><p>Datei <code>firmware.bin</code> auswaehlen "
  "und hochladen. Das Geraet startet danach neu.</p>"
  "<form method=POST action=/update enctype=multipart/form-data>"
  "<input type=file name=fw accept=.bin required><br>"
  "<button type=submit>Hochladen</button></form>";

static void handleUpload() {
  HTTPUpload& up = server.upload();
  if (up.status == UPLOAD_FILE_START) {
    Serial.printf("[OTA] Upload: %s\n", up.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) Update.printError(Serial);
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (Update.write(up.buf, up.currentSize) != up.currentSize) Update.printError(Serial);
  } else if (up.status == UPLOAD_FILE_END) {
    if (Update.end(true)) Serial.printf("[OTA] %u Byte geschrieben\n", (unsigned)up.totalSize);
    else                  Update.printError(Serial);
  }
}

// ---------------------------------------------------------------- Fenster
void otaRun() {
  loadCreds();
  if (ssidBuf[0] == '\0') {
    Serial.println("[OTA] keine SSID gesetzt (GATT 0x2B07/0x2B08) - abgebrochen");
    pending = false;
    return;
  }

  ledSetMode(LED_OTA);
  Serial.println("[OTA] BLE wird abgeschaltet");
  NimBLEDevice::deinit(true);
  delay(100);

  char host[20];
  snprintf(host, sizeof(host), "strahler-%03d", cfg.sender_nr);

  Serial.printf("[OTA] WLAN '%s' ...\n", ssidBuf);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(host);
  WiFi.begin(ssidBuf, pskBuf);

  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < WIFI_TIMEOUT_MS) {
    ledTask();
    delay(20);
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[OTA] WLAN fehlgeschlagen - Neustart");
    delay(200);
    ESP.restart();
  }

  Serial.printf("[OTA] verbunden, IP %s, Host %s.local\n",
                WiFi.localIP().toString().c_str(), host);

  ArduinoOTA.setHostname(host);
  ArduinoOTA.onStart ([]() { Serial.println("[OTA] espota Start"); });
  ArduinoOTA.onEnd   ([]() { Serial.println("[OTA] espota fertig"); });
  ArduinoOTA.onError ([](ota_error_t e) { Serial.printf("[OTA] Fehler %u\n", (unsigned)e); });
  ArduinoOTA.begin();

  MDNS.begin(host);
  MDNS.addService("http", "tcp", 80);

  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", UPLOAD_PAGE);
  });
  server.on("/update", HTTP_POST,
            []() {
              server.send(200, "text/plain",
                          Update.hasError() ? "Update fehlgeschlagen" : "OK - Geraet startet neu");
              delay(500);
              ESP.restart();
            },
            handleUpload);
  server.begin();

  Serial.printf("[OTA] Fenster offen fuer %lu min\n", (unsigned long)(OTA_WINDOW_MS / 60000));
  t0 = millis();
  while (millis() - t0 < OTA_WINDOW_MS) {
    ArduinoOTA.handle();
    server.handleClient();
    ledTask();
    delay(5);
  }

  Serial.println("[OTA] Fenster abgelaufen - Neustart");
  server.stop();
  WiFi.disconnect(true);
  delay(200);
  ESP.restart();
}
