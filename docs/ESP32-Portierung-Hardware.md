# RN-Strahler auf ESP32 portieren — Hardware & Plan

Stand 20.08.2026 · Zielbild: headless Strahler-Sender, Gehäuse, externe 2,4-GHz-Antenne, Akku mit Hauptschalter, Betriebs-LED außen, Konfiguration über BLE-GATT, Firmware-Update über WLAN.

Ohne Display und ohne Tastenmenü wird das Gerät deutlich einfacher als der T-Echo: kein E-Paper, kein Driver Board, keine Menü-Logik, kein Deep-Sleep-Abschaltkonzept — der Hauptschalter trennt hart.

## 1. Stückliste (pro Gerät)

| Pos | Artikel | MPN | Bestellnr. | Preis | Zweck |
|---|---|---|---|---|---|
| 1 | Seeed **XIAO ESP32S3** | 113991114 | Mouser 713-113991114 · DigiKey 1597-113991114-ND | 6,44 € | ESP32-S3, U.FL, **Antenne liegt bei**, LiPo-Lader 100 mA, 8 MB Flash + 8 MB PSRAM |
| 2 | Pigtail U.FL 90° → SMA-Buchse Bulkhead, 100 mm | Taoglas **CAB.719** | Mouser 960-CAB719 · DigiKey 931-1187-ND | 4,52 € | Antenne nach außen |
| 3 | Antenne 2,4 GHz, **SMA male (Standard, nicht RP!)**, ~3 dBi, IP65 | Taoglas **GW.15.2113** | Mouser 960-GW152113 · DigiKey 2332689 | 6,62 € | Sendeantenne |
| 4 | Gehäuse ABS, 85 × 56 × 26 mm, IP54 | Hammond **1591MBK** | Mouser 546-1591M-BK · DigiKey 284748 | 7,23 € | |
| 5 | **Hauptschalter DPDT On-Off-On** (Mittelstellung = AUS), Lötfahnen, Bohrung 6,35 mm | E-Switch **100DP3T1B1M1QEH** | DigiKey **EG2413-ND / 378882** | ~4 USD | trennt Akku zweipolig |
| 6 | Betriebs-LED, Panel, grün, mit integriertem Widerstand | Dialight **5590202003F** | DigiKey **2430874** | n. b. | Bohrung ebenfalls **6,35 mm** — ein Bohrer für Pos. 5 + 6 |
| 7 | LiPo 3,7 V, **950–1400 mAh** (bewusst klein, siehe Abschnitt 4), PCM | Jauch **LP523450JU+PCM+JST PHR-3 70MM** (950 mAh) oder **LP103048JU+PCM+2 WIRES** (1,43 Ah) | digikey.de **25623211** bzw. **13540168** | n. b. | Versand aus Villingen-Schwenningen, keine Luftfracht-Sperre. Stecker egal — er wird für den Schalter abgeschnitten |
| 8 | 2 × 200 kΩ (1 %) + 100 nF | beliebig | – | < 1 € | Spannungsteiler für die Akkumessung |

**Amazon-Alternativen:** Akku [B0C7L2NM66](https://www.amazon.de/BerryBase-wiederaufladbarer-JST-Stecker-Schutzplatine-Isolationsbeschichtung/dp/B0C7L2NM66) (BerryBase, 2500 mAh, JST-PH-2) · Schalter MTS-202/MTS-203 (ON-OFF bzw. ON-OFF-ON) · vorverdrahtete 5-V-LED (die verbreiteten „12 V"-Typen leuchten an 3,7 V nicht).

**Fallen:** RP-SMA-Varianten (Taoglas GW.22.5151, Siretta `.../RP/...`, Taoglas CAB.774) schrauben sich an, machen aber keinen Innenleiterkontakt. Und die üblichen Mini-Kippschalter „2× EIN/EIN" haben **keine** Aus-Stellung.

## 2. Verdrahtung

```
Akku +  ──┐
          ├── Schalter Pol 1 (Mitte) ──► XIAO BAT+
Akku −  ──┴── Schalter Pol 2 (Mitte) ──► XIAO BAT−

XIAO 3V3 ──► LED-Indikator (+)              [alternativ: LED an D1, siehe unten]
XIAO D1 (GPIO2) ──► LED über 680 Ω          [Firmware-gesteuert, empfohlen]
XIAO A0 (D0/GPIO1) ◄── Mittelabgriff 2×200 kΩ zwischen BAT+ und GND, 100 nF nach GND
XIAO U.FL ──► CAB.719 ──► SMA-Bulkhead in der 56-mm-Stirnseite (Ø 6,5 mm)
```

Pin-Belegung belegt aus der Arduino-Variante `XIAO_ESP32S3`: D0 = GPIO1 = A0, D1 = GPIO2, D2 = GPIO3 (**Strapping-Pin, meiden**), D3 = GPIO4, User-LED on-board = GPIO21.

Wichtige Punkte:

- **Laden funktioniert nur bei Schalter EIN.** Der Lader des XIAO sitzt an den BAT-Pads; wenn der Schalter die Zelle trennt, lädt USB nicht. Das ist meist erwünscht (definierter Auslieferzustand), muss aber im Handbuch stehen.
- **Polarität am Akku vor dem ersten Anschluss messen.** Beim XIAO liegt der **Minuspol näher am USB-Anschluss** — die Zellen mit JST-Stecker sind herstellerabhängig belegt, und verpolt ist das Board sofort tot.
- U.FL senkrecht aufdrücken bis es klickt, nicht schräg — die Buchse hält ~30 Steckzyklen.
- Teiler-Ruhestrom: 4,2 V / 400 kΩ ≈ 10 µA, vernachlässigbar. Am ADC liegen max. 2,1 V, das passt in den Messbereich.

## 3. Bedeutung der Betriebs-LED (Vorschlag)

| Muster | Zustand |
|---|---|
| 1 kurzer Blitz alle 3 s | Gerät an, Advertising aktiv |
| Doppelblitz alle 3 s | Gerät an, Advertising aus (`cfg.aktiv == false`) |
| schnelles Blinken (5 Hz) | WLAN-/OTA-Fenster offen |
| 3 Blitze, dann Pause | Akku < 15 % |
| Dauerlicht | optional als „einfach nur an" konfigurierbar |

Warum pulsen statt Dauerlicht: der Dialight-Indikator zieht an 3,3 V rund 6 mA dauerhaft — ein Blitz alle 3 s liegt im Mittel bei ~0,05 mA. Bei einem Gerät, das wochenlang im Schrank liegt, ist das der Unterschied zwischen einsatzbereit und leer. Ein Blitz alle 3 s beantwortet „ist das Ding an?" genauso eindeutig.

## 4. Energiebudget für einen Übungstag (8 h)

Einsatzprofil: morgens einschalten, abends ausschalten, nachts laden. Damit ist der Verbrauch unkritisch — **der Engpass ist die Ladezeit**, weil der XIAO ESP32S3 mit fest eingestelltem **100 mA** lädt.

Verbrauch, pessimistisch gerechnet:

| Posten | Annahme | 8 h |
|---|---|---|
| ESP32-S3, BLE-Advertising, Modem-Sleep, 240 MHz | 25–40 mA | 200–320 mAh |
| dito mit `setCpuFrequencyMhz(80)` | 15–25 mA | 120–200 mAh |
| Betriebs-LED, gepulst | 0,05 mA | 0,4 mAh |
| Betriebs-LED, Dauerlicht | 6 mA | 48 mAh |

Ladezeit bei 100 mA (grob Kapazität / 80 mA effektiv):

| Zelle | Ladezeit | Reserve für 8 h |
|---|---|---|
| 950 mAh | **~11 h** — passt über Nacht | 3–4 × |
| 1200–1400 mAh | ~14–16 h — knapp, aber bei Übung an aufeinanderfolgenden Tagen noch machbar | 4–5 × |
| 1900 mAh | ~20–22 h — **morgens nicht wieder voll** | 6–8 × |

Deshalb die kleine Zelle: mehr Kapazität bringt keine nutzbare Laufzeit, kostet aber Ladezeit, die du nicht hast. **950–1400 mAh ist der Sweet Spot.**

Konsequenzen für die Firmware: Advertising kann bei 100 ms bleiben (kürzere Reaktionszeit am Empfänger), Light Sleep und sonstige Power-Management-Akrobatik sind nicht nötig, die LED darf auch Dauerlicht sein. Das macht den Port einfacher, nicht komplizierter.

Wenn du trotzdem eine größere Zelle willst: externer Lader mit 500 mA statt der 100 mA des XIAO, z. B. **Adafruit Micro-Lipo USB-C (4410, Mouser 485-4410, 5,12 €)** — dann liegt der USB-C-Anschluss des Laders an der Gehäusewand und der des XIAO bleibt nur fürs Flashen.

Was in jedem Fall vor der Serie gemessen werden sollte: der Ruhestrom im Advertising-Betrieb. Liegt er über 40 mA, ist `setCpuFrequencyMhz(80)` die erste Maßnahme.

**USB-C-Zugang:** fürs Laden und Flashen einen Ausschnitt von ca. 10 × 4 mm in die Gehäusewand fräsen (Feile/Dremel), bündig zur Buchse des XIAO — sonst muss zum Laden jedes Mal der Deckel ab.

## 5. Firmware-Portierung

Was **entfällt** (headless): `display_ui.cpp`, `menu.cpp`, `include/thw_logo.h`, `power.cpp` (Hauptschalter statt System OFF), der Touch-Zweig in `input.cpp`.

Was **bleibt inhaltlich unverändert**: das Funkprotokoll. Manufacturer-Data v0x02 (10 Byte, Sender-Nr / Nuklid / Dosis BE / unit_idx / TX-Power / XOR-Prüfbyte) und der GATT-Service 0x1900 mit 0x2B00–0x2B05 werden 1:1 übernommen — die Sonde als Empfänger merkt vom Hardwarewechsel nichts.

Was **neu geschrieben wird**:

| Modul | Inhalt |
|---|---|
| `ble_radio_esp32.cpp` | NimBLE-Arduino ≥ 2.5.1, `NimBLEExtAdvertisement(BLE_HCI_LE_PHY_CODED, BLE_HCI_LE_PHY_CODED)`, connectable, GATT-Server mit denselben UUIDs. Build-Flags `-DCONFIG_BT_NIMBLE_EXT_ADV=1`, `-DCONFIG_BT_NIMBLE_MAX_EXT_ADV_INSTANCES=1`. Legacy-Modus (`cfg.long_range == false`) über `setLegacyAdvertising(true)` + 1M-PHY. |
| `config_esp32.cpp` | `Adafruit_LittleFS/InternalFS` → `Preferences` (NVS). Struktur `Config` unverändert, damit die Feldsemantik erhalten bleibt. |
| `battery_esp32.cpp` | `analogReadMilliVolts(A0) * 2`, 16-fach gemittelt; zusätzlich BLE **Battery Service 0x180F**, damit der Ladezustand ohne Display sichtbar ist. |
| `status_led.cpp` | Muster aus Abschnitt 3, non-blocking über `millis()`. |
| `ota.cpp` | OTA-Fenster: BLE per `NimBLEDevice::deinit(true)` abschalten → WLAN verbinden → Update → Reboot. Auslöser: neue GATT-Charakteristik 0x2B06 („OTA starten", uint8) — kein zusätzlicher Taster nötig. |
| `platformio.ini` | zweites Env `[env:xiao_s3]` (`platform = espressif32`, `board = seeed_xiao_esp32s3`, `partitions = default_8MB.csv`), T-Echo-Env bleibt baubar über `build_src_filter`. |

**Warum NimBLE und nicht Bluedroid:** seit arduino-esp32 3.3 ist NimBLE auf S3/C6 der Default-Host, `BLEMultiAdvertising` (der Bluedroid-Weg zu Extended Advertising) existiert dort nicht mehr, und mit Core 4.0 verschwindet Bluedroid ganz. NimBLE hat für den Fall „connectable Extended Advertising auf Coded PHY + GATT-Server" ein offizielles Beispiel. Versionshinweis: Core ≥ 3.3.11 **mit** NimBLE-Arduino ≥ 2.5.1 — ältere Kombinationen crashen beim Init.

**OTA-Design:** WLAN nur für das Update-Fenster. Espressif warnt explizit, dass eine ACL-Verbindung auf Coded PHY die WLAN-Performance massiv einbricht; BLE für die Dauer des Updates abzuschalten ist deshalb nicht nur eine Akku-Frage. Umsetzung mit `HTTPUpdate` (in-core, TLS-fähig, Pull von einem Server) oder einer eigenen Upload-Seite über `Update.h` (~100 Zeilen, keine Fremdlizenz). **ElegantOTA v3 nicht verwenden** — Lizenzwechsel auf AGPL-3.0, kommerzielle Nutzung nur über die kostenpflichtige Pro-Lizenz.

Partitionen: `default_8MB.csv` des XIAO-S3-Boards hat zwei App-Slots mit je 3,34 MB — für BLE + WLAN + OTA reichlich, keine Anpassung nötig.

## 6. Offene Punkte

- Ruhestrom im Advertising-Betrieb einmal messen (Abschnitt 4) — entscheidet nur noch über die Reserve, nicht über die Machbarkeit.
- Ob die Reichweite mit der externen 3-dBi-Antenne die des T-Echo (PCB-Antenne) tatsächlich schlägt, sollte einmal gemessen werden; die Beacon/Scanner-Sketches aus dem Testkit sind dafür gebaut.
- WLAN-Zugangsdaten: hart einkompiliert, per GATT setzbar oder AP-Modus mit Konfigurationsseite? Entscheidung offen.
- Keiner der Code-Vorschläge ist auf echter Hardware gebaut oder geflasht — die Espressif-Toolchain ist aus meiner Umgebung nicht erreichbar. Kompilieren und Messen passiert bei dir.
