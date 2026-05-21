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
static BLECharacteristic chrDosis (0x2B02);   // uint16 0.01 µSv/h, Big Endian
static BLECharacteristic chrAktiv (0x2B03);   // uint8  0=aus, 1=an
static BLECharacteristic chrRange (0x2B04);   // uint8  0=normal, 1=long range

// Eigenes Advertising-Set - Bluefruit's Wrapper kann primary_phy/secondary_phy
// nicht setzen, deshalb gehen wir direkt an die SoftDevice-API.
static uint8_t adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;
static uint8_t adv_data_buf[BLE_GAP_ADV_SET_DATA_SIZE_EXTENDED_MAX_SUPPORTED];
static size_t  adv_data_len = 0;

// Baut die AD-Struktur (Flags, Manufacturer-Data, Complete Local Name)
// in adv_data_buf. Layout der Manufacturer-Data siehe README.
static void buildAdvData() {
  uint8_t mfg[9];
  mfg[0] = 0xFF; mfg[1] = 0xFF;
  mfg[2] = 0x01;
  mfg[3] = cfg.sender_nr;
  mfg[4] = cfg.nuklid;
  mfg[5] = (cfg.max_dosis_100 >> 8) & 0xFF;
  mfg[6] =  cfg.max_dosis_100       & 0xFF;
  mfg[7] = (uint8_t)(int8_t)TX_POWER;
  mfg[8] = mfg[2] ^ mfg[3] ^ mfg[4] ^ mfg[5] ^ mfg[6] ^ mfg[7];

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
  err = sd_ble_gap_adv_start(adv_handle, BLE_CONN_CFG_TAG_DEFAULT);
  if (err != NRF_SUCCESS) {
    Serial.printf("[BLE] adv_start err=0x%lx\n", (unsigned long)err);
    return;
  }
  Serial.printf("[BLE%s] Q%03d | %s | %.2f uSv/h\n",
    cfg.long_range ? "-LR" : "",
    cfg.sender_nr, nuklid_name[cfg.nuklid],
    cfg.max_dosis_100 / 100.0f);
}

void stopAdvertising() {
  if (adv_handle != BLE_GAP_ADV_SET_HANDLE_NOT_SET) {
    sd_ble_gap_adv_stop(adv_handle);
  }
}

// Wird vom Bluefruit-Stack bei jedem GATT-Write aufgerufen.
// Aktualisiert cfg, schreibt sie persistent und re-startet Advertising.
static void onGATTWrite(uint16_t /*hdl*/, BLECharacteristic* chr,
                        uint8_t* data, uint16_t len) {
  if      (chr == &chrSender && len >= 1) cfg.sender_nr     = constrain((int)data[0], 1, 254);
  else if (chr == &chrNuklid && len >= 1) cfg.nuklid        = constrain((int)data[0], 1, (int)NUKLID_COUNT);
  else if (chr == &chrDosis  && len >= 2) cfg.max_dosis_100 = ((uint16_t)data[0] << 8) | data[1];
  else if (chr == &chrAktiv  && len >= 1) cfg.aktiv         = data[0] != 0;
  else if (chr == &chrRange  && len >= 1) cfg.long_range    = data[0] != 0;

  saveConfig();
  if (cfg.aktiv) startAdvertising();
  else           stopAdvertising();
  updateDisplay();
}

static void setupGATT() {
  svcCfg.begin();
  auto sc = [](BLECharacteristic& c, uint8_t len) {
    c.setProperties(CHR_PROPS_READ | CHR_PROPS_WRITE);
    c.setPermission(SECMODE_OPEN, SECMODE_OPEN);
    c.setFixedLen(len);
    c.setWriteCallback(onGATTWrite);
    c.begin();
  };
  sc(chrSender, 1); chrSender.write8 (cfg.sender_nr);
  sc(chrNuklid, 1); chrNuklid.write8 (cfg.nuklid);
  sc(chrDosis,  2); chrDosis .write16(cfg.max_dosis_100);
  sc(chrAktiv,  1); chrAktiv .write8 (cfg.aktiv      ? 1 : 0);
  sc(chrRange,  1); chrRange .write8 (cfg.long_range ? 1 : 0);
}

void bleBegin() {
  Bluefruit.begin();
  Bluefruit.setTxPower(TX_POWER);

  char name[16];
  snprintf(name, sizeof(name), "Strahler-%03d", cfg.sender_nr);
  Bluefruit.setName(name);

  setupGATT();
  if (cfg.aktiv) startAdvertising();
}
