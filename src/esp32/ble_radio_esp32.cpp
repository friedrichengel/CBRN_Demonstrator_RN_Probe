#include "esp32/ble_radio_esp.h"
#include "esp32/status_led.h"
#include "esp32/ota.h"
#include "config.h"
#include <NimBLEDevice.h>

#if !defined(CONFIG_BT_NIMBLE_EXT_ADV) || CONFIG_BT_NIMBLE_EXT_ADV == 0
#error "Extended Advertising fehlt: -DCONFIG_BT_NIMBLE_EXT_ADV=1 in platformio.ini setzen"
#endif

// ================================================================
// Advertising-Instanz 0 wird je nach cfg.long_range als
// Extended/Coded-PHY-Set oder als Legacy-Set (1M PHY) konfiguriert.
// Beide Varianten sind connectable, damit die Handy-App den
// GATT-Service in jedem Modus erreicht.
// ================================================================
static constexpr uint8_t ADV_INSTANCE = 0;
static constexpr uint32_t ADV_INTERVAL_UNITS = 160;  // 160 * 0,625 ms = 100 ms

static NimBLEServer* srv = nullptr;
static NimBLECharacteristic* chrSender = nullptr;   // 0x2B00 uint8
static NimBLECharacteristic* chrNuklid = nullptr;   // 0x2B01 uint8
static NimBLECharacteristic* chrDosis  = nullptr;   // 0x2B02 uint16 LE
static NimBLECharacteristic* chrAktiv  = nullptr;   // 0x2B03 bool
static NimBLECharacteristic* chrRange  = nullptr;   // 0x2B04 bool
static NimBLECharacteristic* chrUnit   = nullptr;   // 0x2B05 uint8
static NimBLECharacteristic* chrOta    = nullptr;   // 0x2B06 uint8
static NimBLECharacteristic* chrSsid   = nullptr;   // 0x2B07 string
static NimBLECharacteristic* chrPsk    = nullptr;   // 0x2B08 string (write only)
static NimBLECharacteristic* chrBatt   = nullptr;   // 0x2A19 uint8

static bool connected = false;

// ---------------------------------------------------------------- Advertising
static void deviceName(char* buf, size_t len) {
  snprintf(buf, len, "Strahler-%03d", cfg.sender_nr);
}

// Manufacturer-Data v0x02 - Layout identisch zur T-Echo-Firmware:
//   [0..1] 0xFFFF Company-ID (Test/Entwicklung)
//   [2]    Protokollversion 0x02
//   [3]    Sender-Nr
//   [4]    Nuklid-ID
//   [5..6] Dosis, Big Endian
//   [7]    Einheiten-Index
//   [8]    TX-Power (int8)
//   [9]    XOR-Pruefbyte ueber [2..8]
static void buildMfgData(uint8_t mfg[10]) {
  mfg[0] = 0xFF; mfg[1] = 0xFF;
  mfg[2] = 0x02;
  mfg[3] = cfg.sender_nr;
  mfg[4] = cfg.nuklid;
  mfg[5] = (uint8_t)((cfg.max_dosis_100 >> 8) & 0xFF);
  mfg[6] = (uint8_t)( cfg.max_dosis_100       & 0xFF);
  mfg[7] = cfg.unit_idx;
  mfg[8] = (uint8_t)(int8_t)TX_POWER;
  mfg[9] = mfg[2] ^ mfg[3] ^ mfg[4] ^ mfg[5] ^ mfg[6] ^ mfg[7] ^ mfg[8];
}

void startAdvertising() {
  NimBLEExtAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->stop(ADV_INSTANCE);

  uint8_t mfg[10];
  buildMfgData(mfg);

  char name[16];
  deviceName(name, sizeof(name));

  if (cfg.long_range) {
    // BLE 5 Extended Advertising, Coded PHY (S=8) auf primaeren und
    // sekundaeren Kanaelen. Connectable-Extended darf nicht scannable sein.
    NimBLEExtAdvertisement ext(BLE_HCI_LE_PHY_CODED, BLE_HCI_LE_PHY_CODED);
    ext.setConnectable(true);
    ext.setScannable(false);
    ext.setLegacyAdvertising(false);
    ext.setMinInterval(ADV_INTERVAL_UNITS);
    ext.setMaxInterval(ADV_INTERVAL_UNITS);
    ext.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);
    ext.setManufacturerData(mfg, sizeof(mfg));
    ext.setName(name);
    adv->setInstanceData(ADV_INSTANCE, ext);
  } else {
    // Legacy-Advertising auf 1M PHY - kompatibel mit BLE-4.x-Scannern.
    // Payload: Flags 3 + Mfg 12 + Name 14 = 29 Byte, passt in die 31.
    NimBLEExtAdvertisement leg(BLE_HCI_LE_PHY_1M, BLE_HCI_LE_PHY_1M);
    leg.setLegacyAdvertising(true);
    leg.setConnectable(true);
    leg.setScannable(true);
    leg.setMinInterval(ADV_INTERVAL_UNITS);
    leg.setMaxInterval(ADV_INTERVAL_UNITS);
    leg.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);
    leg.setManufacturerData(mfg, sizeof(mfg));
    leg.setName(name);
    adv->setInstanceData(ADV_INSTANCE, leg);
  }

  if (!adv->start(ADV_INSTANCE)) {
    Serial.println("[BLE] adv start fehlgeschlagen");
    return;
  }

  ledSetMode(LED_ADV_ACTIVE);
  Serial.printf("[BLE%s] Q%03d | %s | %u %s\n",
                cfg.long_range ? "-LR" : "",
                cfg.sender_nr, nuklidName(cfg.nuklid),
                (unsigned)cfg.max_dosis_100, unit_table[cfg.unit_idx].name);
}

void stopAdvertising() {
  NimBLEDevice::getAdvertising()->stop(ADV_INSTANCE);
  ledSetMode(LED_ADV_OFF);
  Serial.println("[BLE] Advertising aus");
}

bool bleConnected() { return connected; }

// ---------------------------------------------------------------- GATT
void bleSyncChars() {
  chrSender->setValue<uint8_t>(cfg.sender_nr);
  chrNuklid->setValue<uint8_t>(cfg.nuklid);
  chrDosis ->setValue<uint16_t>(cfg.max_dosis_100);   // GATT: Little Endian
  chrAktiv ->setValue<uint8_t>(cfg.aktiv      ? 1 : 0);
  chrRange ->setValue<uint8_t>(cfg.long_range ? 1 : 0);
  chrUnit  ->setValue<uint8_t>(cfg.unit_idx);
}

void bleSetBatteryLevel(uint8_t percent) {
  if (!chrBatt) return;
  chrBatt->setValue<uint8_t>(percent);
  if (connected) chrBatt->notify();
}

class ServerCb : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer*, NimBLEConnInfo& info) override {
    connected = true;
    Serial.printf("[BLE] connected: %s\n", info.getAddress().toString().c_str());
  }
  void onDisconnect(NimBLEServer*, NimBLEConnInfo&, int reason) override {
    connected = false;
    Serial.printf("[BLE] disconnected, reason=0x%02X\n", reason);
    // Ext-Adv startet nach einem Connect nicht von selbst wieder.
    if (cfg.aktiv) startAdvertising();
  }
};

class ChrCb : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* c, NimBLEConnInfo&) override {
    const NimBLEAttValue v = c->getValue();
    const uint8_t* d = v.data();
    size_t len = v.size();
    if (len == 0) return;

    bool restartAdv = true;

    if (c == chrSender) {
      cfg.sender_nr = (uint8_t)constrain((int)d[0], 1, 254);
      char n[16]; deviceName(n, sizeof(n));
      NimBLEDevice::setDeviceName(n);
    } else if (c == chrNuklid) {
      if (nuklidIndex(d[0]) >= 0) cfg.nuklid = d[0];
      else Serial.printf("[BLE-WR] unbekannte Nuklid-ID %u verworfen\n", (unsigned)d[0]);
      chrNuklid->setValue<uint8_t>(cfg.nuklid);
    } else if (c == chrDosis) {
      // 1-Byte-Writes akzeptieren, damit der UINT8-Modus der App auch geht.
      uint16_t val = (uint16_t)d[0];
      if (len >= 2) val |= ((uint16_t)d[1]) << 8;
      cfg.max_dosis_100 = val;
    } else if (c == chrAktiv) {
      cfg.aktiv = d[0] != 0;
    } else if (c == chrRange) {
      cfg.long_range = d[0] != 0;
    } else if (c == chrUnit) {
      cfg.unit_idx = (uint8_t)constrain((int)d[0], 0, (int)UNIT_COUNT - 1);
    } else if (c == chrOta) {
      if (d[0] != 0) {
        Serial.println("[OTA] per BLE angefordert");
        otaRequest();
      }
      return;                       // keine Config, kein Adv-Restart
    } else if (c == chrSsid) {
      otaSetWifi(std::string((const char*)d, len).c_str(), nullptr);
      Serial.println("[WLAN] SSID gespeichert");
      return;
    } else if (c == chrPsk) {
      otaSetWifi(nullptr, std::string((const char*)d, len).c_str());
      Serial.println("[WLAN] Passwort gespeichert");
      return;
    } else {
      restartAdv = false;
    }

    saveConfig();
    bleSyncChars();

    // Waehrend einer aktiven Verbindung kein Adv-Neustart - das uebernimmt
    // onDisconnect() mit dem dann aktuellen cfg.
    if (restartAdv && !connected) {
      if (cfg.aktiv) startAdvertising();
      else           stopAdvertising();
    }
    if (!cfg.aktiv) ledSetMode(LED_ADV_OFF);
  }
};

static ServerCb serverCb;
static ChrCb    chrCb;

// Hilfsfunktion: Charakteristik mit Beschreibung (0x2901) und
// Presentation Format (0x2904) anlegen - damit die Handy-App
// (z.B. nRF Connect) den richtigen Eingabetyp anbietet.
static NimBLECharacteristic* makeChr(NimBLEService* svc, uint16_t uuid,
                                    uint32_t props, uint8_t fmt,
                                    const char* desc) {
  NimBLECharacteristic* c = svc->createCharacteristic(NimBLEUUID(uuid), props);
  NimBLEDescriptor* d = c->createDescriptor(NimBLEUUID((uint16_t)0x2901),
                                           NIMBLE_PROPERTY::READ, 24);
  d->setValue(desc);
  NimBLE2904* pf = c->create2904();
  pf->setFormat(fmt);
  pf->setExponent(0);
  pf->setUnit(0x2700);            // unitless
  pf->setNamespace(1);
  pf->setDescription(0);
  c->setCallbacks(&chrCb);
  return c;
}

void bleBegin() {
  char name[16];
  deviceName(name, sizeof(name));

  NimBLEDevice::init(name);
  NimBLEDevice::setPower(TX_POWER);
  // Bevorzugte PHY fuer bestehende Verbindungen: Coded, wenn LR aktiv ist.
  if (cfg.long_range) {
    NimBLEDevice::setDefaultPhy(BLE_GAP_LE_PHY_CODED_MASK, BLE_GAP_LE_PHY_CODED_MASK);
  }

  srv = NimBLEDevice::createServer();
  srv->setCallbacks(&serverCb, false);

  NimBLEService* svc = srv->createService(NimBLEUUID((uint16_t)0x1900));
  const uint32_t RW = NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE;

  chrSender = makeChr(svc, 0x2B00, RW, NimBLE2904::FORMAT_UINT8,   "Sender-Nr");
  chrNuklid = makeChr(svc, 0x2B01, RW, NimBLE2904::FORMAT_UINT8,   "Nuklid-ID");
  chrDosis  = makeChr(svc, 0x2B02, RW, NimBLE2904::FORMAT_UINT16,  "Max.Dosis");
  chrAktiv  = makeChr(svc, 0x2B03, RW, NimBLE2904::FORMAT_BOOLEAN, "Aktiv");
  chrRange  = makeChr(svc, 0x2B04, RW, NimBLE2904::FORMAT_BOOLEAN, "Long Range");
  chrUnit   = makeChr(svc, 0x2B05, RW, NimBLE2904::FORMAT_UINT8,   "Einheit-Idx");
  chrOta    = makeChr(svc, 0x2B06, RW, NimBLE2904::FORMAT_UINT8,   "OTA starten");
  chrSsid   = makeChr(svc, 0x2B07, RW, NimBLE2904::FORMAT_UTF8,    "WLAN-SSID");
  chrPsk    = makeChr(svc, 0x2B08, NIMBLE_PROPERTY::WRITE,
                      NimBLE2904::FORMAT_UTF8, "WLAN-Passwort");
  svc->start();

  // Standard Battery Service - ersetzt die Akkuanzeige des Displays.
  NimBLEService* bat = srv->createService(NimBLEUUID((uint16_t)0x180F));
  chrBatt = bat->createCharacteristic(NimBLEUUID((uint16_t)0x2A19),
                                      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  chrBatt->setValue<uint8_t>(0);
  bat->start();

  bleSyncChars();
  chrSsid->setValue(otaGetSsid());

  if (cfg.aktiv) startAdvertising();
  else           ledSetMode(LED_ADV_OFF);
}
