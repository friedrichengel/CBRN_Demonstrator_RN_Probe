# CBRN-RN Demonstrator – Strahler-Firmware

Trainingsfirmware für das **THW OV Berlin Friedrichshain-Kreuzberg**.
Sie verwandelt einen handelsüblichen **LilyGo T-Echo** (nRF52840 + 1,54″
E-Paper) in einen Funk-Pseudostrahler für die Ausbildung von
CBRN-Erkundern: Das Gerät sendet per BLE-Advertising eine "Dosisleistung"
samt Nuklid und Sender-Nummer, die mit einem zugehörigen
Empfänger-/Probe-Gerät detektiert werden kann.

Es wird **keine echte Strahlung erzeugt**. Das Manufacturer-Data des
BLE-Advertisements ersetzt das Detektorsignal.

---

## Hardware

| Komponente | Detail |
|---|---|
| Board | LilyGo T-Echo (Stock, nRF52840) |
| Display | 1,54″ E-Paper, 200 × 200, Controller SSD1681 |
| Bedienung | Hauptknopf (P1.10) + kapazitive Touchfläche (P0.23) |
| Funk | BLE 5 via Nordic SoftDevice S140 (TX-Power +4 dBm) |
| Persistenz | InternalFS (LittleFS) im QSPI-Flash |

Wichtige Build-Eigenheit: `board = nrf52840_dk_adafruit` in
`platformio.ini`. **Nicht** auf `adafruit_feather_nrf52840` umstellen –
deren Pin-Map mappt die rohen `_PINNUM(0,N)`-Defines auf falsche GPIOs,
und nichts auf dem Board funktioniert mehr (EPD, LoRa, Flash). Siehe
`include/utilities.h` für die Pin-Belegung.

---

## Aufbau

```
src/
  main.cpp        - setup() + loop() + 5-min Battery-Refresh-Timer
  config.cpp      - persistente Konfiguration (LittleFS)
  display_ui.cpp  - E-Paper-Treiber + alle Zeichen-Funktionen
  ble_radio.cpp   - BLE-Advertising + GATT-Service
  input.cpp       - Button-/Touch-Polling mit Debounce + Long-Press
  menu.cpp        - Menü-Logik + Input-Handler
  power.cpp       - System-OFF / Aufwachen
  battery.cpp     - ADC-Lesung + Prozent-Schätzung
include/
  config.h        - Config-Struct, Nuklid-Enum
  display_ui.h    - display-Objekt + updateDisplay() / drawOffScreen()
  ble_radio.h     - bleBegin() / startAdvertising() / TX_POWER
  input.h         - PIN_BTN / PIN_TOUCH + inputBegin() / inputPoll()
  menu.h          - MenuState / View / Handler-Deklarationen
  power.h         - powerOff()
  battery.h       - batteryBegin() / batteryVolts() / batteryPercent()
  thw_logo.h      - 130x130 1-Bit-Bitmap des THW-Logos (auto-generiert)
  utilities.h     - T-Echo-Pinout (nicht editieren - vom Hardware-Layout vorgegeben)
tools/
  gen_thw_logo.py - regeneriert include/thw_logo.h aus dem Wikimedia-SVG
  .venv/          - lokales Python-Env mit svglib, reportlab, Pillow
```

Globale Zustände:

| Variable | wohnt in | Bedeutung |
|---|---|---|
| `cfg` | `config.cpp` | aktuelle Konfiguration (im RAM, persistiert in `/strahler.bin`) |
| `display` | `display_ui.cpp` | das Adafruit_SSD1681-Objekt |
| `menuState` | `menu.cpp` | welcher Menüpunkt ist gerade aktiv |
| `editMode` | `menu.cpp` | wird im Status-Menü gerade ein Wert editiert? |
| `view` | `menu.cpp` | `VIEW_NORMAL` (Status/Menü) oder `VIEW_KONTAKT` (Vollbild OV-Daten) |

---

## Bedienung

Zwei Tasten – Hauptknopf (BTN) und Touchfläche (TCH). Jeder Druck
zählt mit Debounce ≥ 50 ms; ein Halten ≥ 800 ms zählt als Long-Press.

### Menü-Punkte

Mit BTN-kurz weiter, mit TCH-kurz zurück:

1. **Status** – Hauptbildschirm: Sender-Nr, Aktiv-Flag, Nuklid, max. Dosisleistung
2. **Sender-Nr** – 1 … 254
3. **Nuklid** – Cs-137, Co-60, Am-241, Ir-192, Ra-226, Th-232, U-238, Pu-239
4. **Max. Dosis** – 0,50 … 6000,00 µSv/h (Schrittweite passt sich an)
5. **Aktiv** – BLE-Advertising an/aus
6. **Reichweite** – `Normal` (Legacy BLE, 1 Mbps, ~50 m) oder `Lang` (BLE 5 Coded PHY, ~150–200 m)
7. **Kontakt** – OV-Adresse / Telefon / E-Mail (Vollbild)
8. **Aus** – Gerät abschalten

### Bedienlogik

| Aktion | Was passiert |
|---|---|
| BTN kurz im Status | nächster Menüpunkt |
| BTN lang im Status | Aktiv an/aus toggeln (gleicher Shortcut wie Menüpunkt "Aktiv") |
| BTN lang in *Sender-Nr / Nuklid / Max.Dosis / Aktiv / Reichweite* | Edit-Modus an/aus |
| TCH kurz im Edit-Modus | Wert +1 (mit angepasster Schrittweite bei Dosis) |
| TCH lang im Edit-Modus | Autorepeat (250 ms). **1. langes Halten zählt aufwärts, beim nächsten Halten abwärts, dann wieder aufwärts** – Richtung wird beim Eintritt in den Edit-Modus auf "aufwärts" zurückgesetzt |
| BTN kurz im Edit-Modus | Speichern, Edit verlassen, Advertising neu starten |
| BTN lang im Menüpunkt *Kontakt* | Adresskarte im Vollbild zeigen |
| BTN kurz oder TCH kurz in Adresskarte | zurück zum Status |
| BTN lang im Menüpunkt *Aus* | Gerät schaltet ab (siehe unten) |

### Abschalten

Im Off-Zustand zeigt das E-Paper das THW-Logo (130×130 px Bitmap, aus
dem offiziellen Wikimedia-SVG generiert) mit "THW OFRK" und
"(ausgeschaltet)". Das Bild bleibt **ohne Stromverbrauch** sichtbar
(Eigenschaft der E-Paper-Technik). Der nRF52840 ist im
**System-OFF-Modus** (~1 µA), das BLE-Funkmodul aus, das Display zeigt
nur das gespeicherte Bild und erhält keine Updates mehr – kein Battery-
Indikator, kein Status.

**Wieder-Einschalten:** einmal den **Hauptknopf** drücken. Die
Touchfläche ist *absichtlich* nicht als Wake-Source konfiguriert –
kapazitive Berührung würde sonst beim Tragen in der Hosentasche oder
beim Ablegen das Gerät versehentlich aufwecken. Das Gerät führt einen
Reset durch und `setup()` läuft von vorne – also Boot bis Status-Screen
wie nach Stromanschluss; während des kurzen Boot-Vorgangs steht noch
das THW-Logo, bis der erste Full-Refresh den Status-Screen aufbaut.

### Akku-Anzeige

Oben rechts auf dem Status-Screen: kleines Battery-Icon + Prozentwert.
Lesung erfolgt über ADC am `Adc_Pin` (P0.04) durch den 2:1
Spannungsteiler des T-Echo. Lineare Schätzung zwischen 3,0 V (0 %) und
4,2 V (100 %) – nicht kalibriert, reicht aber als Trainings-Indikator.

Der Display-Refresh ist teuer (~2 s), also wird der Indikator nur **alle
5 Minuten** aktualisiert (`BATTERY_REFRESH_MS` in `main.cpp`). Der
Refresh wird unterdrückt während Edit-Modus und in der Kontakt-
Vollansicht, damit dort nichts ungewollt überschrieben wird.

### THW-Logo regenerieren

Falls Größe oder Aussehen des Logos angepasst werden soll:

```
tools/.venv/Scripts/python.exe tools/gen_thw_logo.py
```

`TARGET` und `THRESHOLD` im Script-Kopf justieren. Das Script lädt das
SVG einmalig auf `tools/_thw.svg` und rastert daraus
`include/thw_logo.h`. Der Header wird dann automatisch beim nächsten
Build mitkompiliert.

---

## BLE-Protokoll

Geräte-Name beim Advertising: `Strahler-NNN` (NNN = Sender-Nr).

### Reichweiten-Modus

Über `Reichweite` im Menü (oder GATT-Char `0x2B04`) lässt sich zwischen zwei
Funkmodi umschalten:

| Modus | PHY | Reichweite (Schätzung outdoor) | Scanner-Anforderung |
|---|---|---|---|
| `Normal` | Legacy advertising, 1 Mbps | ~30–50 m | jeder BLE 4.0+ Scanner |
| `Lang` (LR) | BLE 5 Extended Advertising, **Coded PHY S=8** | ~150–200 m | muss BLE 5 Coded PHY scannen können |

**Wichtig:** Im `Lang`-Modus wird das Advertising **nicht** mehr von BLE-4-Geräten
gesehen. Der zugehörige Empfänger / die Probe muss explizit auf Coded PHY scannen
(`ScanSettings.PHY_LE_CODED` auf Android, eigene nRF52-Firmware mit
`BLE_GAP_PHY_CODED` in `scan_phys`). Ein Standard-Phone-Scanner ohne explizite
Konfiguration findet das Gerät im `Lang`-Modus nicht. Der Status-Screen zeigt
`[AN/LR]` bzw. `[AUS/LR]` wenn der Modus aktiv ist.

### Manufacturer-Data (passiv)

Genügt einem reinen Scanner; **keine GATT-Verbindung nötig**.

| Byte | Inhalt |
|---:|---|
| 0–1 | `0xFF 0xFF` Company ID (custom/Test) |
| 2 | `0x01` Protokoll-Version |
| 3 | Sender-Nr (1–254) |
| 4 | Nuklid-ID (1–8, siehe `config.h`) |
| 5–6 | max. Dosisleistung in 0,01 µSv/h, Big Endian |
| 7 | TX-Power in dBm (int8) – derzeit `+4` |
| 8 | XOR-CRC über Byte 2–7 |

Advertising-Intervall: 100 ms.

### GATT-Service (aktiv beschreibbar)

Service-UUID `0x1900` mit fünf R/W-Characteristics ohne Pairing:

| UUID | Typ | Bedeutung |
|---|---|---|
| `0x2B00` | `uint8`  | Sender-Nr (1–254) |
| `0x2B01` | `uint8`  | Nuklid-ID (1–8) |
| `0x2B02` | `uint16 BE` | max. Dosisleistung in 0,01 µSv/h |
| `0x2B03` | `uint8`  | Aktiv (0=aus, 1=an) |
| `0x2B04` | `uint8`  | Reichweite (0=normal/1 Mbps, 1=lang/Coded PHY) |

Jeder Write persistiert die neue Konfiguration sofort, aktualisiert das
Display und re-startet das Advertising mit den neuen Werten.

---

## Bauen & Flashen

PlatformIO (CLI oder VSCode-Extension):

```
pio run -e t_echo            # bauen
pio run -e t_echo -t upload  # flashen (Bootloader-DFU via nrfutil)
pio device monitor           # 115200, Default-Settings
```

Beim ersten Build holt PlatformIO automatisch:

- Adafruit TinyUSB Library (`>= 3.1.0`) – nötig damit die Wire-Lib der
  Adafruit-nRF52-BSP übersetzt
- Adafruit EPD (`^4.5.4`) – SSD1681-Treiber für das T-Echo-Display
- Adafruit GFX Library (`^1.11.9`) – Schriften + Primitive

Die `platformio.ini` setzt zusätzlich:

- `lib_ldf_mode = deep+` – damit die LDF die TinyUSB-Header auch für
  Framework-Libraries (z.B. Wire) findet
- `lib_archive = no` – verhindert Reihenfolge-Probleme beim Linken
- `-DUSE_TINYUSB` und `-DCONFIG_NFCT_PINS_AS_GPIOS` – Standard-Flags
  für T-Echo

### Aufwachen aus Off-Zustand

Das Flashen aus dem Off-Zustand funktioniert direkt: USB anstecken,
`pio run -t upload` – nrfutil löst den DFU-Modus über UART aus und der
Bootloader nimmt die neue Firmware an.

---

## Bekannte Eigenheiten

- **`File`-Namensraumkonflikt**: `Adafruit_LittleFS_Namespace::File` und
  SdFat's globales `typedef File32 File` kollidieren, sobald
  TinyUSB-MSC mitgezogen wird. Lösung in `config.cpp`: Namespace-Alias
  `namespace LFS = Adafruit_LittleFS_Namespace;` statt
  `using namespace`.
- **Default-GFX-Font kennt kein ß und keine Umlaute**: Adresstext nutzt
  daher ASCII-Schreibweise ("Strasse").
- **EPD BUSY tri-stated im Deep-Sleep**: Direkt nach dem Booten zeigt
  ein BUSY-Read 0, *bevor* der Controller aufgeweckt wurde. Das ist
  normal und kein Hinweis auf fehlende Stromversorgung. Der erste
  `display.begin()`-Aufruf treibt den Pin korrekt.
- **Flackern beim Refresh**: Der SSD1681 macht beim Full-Refresh das
  klassische E-Paper-Invertieren (schwarz/weiß flackert ~3 s lang).
  `display_ui.cpp` nutzt deshalb `displayPartial()` für routine
  Aktualisierungen (Status, Kontakt) und alle 20 Updates einen
  Full-Refresh zum Aufräumen von Ghosting. Beim Abschalten
  (`drawOffScreen`) wird immer Full gemacht, damit das Logo sauber
  steht. Die Adafruit-EPD-Library spammt bei jedem Partial-Refresh die
  Zeilen `---------------partial=============` und `Transfering:` auf
  Serial – kommt aus der Library und ist harmlos.
