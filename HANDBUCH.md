# Handbuch — CBRN-RN Strahler

**Trainings-Pseudostrahler für den CBRN-RN-Demonstrator** · Firmware 1.02
Privates Projekt, eingesetzt in der Ausbildung im THW OV Berlin
Friedrichshain-Kreuzberg

> **Hinweis:** Dies ist ein **privates Projekt** und **kein offizielles
> Produkt oder Ausbildungsmittel des THW**. Das Gerät misst keine Strahlung
> und erzeugt keine; es ist ein Ausbildungsmittel, kein Dosimeter und keine
> Prüfstrahlenquelle. Quellcode und Lizenz (MIT):
> <https://github.com/friedrichengel/CBRN_Demonstrator_RN_Probe>

---

## 1. Was das Gerät ist und was es nicht ist

Der Strahler ist die **Quelle** des Demonstrator-Sets. Er sendet per Bluetooth
fortlaufend ein Datenpaket mit einer eingestellten Nuklid-Art, einer
Sender-Nummer und einer maximalen Dosisleistung. Die zugehörige **Sonde** oder
die **Android-App** empfangen dieses Paket und zeigen daraus eine
entfernungsabhängige Dosisleistung an.

> **Wichtig:** Es wird **keine Strahlung erzeugt.** Das Gerät ist ein reiner
> Funksender und dient ausschließlich der Ausbildung. Es darf nicht als
> Ersatz für eine echte Prüfquelle verwendet oder als solche gekennzeichnet
> werden.

Das Gerät hat ein E-Paper-Display: die Anzeige bleibt auch ohne Strom stehen,
und die Anzeige baut sich beim Aktualisieren sichtbar neu auf. Ein kurzes
Flackern oder Schwarz-Weiß-Umschlagen beim Bildwechsel ist völlig normal.

---

## 2. Bedienelement und Gesten

Bedient wird das Gerät mit **einem einzigen Knopf** — dem seitlichen
Hauptknopf (im Handbuch **BTN**). Die berührungsempfindliche Fläche auf der
Gehäusevorderseite wird **nicht** verwendet.

Es gibt **drei Gesten**:

| Geste | Definition |
|---|---|
| **kurz** | drücken und in **unter 0,8 Sekunden** wieder loslassen |
| **doppel** | zwei Kurzdrücke schnell hintereinander (zweiter innerhalb **0,35 s**) |
| **lang** | mindestens **0,8 Sekunden** gedrückt halten |

Wird der Knopf über die 0,8 Sekunden hinaus **gehalten**, wiederholt sich die
Aktion alle 0,25 Sekunden (Dauerlauf / Autorepeat). Das wird beim Einstellen
von Zahlenwerten genutzt.

---

## 3. Einschalten und Ausschalten

**Einschalten:** BTN einmal drücken. Das Gerät startet neu und zeigt nach
einigen Sekunden den Status-Bildschirm. Während des Startvorgangs steht noch
das Abschaltbild (THW-Logo) auf dem Display — das ist normal.

**Ausschalten:** Im Menü den Punkt **Aus** ansteuern und **BTN lang** drücken.
Das Display zeigt danach dauerhaft das THW-Logo mit „THW OFRK" und
„(ausgeschaltet)". Der Funksender ist aus, der Prozessor im tiefsten
Schlafmodus.

> Das Abschaltbild bleibt **ohne Stromverbrauch** stehen — ein Gerät mit
> Logo-Anzeige ist **aus**, nicht defekt und nicht leer.

**Wieder einschalten:** BTN einmal drücken.

Alle Einstellungen bleiben dauerhaft im Gerät gespeichert.

---

## 4. Der Status-Bildschirm

```
                                        [▮] 87%
  Q001                       [AN/LR]
  ─────────────────────────────────────
  Cs-137

  Max. Dosisleistung:

  250 µSv/h
  ─────────────────────────────────────
  kurz/lang/2x BTN                  v1.02
```

| Bereich | Bedeutung |
|---|---|
| **`Q001`** | Sender-Nummer, dreistellig |
| **`[AN]` / `[AUS]`** | sendet das Gerät gerade oder nicht |
| **`/LR`** angehängt | Reichweite steht auf „Lang" |
| **Nuklid** | eingestellte Nuklid-Bezeichnung |
| **Große Zahl mit Einheit** | eingestellte maximale Dosisleistung |
| **Unterste Zeile** | aktueller Menüpunkt bzw. `[EDIT]` im Änderungsmodus |
| **Oben rechts** | Akkusymbol mit Prozentwert, bei USB-Betrieb ein Blitz und `USB` |
| **Unten rechts** | Firmware-Version |

Steht das Gerät auf dem Menüpunkt „Status", zeigt die unterste Zeile den
Bedienhinweis `kurz/lang/2x BTN`. Bei allen anderen Menüpunkten steht dort
`> <Menüpunkt>`, im Änderungsmodus `[EDIT] <Menüpunkt>`.

---

## 5. Menü und Bedienlogik

Das Gerät hat kein aufklappendes Menü — es wird durch **neun Menüpunkte
durchgeschaltet**, der aktuelle Punkt steht unten im Display.

### 5.1 Navigation (außerhalb des Änderungsmodus)

| Geste | Wirkung |
|---|---|
| **kurz** | nächster Menüpunkt |
| **doppel** | vorheriger Menüpunkt |
| **lang** | Aktion des aktuellen Menüpunkts ausführen (siehe Tabelle) |

Die Liste läuft zyklisch: nach „Aus" folgt wieder „Status".

### 5.2 Die neun Menüpunkte

| # | Menüpunkt | Wirkung bei **lang** | Wertebereich |
|---|---|---|---|
| 1 | **Status** | schaltet direkt **Aktiv an/aus** | — |
| 2 | **Sender-Nr** | Änderungsmodus | 1 … 254 |
| 3 | **Nuklid** | Änderungsmodus | 9 Nuklide, zyklisch |
| 4 | **Max.Dosis** | Änderungsmodus | 0 … 65535 |
| 5 | **Einheit** | Änderungsmodus | nSv/h, µSv/h, mSv/h, Sv/h |
| 6 | **Aktiv** | Änderungsmodus (Ein/Aus) | an / aus |
| 7 | **Reichweite** | Änderungsmodus (Normal/Lang) | Normal / Lang |
| 8 | **Kontakt** | zeigt die Adresskarte des Ortsverbands | — |
| 9 | **Aus** | schaltet das Gerät ab | — |

### 5.3 Änderungsmodus (Edit)

In den Änderungsmodus kommt man mit **lang** auf einem Wert-Menüpunkt. Unten
im Display steht dann `[EDIT] <Menüpunkt>`.

| Geste | Wirkung |
|---|---|
| **kurz** | Wert um eins weiter (bei Auswahl-Listen zyklisch, bei Schaltern umschalten) |
| **lang halten** | Dauerlauf in der aktuellen Richtung |
| **doppel** | **Speichern**, Änderungsmodus verlassen, Senden mit neuen Werten neu starten |

**Richtungswechsel beim Dauerlauf:** Beim Betreten des Änderungsmodus zählt das
erste Halten **aufwärts**. Nach dem Loslassen dreht sich die Richtung um — das
zweite Halten zählt **abwärts**, das dritte wieder aufwärts, und so weiter. So
kommt man mit einem Knopf in beide Richtungen.

**Beschleunigung** (nur bei **Sender-Nr** und **Max.Dosis**): Beim Halten wird
die Schrittweite stufenweise größer — die ersten zehn Schritte je ±1, die
nächsten zehn je ±10, dann ±100, dann ±1000 und so weiter. Beim Loslassen und
bei jedem neuen Eintritt in den Änderungsmodus beginnt es wieder bei ±1.

Beispiel: Von 10 auf 5000 gelangt man mit einmal Halten. Für den Feinschliff
loslassen (Richtung dreht auf abwärts), kurz antippen und einzeln zurückzählen.

**Nuklid und Einheit** haben bewusst **keine Beschleunigung** — sie zählen
immer einzeln durch die Liste.

**Aktiv und Reichweite** sind Schalter: hier wirkt nur der erste Tick, es gibt
keinen Dauerlauf.

> **Wichtig:** Änderungen werden erst mit **doppel** gespeichert. Wer den
> Änderungsmodus nicht sauber verlässt, riskiert, dass der Wert beim nächsten
> Neustart wieder auf dem alten Stand steht.

### 5.4 Die einzelnen Einstellungen

#### Sender-Nr (1 … 254)

Nummer, die den Strahler eindeutig kennzeichnet. Sie erscheint im Funknamen
(`Strahler-NNN`), auf dem Display als `Q001` und in der Quellenliste der Sonde.

**In einer Übung sollte jede Nummer nur einmal vergeben werden.**

#### Nuklid

Zyklische Auswahl aus neun Nukliden:

| | | |
|---|---|---|
| Cs-137 | Ir-192 | U-238 |
| Co-60 | Ra-226 | Pu-239 |
| Am-241 | Th-232 | Se-75 |

Die Auswahl ist rein darstellend — sie beeinflusst weder Sendeleistung noch
den angezeigten Dosiswert, sondern dient dem Übungsszenario.

#### Max.Dosis (0 … 65535)

Die Dosisleistung, die die Sonde bei größtmöglicher Annäherung anzeigt, also
der **Vollausschlag** des Rundinstruments. Der Wert `0` ist zulässig und
sinnvoll, etwa um Hintergrundstrahlung darzustellen.

#### Einheit

Zyklisch: `nSv/h` → `µSv/h` → `mSv/h` → `Sv/h` → `nSv/h` …

> **Achtung:** Beim Wechsel der Einheit wird der **Zahlenwert nicht
> umgerechnet.** Aus `250 µSv/h` wird durch einen Einheitenwechsel
> `250 mSv/h` — nicht `0,25 mSv/h`. Der Zahlenwert muss anschließend von Hand
> angepasst werden.

#### Aktiv

Schaltet das Senden ein und aus. Bei `AUS` ist der Strahler für Sonde und App
unsichtbar. Praktisch zum Vorbereiten der Geräte, bevor die Übung beginnt.

Der Schnellweg: auf dem Menüpunkt **Status** einfach **lang** drücken.

#### Reichweite

| Modus | Anzeige | typische Reichweite im Freien | Empfänger |
|---|---|---|---|
| **Normal** | `[AN]` | ca. 30–50 m | jeder Bluetooth-Empfänger |
| **Lang** | `[AN/LR]` | ca. 150–200 m | nur Bluetooth-5-fähige Empfänger |

> **Wichtig:** Im Modus **Lang** ist das Gerät für ältere Bluetooth-Geräte
> **unsichtbar**. Die Sonde muss ebenfalls auf `LR` stehen. Standard-Bluetooth-
> Scanner am Handy finden das Gerät im Lang-Modus in der Regel nicht — die
> zugehörige App dagegen schon.

#### Kontakt

Zeigt formatfüllend die Adresskarte des Ortsverbands (Anschrift, Telefon, Fax,
E-Mail). **Jede beliebige Geste** führt zurück zum Status-Bildschirm.

#### Aus

Schaltet das Gerät ab (siehe Abschnitt 3).

---

## 6. Konfiguration über die Android-App

Alle Einstellungen außer **Kontakt** und **Aus** lassen sich auch bequem mit
der App „CBRN-RN Strahler Konfigurator" am Handy vornehmen. Dafür ist keine
Kopplung und kein Passwort nötig — das Gerät einfach in der App suchen und
antippen.

> Solange eine App-Verbindung besteht, **sendet der Strahler nicht**. Nach dem
> Trennen läuft er automatisch mit den neuen Werten weiter. Ein gerade per App
> verbundener Strahler ist deshalb im Mess-Betrieb nicht zu sehen.

Werte, die man am Gerät per Knopf ändert, werden ebenfalls sofort in die
App-Schnittstelle übernommen. Näheres im Handbuch der App.

---

## 7. Akku und Laden

- Oben rechts auf dem Status-Bildschirm steht ein kleines Akkusymbol mit
  Prozentwert (grobe Schätzung aus der Akkuspannung, nicht kalibriert).
- Bei angeschlossenem USB-Kabel zeigt das Symbol einen **Blitz** und daneben
  steht `USB` statt eines Prozentwerts. Während des Ladens ist der Akkuwert
  nicht aussagekräftig.
- Der Akkustand wird nur **alle 5 Minuten** aktualisiert, weil jede
  Display-Aktualisierung beim E-Paper Zeit und Strom kostet. Beim Ein- oder
  Ausstecken des USB-Kabels wird sofort aktualisiert.
- Im Änderungsmodus und in der Kontakt-Ansicht wird die Aktualisierung
  unterdrückt, damit dort nichts überschrieben wird.

---

## 8. Typische Abläufe

### Mehrere Strahler für eine Übung vorbereiten

Für jedes Gerät:

1. Einschalten, Akkustand prüfen.
2. **Sender-Nr** setzen — pro Gerät eine eigene Nummer.
3. **Nuklid** nach Szenario wählen.
4. **Einheit** zuerst wählen, **danach** die **Max.Dosis** einstellen (der
   Zahlenwert wird beim Einheitenwechsel nicht umgerechnet).
5. **Reichweite** einstellen — bei allen Geräten und der Sonde **gleich**.
6. **Aktiv** zunächst auf `AUS` lassen; erst am Übungsort einschalten
   (Menüpunkt „Status", **lang**).
7. Jede Änderung mit **doppel** speichern.

### Gerät im Gelände auslegen

1. Menüpunkt **Status** ansteuern.
2. **lang** drücken → Anzeige wechselt auf `[AN]` bzw. `[AN/LR]`.
3. Ablegen, verstecken oder befestigen. Das Gerät braucht keine weitere
   Bedienung.
4. Nach der Übung: **Status**, **lang** → `[AUS]`, und über Menüpunkt **Aus**
   abschalten.

---

## 9. Störungen und Abhilfe

| Beobachtung | Mögliche Ursache und Abhilfe |
|---|---|
| Sonde findet den Strahler nicht | Steht `[AN]` im Display? Stimmt die **Reichweite** auf beiden Geräten überein? Ist eine App gerade verbunden? |
| Display zeigt nur das THW-Logo | Gerät ist **ausgeschaltet**. BTN drücken. |
| Display flackert beim Umschalten | Normal beim E-Paper. Zum Beseitigen von Schatten wird gelegentlich ein voller Bildaufbau gemacht. |
| Blasse Schatten vom vorherigen Bild | Normal (Ghosting). Verschwindet beim nächsten vollen Bildaufbau, spätestens nach 5 Minuten oder 10 Aktualisierungen. |
| Wert springt beim Halten zu schnell | Beschleunigung greift nach zehn Schritten. Loslassen (Richtung dreht sich um) und einzeln zurücktippen. |
| Wert zählt in die falsche Richtung | Die Richtung wechselt nach jedem Loslassen. Einmal kurz loslassen und wieder halten. |
| Geänderter Wert ist nach Neustart weg | Der Änderungsmodus wurde nicht mit **doppel** verlassen. |
| Nach Einheitenwechsel steht ein unsinniger Wert | Der Zahlenwert wird nicht umgerechnet — Wert von Hand anpassen. |
| Akkuanzeige steht auf `USB` obwohl kein Kabel steckt | Sollte sofort wechseln; sonst maximal 5 Minuten bis zur nächsten Aktualisierung warten. |
| Gerät reagiert nicht auf den Knopf | Doppelklick zu langsam (Fenster 0,35 s) oder Langdruck zu kurz (0,8 s). Sonst USB anschließen und laden. |

---

## 10. Technischer Anhang

*Dieser Abschnitt richtet sich an Ausbilder und die Gerätebetreuung.*

### 10.1 Hardware

| Merkmal | Wert |
|---|---|
| Board | LilyGo T-Echo (nRF52840) |
| Display | 1,54″ E-Paper, 200 × 200 Pixel |
| Bedienung | seitlicher Hauptknopf (Touchfläche wird nicht verwendet) |
| Funk | Bluetooth 5, Sendeleistung +8 dBm |
| Speicher | Konfiguration dauerhaft im internen Dateisystem |
| Stromaufnahme im Aus-Zustand | ca. 1 µA (System-OFF) |
| Firmware | 1.02 |

### 10.2 Bedien-Zeitkonstanten

| Größe | Wert |
|---|---|
| Entprellung | 50 ms |
| Schwelle Langdruck | 800 ms |
| Doppelklick-Fenster | 350 ms |
| Dauerlauf-Takt | 250 ms |
| Beschleunigungsstufe | alle 10 Ticks Faktor 10 |
| Akku-Aktualisierung | alle 5 Minuten |
| Voller Bildaufbau | nach 10 Teil-Aktualisierungen, nach 5 Minuten oder nach App-Trennung |

### 10.3 Funkprotokoll (passiv empfangbar)

Gerätename beim Senden: `Strahler-NNN`. Sendeintervall 100 ms. Eine Verbindung
ist zum Empfang nicht nötig.

| Byte | Inhalt |
|---|---|
| 0–1 | Kennung `0xFF 0xFF` |
| 2 | Protokollversion `0x02` |
| 3 | Sender-Nummer (1–254) |
| 4 | Nuklid-Kennung (1–9) |
| 5–6 | max. Dosisleistung als Ganzzahl (Big Endian) |
| 7 | Einheiten-Index (0 = nSv/h, 1 = µSv/h, 2 = mSv/h, 3 = Sv/h) |
| 8 | Sendeleistung in dBm |
| 9 | Prüfsumme (XOR über Byte 2–8) |

**Nuklid-Kennungen:**

| ID | Nuklid | ID | Nuklid | ID | Nuklid |
|---|---|---|---|---|---|
| 1 | Cs-137 | 4 | Ir-192 | 7 | U-238 |
| 2 | Co-60 | 5 | Ra-226 | 8 | Pu-239 |
| 3 | Am-241 | 6 | Th-232 | 9 | Se-75 |

**Reichweiten-Modi:**

| Modus | Verfahren |
|---|---|
| Normal | klassisches Advertising, 1 Mbit/s |
| Lang | Bluetooth-5 Extended Advertising, Coded PHY S=8 |

### 10.4 Konfigurationsschnittstelle (GATT)

Dienst `0x1900`, sechs les- und beschreibbare Merkmale, **ohne Kopplung**:

| Kennung | Bedeutung | Format |
|---|---|---|
| `0x2B00` | Sender-Nr | uint8, 1–254 |
| `0x2B01` | Nuklid-ID | uint8, 1–9 |
| `0x2B02` | Max. Dosisleistung | uint16 **Little Endian**, 0–65535 |
| `0x2B03` | Aktiv | 0 = aus, 1 = an |
| `0x2B04` | Reichweite | 0 = Normal, 1 = Lang |
| `0x2B05` | Einheiten-Index | uint8, 0–3 |

Jedes Merkmal trägt eine Klartextbeschreibung und eine Formatangabe, sodass
gängige Bluetooth-Werkzeuge (z. B. nRF Connect) direkt Zahlenfelder statt
Bytefolgen anbieten.

> **Hinweis zur Byte-Reihenfolge:** Über GATT ist die Dosis **Little Endian**,
> in den passiv gesendeten Herstellerdaten dagegen **Big Endian**. Das sind
> zwei getrennte Schnittstellen.

Jeder Schreibvorgang wird sofort dauerhaft gespeichert und aktualisiert das
Display. Solange eine Verbindung besteht, wird das Senden nicht neu
konfiguriert; nach dem Trennen läuft es mit den neuen Werten weiter.

### 10.5 Gespeicherte Werte

Dauerhaft gespeichert werden Sender-Nr, Nuklid, Max. Dosisleistung, Einheit,
Aktiv-Zustand und Reichweite. Bei beschädigter oder veralteter
Konfigurationsdatei setzt das Gerät beim Start automatisch auf die Werkswerte
zurück: **Q001, Cs-137, 10 µSv/h, Aktiv = an, Reichweite = Normal.**

---

*Handbuch zur Firmware 1.02 · privates Projekt, kein offizielles THW-Produkt · MIT-Lizenz · <https://github.com/friedrichengel/CBRN_Demonstrator_RN_Probe>*
