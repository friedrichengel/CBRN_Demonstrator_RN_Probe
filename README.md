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

Nur **ein** Bedienelement: der seitliche Hauptknopf (BTN). Die kapazitive
Touchfläche des T-Echo wird nicht verwendet (war hardwareseitig nicht
zuverlässig genug — der TTP223 lieferte auf unseren Boards spontane LOW-Pulse
durch EMI vom EPD-Refresh und reagierte teilweise nicht auf echte Berührung).

Drei Gesten am BTN:

| Geste | Definition |
|---|---|
| **Kurz** | drücken und in unter 800 ms wieder loslassen |
| **Doppel** | zwei Kurzdrücke innerhalb von 350 ms |
| **Lang** | ≥ 800 ms gedrückt halten |

Debounce ist 50 ms.

### Menü-Punkte

Mit BTN-kurz weiter, mit TCH-kurz zurück:

1. **Status** – Hauptbildschirm: Sender-Nr, Aktiv-Flag, Nuklid, max. Dosisleistung
2. **Sender-Nr** – 1 … 254
3. **Nuklid** – Cs-137, Co-60, Am-241, Ir-192, Ra-226, Th-232, U-238, Pu-239, Se-75
4. **Max. Dosis** – Ganzzahl, 0 … 65535. Grundschritt immer ±1; im Hold beschleunigt die Schrittweite stufenweise auf ±10, ±100, ±1000 …
5. **Einheit** – `nSv/h` / `uSv/h` / `mSv/h` / `Sv/h` (zyklisch). Reine Anzeige-Einheit – der Zahlwert wird *nicht* automatisch umgerechnet beim Wechsel.
6. **Aktiv** – BLE-Advertising an/aus
7. **Reichweite** – `Normal` (Legacy BLE, 1 Mbps, ~50 m) oder `Lang` (BLE 5 Coded PHY, ~150–200 m)
8. **Kontakt** – OV-Adresse / Telefon / E-Mail (Vollbild)
9. **Aus** – Gerät abschalten

### Bedienlogik

**Außerhalb Edit-Modus (Menü-Navigation):**

| Geste | Was passiert |
|---|---|
| Kurz | nächster Menüpunkt |
| Doppel | vorheriger Menüpunkt |
| Lang | Aktion des aktuellen Menüpunkts: |
| | • Status → Aktiv an/aus toggeln |
| | • Sender-Nr / Nuklid / Max.Dosis / Aktiv / Reichweite → Edit-Modus betreten |
| | • Kontakt → Adresskarte anzeigen |
| | • Aus → Gerät abschalten |

**Innerhalb Edit-Modus:**

| Geste | Was passiert |
|---|---|
| Kurz | Wert ±1 (Einheit zyklisch, Aktiv/Reichweite Toggle) |
| Lang (halten) | Autorepeat mit aktueller Richtung. **1. Halten zählt hoch, 2. Halten runter, 3. hoch, …** Loslassen flippt die Richtung für das nächste Halten. Beim Eintritt in den Edit-Modus startet die Richtung auf "hoch". Für Aktiv/Reichweite (Bool) wirkt nur der erste Tick — kein Autorepeat-Flackern. **Beschleunigung (nur Sender-Nr & Max.Dosis):** alle 10 Ticks wächst die Schrittweite um Faktor 10, **unabhängig vom aktuellen Wert** — Ticks 1–10 ±1, 11–20 ±10, 21–30 ±100, 31–40 ±1000, … Beim Loslassen oder bei jedem neuen Edit-Eintritt wieder bei ±1. So kommt man im Bereich 0–10 in 1er-Schritten exakt hin und kann notfalls per Richtungsflip einzeln zurückgehen. Nuklid und Einheit bleiben immer ±1 (zyklische Auswahl, Beschleunigung wäre unsinnig). |
| Doppel | Speichern, Edit-Modus verlassen, Advertising mit neuen Werten neu starten |

**In Kontakt-Vollansicht:**

| Geste | Was passiert |
|---|---|
| Jede Geste | zurück zum Status-Screen |

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

**Wenn USB angeschlossen ist** (VBUS am `NRF_POWER->USBREGSTATUS` erkannt),
zeigt das Icon einen Blitz und der Text neben dem Icon steht auf `USB`
statt einem Prozentwert – die ADC-Lesung ist beim Laden nicht
aussagekräftig (Versorgungsspannung verfälscht den Akku-Wert auf "100 %").
Beim Abstecken triggert die Firmware sofort einen Display-Refresh,
damit nicht 5 min lang die alte USB-Anzeige stehen bleibt.

Der periodische Display-Refresh ist teuer (~2 s), also wird der Indikator
ansonsten nur **alle 5 Minuten** aktualisiert (`BATTERY_REFRESH_MS` in
`main.cpp`). Der Refresh wird unterdrückt während Edit-Modus und in
der Kontakt-Vollansicht, damit dort nichts ungewollt überschrieben wird.

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
| 2 | `0x02` Protokoll-Version |
| 3 | Sender-Nr (1–254) |
| 4 | Nuklid-ID (1–9, 9 = Se-75, siehe `config.h`) |
| 5–6 | max. Dosisleistung als Ganzzahl, Big Endian |
| 7 | Einheits-Index (0=nSv/h, 1=uSv/h, 2=mSv/h, 3=Sv/h) |
| 8 | TX-Power in dBm (int8) – derzeit `+4` |
| 9 | XOR-CRC über Byte 2–8 |

Advertising-Intervall: 100 ms.

Empfänger müssen den Wert in Byte 5–6 zusammen mit dem Einheits-Index in
Byte 7 interpretieren — z.B. `5-6 = 250, 7 = 2` → 250 mSv/h. Der GATT-Char
`0x2B05` (siehe unten) liefert denselben Index, ist aber nur nach Connect
verfügbar; für passive Scanner ist Byte 7 die maßgebliche Quelle.

> **Protokoll-Versions-Bump:** Version 0x01 (Vorgänger-Firmware) hatte 9 Byte
> ohne Einheits-Byte; die Dosis wurde dort fest als `0.01 × uSv/h` interpretiert.
> Empfänger sollten Byte 2 prüfen und bei `0x01` das alte Schema, bei `0x02`
> das neue Schema parsen.

### GATT-Service (aktiv beschreibbar)

Service-UUID `0x1900` mit sechs R/W-Characteristics ohne Pairing:

| UUID | App-Typ (CPF) | Beschreibung |
|---|---|---|
| `0x2B00` | `uint8`  / „Sender-Nr"  | Sender-Nr (1–254) |
| `0x2B01` | `uint8`  / „Nuklid-ID"  | Nuklid-ID (1–9, 9 = Se-75); unbekannte IDs werden verworfen |
| `0x2B02` | `uint16 LE` exp=0 / „Max.Dosis" | max. Dosisleistung – Ganzzahl in *Einheit* (1-Byte-Writes erlaubt für 0…255) |
| `0x2B03` | `boolean` / „Aktiv"     | 0=aus, 1=an |
| `0x2B04` | `boolean` / „Long Range" | 0=normal/1 Mbps, 1=lang/Coded PHY |
| `0x2B05` | `uint8`  / „Einheit-Idx" | Einheit-Index (0=nSv/h, 1=uSv/h, 2=mSv/h, 3=Sv/h) |

Jede Charakteristik liefert zusätzlich

- einen **User Description Descriptor** (0x2901) mit dem Klartextnamen oben und
- einen **Characteristic Presentation Format Descriptor** (0x2904) mit Typ &
  Exponent.

Dadurch zeigt nRF Connect (Mobile/Desktop) direkt den passenden Eingabetyp
(UINT8 / UINT16 / Boolean) statt „Byte Array" – die Werte können als normale
Ganzzahlen eingegeben werden, z.B. bei Max.Dosis einfach `100` und das Display
zeigt `100 <Einheit>`.

Charakteristika nutzen `setMaxLen`, kürzere Writes werden vom SoftDevice
akzeptiert und im Callback per Längenprüfung ggf. verworfen – nRF Connect &
Co. sehen damit keine `INVALID_ATTRIBUTE_LENGTH`-Fehler mehr.

> **Endianness-Detail:** `0x2B02` (Dosis) verwendet im GATT **Little Endian**
> (GATT-Standard, passt zur uint16-CPF). Die parallel ausgesendete
> Manufacturer-Data (siehe oben) nutzt dagegen Byte 5–6 **Big Endian** – das ist
> ein anderes Protokoll für Empfänger-Firmware und bleibt aus Kompatibilitäts-
> gründen unverändert.

Jeder Write persistiert die neue Konfiguration sofort und aktualisiert das
Display (Partial-Refresh). Solange eine Verbindung aktiv ist, wird das
Advertising **nicht** neu konfiguriert (der SoftDevice hat nur einen
Peripheral-Slot und würde sonst `NRF_ERROR_INVALID_STATE` werfen) – nach
dem Disconnect läuft das Advertising automatisch mit den neuen Werten weiter.
Beim Disconnect wird zusätzlich **einmal voll geflashed**, um Ghosting durch
viele Partials während der Session zu löschen.

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

### Wie der Upload abläuft

`pio run -t upload` ruft nicht direkt `adafruit-nrfutil` auf, sondern
[`tools/dfu_upload.py`](tools/dfu_upload.py) (eingehängt als `upload_command`
in `platformio.ini`). Der Grund: mit den PlatformIO-Standardeinstellungen
schlägt der Upload auf diesem Board zuverlässig fehl. Im Detail steht das im
Kopf des Skripts, kurz:

- Das Board-Profil `nrf52840_dk_adafruit` setzt kein `use_1200bps_touch`, also
  redet nrfutil mit der App-CDC statt mit dem Bootloader („No data received on
  serial port"). PlatformIOs `wait_for_upload_port` hilft nicht, weil macOS
  App (PID `0x8029`) und Bootloader (`0x0029`/`0x002A`) denselben Portnamen
  gibt – es wartet auf einen *neuen* Portnamen, der nie kommt.
- Der Bootloader nimmt serielles DFU **nur in einem kurzen Fenster direkt nach
  seinem Start** an. Danach bleibt der CDC-Port sichtbar, antwortet aber nicht
  mehr, und eine abgebrochene Sitzung lässt sich nicht fortsetzen.
- nrfutil wartet nur 1 s aufs ACK, sendet danach mit zurückgesetzter
  Sequenznummer weiter (was den Bootloader abwürgt) und liefert bei Fehlern
  trotzdem Exit-Code 0 – PlatformIO meldet dann `[SUCCESS]`, obwohl nichts
  geflasht wurde.

Der normale Ablauf braucht deshalb keinen Tastendruck: Das Skript versetzt die
laufende App per 1200-bps-Touch in den DFU-Modus und flasht sofort los. Ein
Durchlauf dauert ~15 s und endet mit „Board laeuft mit der neuen Firmware."

### Wenn der Upload abbricht

Das Board verliert während des DFU sporadisch die USB-Verbindung (`Device not
configured` bzw. ACK bleibt an zufälliger Stelle aus). Dann ist die
DFU-Sitzung verloren **und die App ist bereits gelöscht** (Single-Bank-DFU
löscht vor dem Schreiben) – das Board hängt also im Bootloader und kann sich
nicht mehr selbst in DFU versetzen.

In diesem Fall fordert das Skript einen Tasterdruck an und wartet:

```
>>> Bitte jetzt den Reset-Taster antippen (oder USB kurz abziehen und wieder
    anstecken). <<<
```

Reset drücken – das Skript erkennt den Bootloader-Start und flasht in dem
Moment weiter, in dem das Fenster offen ist. Per Hand ist das nicht zu
treffen, deshalb macht das Skript es selbst. Häuft sich der Abbruch, liegt es
an der Verbindung: anderes USB-C-Kabel, direkt am Rechner statt über Hub oder
Dock, Akku geladen.

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
  Aktualisierungen (Status, Kontakt). Ein Full-Refresh zum Aufräumen
  von Ghosting wird ausgelöst durch drei unabhängige Trigger — was
  zuerst eintritt:
  - **10 Partials** seit dem letzten Full (interaktionsbasiert) oder
  - **5 min** seit dem letzten Full (zeitbasiert, für Standby) oder
  - **BLE-Disconnect** (über `forceFullRefresh()` aus `onBleDisconnect`).

  Beide Zähler werden bei jedem Full zurückgesetzt. Beim Abschalten
  (`drawOffScreen`) wird immer Full gemacht, damit das Logo sauber
  steht. Die Adafruit-EPD-Library spammt bei jedem Partial-Refresh die
  Zeilen `---------------partial=============` und `Transfering:` auf
  Serial – kommt aus der Library und ist harmlos.
