#include "ble_radio.h"
#include "config.h"
#include "display_ui.h"
#include <ble.h>
#include <ble_gap.h>

// GATT-Service "Strahler-Konfig" (custom 16-Bit UUID 0x1900)
// mit vier R/W-Charakteristika fuer die Felder aus `cfg`.
static BLEService        svcCfg   (0x1900);
static BLECharacteristic chrSender(0x2B00);   // uint8  Sender-Nr (1..254)
static BLECharacteristic chrNuklid(0x2B01);   // uint8  Nuklid-ID (1..NUKLID_COUNT)
static BLECharacteristic chrDosis (0x2B02);   // uint16 0.01 * Einheit, Big Endian
static BLECharacteristic chrAktiv (0x2B03);   // uint8  0=aus, 1=an
static BLECharacteristic chrRange (0x2B04);   // uint8  0=normal, 1=long range
static BLECharacteristic chrUnit  (0x2B05);   // uint8  Index in unit_table (0..UNIT_COUNT-1)

// Eigenes Advertising-Set - Bluefruit's Wrapper kann primary_phy/secondary_phy
// nicht setzen, deshalb gehen wir direkt an die SoftDevice-API.
static uint8_t adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;
static uint8_t adv_data_buf[BLE_GAP_ADV_SET_DATA_SIZE_EXTENDED_MAX_SUPPORTED];
static size_t  adv_data_len = 0;

// Baut die AD-Struktur (Flags, Manufacturer-Data, Complete Local Name)
// in adv_data_buf. Layout der Manufacturer-Data siehe README.
//
// Protokoll-Version 0x02: 10 Bytes, mit unit_idx an Byte 7 (vorher 0x01: 9 Bytes,
// ohne Einheit, dose-Interpretation als 0.01 * uSv/h fixed).
static void buildAdvData() {
  uint8_t mfg[10];
  mfg[0] = 0xFF; mfg[1] = 0xFF;
  mfg[2] = 0x02;                                  // Protokoll-Version
  mfg[3] = cfg.sender_nr;
  mfg[4] = cfg.nuklid;
  mfg[5] = (cfg.max_dosis_100 >> 8) & 0xFF;       // Dosis MSB
  mfg[6] =  cfg.max_dosis_100       & 0xFF;       // Dosis LSB
  mfg[7] = cfg.unit_idx;                          // Einheits-Index in unit_table
  mfg[8] = (uint8_t)(int8_t)TX_POWER;
  mfg[9] = mfg[2] ^ mfg[3] ^ mfg[4] ^ mfg[5] ^ mfg[6] ^ mfg[7] ^ mfg[8];

  char name[16];
  snprintf(name, sizeof(name), "Strahler-%03d", cfg.sender_nr);
  uint8_t nameLen = (uint8_t)strlen(name);

  uint8_t* p = adv_data_buf;

  // Flags
  *p++ = 2;
  *p++ = BLE_GAP_AD_TYPE_FLAGS;
  *p++ = BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED | BLE_GAP_ADV_FLAG_LE_GENERAL_DISC_MODE;

  // Manufacturer Specific Data
  *p++ = (uint8_t)(sizeof(mfg) + 1);
  *p++ = BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA;
  memcpy(p, mfg, sizeof(mfg)); p += sizeof(mfg);

  // Complete Local Name
  *p++ = (uint8_t)(nameLen + 1);
  *p++ = BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME;
  memcpy(p, name, nameLen); p += nameLen;

  adv_data_len = (size_t)(p - adv_data_buf);
}

void startAdvertising() {
  // Laufendes Advertising stoppen, sonst lehnt die SD set_configure ab.
  if (adv_handle != BLE_GAP_ADV_SET_HANDLE_NOT_SET) {
    sd_ble_gap_adv_stop(adv_handle);
  }

  buildAdvData();

  ble_gap_adv_data_t adv_data = {};
  adv_data.adv_data.p_data      = adv_data_buf;
  adv_data.adv_data.len         = (uint16_t)adv_data_len;
  adv_data.scan_rsp_data.p_data = nullptr;
  adv_data.scan_rsp_data.len    = 0;

  ble_gap_adv_params_t adv_params = {};
  adv_params.duration      = 0;             // unbegrenzt
  adv_params.interval      = 0x00A0;        // 160 * 0.625 ms = 100 ms
  adv_params.filter_policy = BLE_GAP_ADV_FP_ANY;
  adv_params.p_peer_addr   = nullptr;

  if (cfg.long_range) {
    // BLE 5 Extended Advertising auf Coded PHY (S=8).
    // Connectable, damit das GATT-Interface auch im LR-Modus erreichbar bleibt -
    // der Client muss dafuer ebenfalls Coded PHY scannen.
    adv_params.properties.type     = BLE_GAP_ADV_TYPE_EXTENDED_CONNECTABLE_NONSCANNABLE_UNDIRECTED;
    adv_params.primary_phy         = BLE_GAP_PHY_CODED;
    adv_params.secondary_phy       = BLE_GAP_PHY_CODED;
  } else {
    // Klassisches Legacy-Advertising auf 1 Mbps - BLE 4.x kompatibel.
    adv_params.properties.type     = BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED;
    adv_params.primary_phy         = BLE_GAP_PHY_1MBPS;
  }

  uint32_t err = sd_ble_gap_adv_set_configure(&adv_handle, &adv_data, &adv_params);
  if (err != NRF_SUCCESS) {
    Serial.printf("[BLE] adv_set_configure err=0x%lx\n", (unsigned long)err);
    return;
  }
  // CONN_CFG_PERIPHERAL (= 1) ist der Tag mit dem Bluefruit::begin() die
  // SoftDevice konfiguriert. BLE_CONN_CFG_TAG_DEFAULT (= 0) ist NICHT
  // konfiguriert und wuerde NRF_ERROR_NOT_FOUND zurueckliefern.
  err = sd_ble_gap_adv_start(adv_handle, CONN_CFG_PERIPHERAL);
  if (err != NRF_SUCCESS) {
    Serial.printf("[BLE] adv_start err=0x%lx\n", (unsigned long)err);
    return;
  }
  Serial.printf("[BLE%s] Q%03d | %s | %u %s\n",
    cfg.long_range ? "-LR" : "",
    cfg.sender_nr, nuklid_name[cfg.nuklid],
    (unsigned)cfg.max_dosis_100, unit_table[cfg.unit_idx].name);
}

void stopAdvertising() {
  if (adv_handle != BLE_GAP_ADV_SET_HANDLE_NOT_SET) {
    sd_ble_gap_adv_stop(adv_handle);
  }
}

void bleSyncChars() {
  // Schreibt die aktuellen cfg-Werte in die SoftDevice-Attribut-Tabelle.
  // Reine RAM-Operation, billig - kann pro Menue-Tick aufgerufen werden.
  chrSender.write8 (cfg.sender_nr);
  chrNuklid.write8 (cfg.nuklid);
  chrDosis .write16(cfg.max_dosis_100);
  chrAktiv .write8 (cfg.aktiv      ? 1 : 0);
  chrRange .write8 (cfg.long_range ? 1 : 0);
  chrUnit  .write8 (cfg.unit_idx);
}

// Wird vom Bluefruit-Stack bei jedem GATT-Write aufgerufen.
// Aktualisiert cfg, schreibt sie persistent und re-startet Advertising.
static void onGATTWrite(uint16_t /*hdl*/, BLECharacteristic* chr,
                        uint8_t* data, uint16_t len) {
  const char* tag = "?";
  if      (chr == &chrSender) tag = "Sender";
  else if (chr == &chrNuklid) tag = "Nuklid";
  else if (chr == &chrDosis ) tag = "Dosis";
  else if (chr == &chrAktiv ) tag = "Aktiv";
  else if (chr == &chrRange ) tag = "Range";
  else if (chr == &chrUnit  ) tag = "Unit";
  Serial.printf("[BLE-WR] %s len=%u", tag, (unsigned)len);
  for (uint16_t i = 0; i < len && i < 8; ++i) Serial.printf(" %02X", data[i]);
  Serial.println();

  bool handled = true;
  if      (chr == &chrSender && len >= 1) cfg.sender_nr     = constrain((int)data[0], 1, 254);
  else if (chr == &chrNuklid && len >= 1) cfg.nuklid        = constrain((int)data[0], 1, (int)NUKLID_COUNT);
  // chrDosis ist Little Endian (GATT-Konvention, passt zu write16/CPF-uint16).
  // Mfg-Data dagegen bleibt Big Endian - das ist ein anderes Protokoll fuer Empfaenger.
  // 1-Byte-Writes werden akzeptiert (LSB), damit nRF Connect's UINT8-Modus
  // (Werte 0..255) und der UINT16-Modus beide funktionieren.
  else if (chr == &chrDosis  && len >= 1) {
    uint16_t v = (uint16_t)data[0];
    if (len >= 2) v |= ((uint16_t)data[1]) << 8;
    cfg.max_dosis_100 = v;
  }
  else if (chr == &chrAktiv  && len >= 1) cfg.aktiv         = data[0] != 0;
  else if (chr == &chrRange  && len >= 1) cfg.long_range    = data[0] != 0;
  else if (chr == &chrUnit   && len >= 1) cfg.unit_idx      = (uint8_t)constrain((int)data[0], 0, (int)UNIT_COUNT - 1);
  else { handled = false; Serial.println("[BLE-WR] ignoriert (len zu klein)"); }

  if (!handled) return;
  saveConfig();
  // Waehrend einer aktiven Verbindung kein Adv-(Re)Start: der SoftDevice hat
  // nur einen Peripheral-Slot und liefert sonst NRF_ERROR_INVALID_STATE (0x12).
  // Nach Disconnect uebernimmt onBleDisconnect() den Neustart mit aktuellem cfg.
  if (Bluefruit.connected() == 0) {
    if (cfg.aktiv) startAdvertising();
    else           stopAdvertising();
  }
  updateDisplay();
}

static void onBleConnect(uint16_t conn_hdl) {
  BLEConnection* c = Bluefruit.Connection(conn_hdl);
  char peer[20] = "?";
  if (c) c->getPeerName(peer, sizeof(peer));
  Serial.printf("[BLE] connected hdl=%u peer=%s\n", conn_hdl, peer);
}

static void onBleDisconnect(uint16_t conn_hdl, uint8_t reason) {
  Serial.printf("[BLE] disconnected hdl=%u reason=0x%02X\n", conn_hdl, reason);
  // Raw-Adv neu starten, wenn aktiv (Bluefruit weiss von unserem Adv-Handle nichts)
  if (cfg.aktiv) startAdvertising();
  // Nach (potenziell vielen) Partial-Refreshes waehrend der Session jetzt
  // einmal voll durchblitzen, um Ghosting auf dem E-Paper zu loeschen.
  forceFullRefresh();
  updateDisplay();
}

// GATT Presentation Format - Typcodes aus BT-SIG-Spec
// (siehe ble_gatts_char_pf_t.format)
static constexpr uint8_t GATT_FMT_BOOL  = 0x01;
static constexpr uint8_t GATT_FMT_UINT8 = 0x04;
static constexpr uint8_t GATT_FMT_UINT16= 0x06;

static void setupGATT() {
  svcCfg.begin();
  // sc() = standard Setup. fmt/exp/desc liefern die GATT-Hinweise, anhand
  // derer nRF Connect & Co. den richtigen Eingabetyp anbieten (uint8 / uint16
  // / boolean) statt nur "Byte Array".
  auto sc = [](BLECharacteristic& c, uint8_t len, uint8_t fmt, int8_t exp,
               const char* desc) {
    c.setProperties(CHR_PROPS_READ | CHR_PROPS_WRITE);
    c.setPermission(SECMODE_OPEN, SECMODE_OPEN);
    // setMaxLen statt setFixedLen: variable Laenge erlaubt, damit nRF Connect
    // & Co. nicht an INVALID_ATTRIBUTE_LENGTH scheitern, wenn z.B. nur 1 Byte
    // statt der erwarteten 2 geschickt wird. Plausibilitaet macht onGATTWrite.
    c.setMaxLen(len);
    c.setUserDescriptor(desc);
    c.setPresentationFormatDescriptor(fmt, exp, UUID16_UNIT_UNITLESS);
    c.setWriteCallback(onGATTWrite);
    c.begin();
  };
  sc(chrSender, 1, GATT_FMT_UINT8 , 0 , "Sender-Nr");   chrSender.write8 (cfg.sender_nr);
  sc(chrNuklid, 1, GATT_FMT_UINT8 , 0 , "Nuklid-ID");   chrNuklid.write8 (cfg.nuklid);
  // exponent=0 → die Dosis ist eine reine Ganzzahl, nRF Connect zeigt sie 1:1
  // (z.B. 100 -> "100"). UNIT bleibt unitless, weil die tatsaechliche Einheit
  // dynamisch ueber chrUnit gewaehlt wird.
  sc(chrDosis,  2, GATT_FMT_UINT16, 0,  "Max.Dosis");   chrDosis .write16(cfg.max_dosis_100);
  sc(chrAktiv,  1, GATT_FMT_BOOL  , 0 , "Aktiv");       chrAktiv .write8 (cfg.aktiv      ? 1 : 0);
  sc(chrRange,  1, GATT_FMT_BOOL  , 0 , "Long Range");  chrRange .write8 (cfg.long_range ? 1 : 0);
  sc(chrUnit,   1, GATT_FMT_UINT8 , 0 , "Einheit-Idx"); chrUnit  .write8 (cfg.unit_idx);
}

void bleBegin() {
  // Coded PHY (125 kbps) braucht ~8x laenger pro Paket als 1 Mbps. Damit der
  // SoftDevice einen Connection-Roundtrip auf der ausgewaehlten Adv-PHY ueberhaupt
  // einplanen kann, muss event_length groesser sein als der Bluefruit-Default
  // (BLE_GAP_EVENT_LENGTH_DEFAULT = 3 Units = 3.75 ms). Sonst lehnt
  // sd_ble_gap_adv_start im LR-Modus mit NRF_ERROR_RESOURCES (0x13) ab.
  // 7 Units = 8.75 ms reichen fuer Coded PHY S=8 mit Margin.
  Bluefruit.configPrphConn(BLE_GATT_ATT_MTU_DEFAULT, /*event_len=*/7,
                           BLE_GATTS_HVN_TX_QUEUE_SIZE_DEFAULT,
                           BLE_GATTC_WRITE_CMD_TX_QUEUE_SIZE_DEFAULT);
  Bluefruit.begin();
  Bluefruit.setTxPower(TX_POWER);

  char name[16];
  snprintf(name, sizeof(name), "Strahler-%03d", cfg.sender_nr);
  Bluefruit.setName(name);

  Bluefruit.Periph.setConnectCallback(onBleConnect);
  Bluefruit.Periph.setDisconnectCallback(onBleDisconnect);

  setupGATT();
  if (cfg.aktiv) startAdvertising();
}
