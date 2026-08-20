# Stückliste – Strahler-Übungsgerät (ESP32-Variante)

Stand: 20.08.2026 · ein Satz = ein Gerät · Preise netto

**Zielaufbau:** Kunststoffgehäuse mit externer 2,4-GHz-Antenne, Hauptschalter in
der Akkuleitung, Betriebs-LED außen, USB-C-Buchse nach außen zum Laden.
Kein Display – eingestellt wird per Handy-App über BLE, Firmware kommt über WLAN.

---

## 1. Hauptteile

Bestellnummern und Preise wie im DigiKey-Warenkorb vom 20.08.2026 bestätigt
(Stückpreis bei Abnahme von 4 Stück, netto).

| Pos | Artikel | MPN | DigiKey-Nr. | Stückpreis | Anmerkung |
|---|---|---|---|---|---|
| 1 | **Seeed XIAO ESP32S3** | 113991114 | **1597-113991114-ND** | **6,55 €** | ESP32-S3 (BLE 5 Coded PHY), U.FL-Buchse, **2,4-GHz-Antenne liegt bei**, LiPo-Laden 100 mA onboard |
| 2 | **Pigtail U.FL → SMA-Buchse**, Bulkhead, 3,9" ≈ 100 mm | Taoglas **CAB.719** | **931-1187-ND** | **4,44 €** | Führt die Antenne an die Gehäusewand |
| 3 | **Antenne 2,4 GHz, SMA male**, Knickdipol ~3 dBi | Taoglas **GW.15.2113** | **931-1062-ND** | **5,67 €** | IP65, 108 mm. Für Reichweitenvergleiche optional 5 dBi: Siretta DELTA6B/x/SMAM/S/S/11 (Mouser 419-D6BXSMAMSS11, 12,91 €) |
| 4 | **Gehäuse ABS**, 85 × 56 × 26 mm | Hammond **1591MBK** | **HM362-ND** | **6,69 €** | FR-ABS UL94 V-0, IP54, M3-Metallschrauben |
| 5 | **Hauptschalter, 2-polig, mit AUS-Stellung** | E-Switch **100DP3T1B1M1QEH** | **EG2413-ND** | **4,51 €** | DPDT **On-Off-On**, Lötfahnen, Bohrung **6,35 mm**. Nur eine Seite wird belegt → wirkt als 2-poliger Ein/Aus-Schalter |
| 6 | **Betriebs-LED, Panel-Einbau**, grün, 5 mm | Dialight **5590202003F** | **350-2786-ND** | **2,75 €** | 5-V-Typ mit **integriertem Vorwiderstand**, Litzen, Bohrung **6,35 mm** – gleicher Bohrer wie Pos. 5. An 3,3 V ca. 6 mA |
| 7 | **LiPo 3,7 V, 950 mAh**, PCM, offene Litzen 70 mm | Jauch **LP523450JU+PCM+2 WIRES 70MM** | **3027-LP523450JU+PCM+2WIRES70MM-ND** | **10,01 €** | Marktplatzprodukt: Versand durch Jauch Quartz (Villingen-Schwenningen), ca. 6 Tage, **separate Versandgebühr möglich**. Offene Litzen sind hier von Vorteil – gehen direkt an den Schalter |

**Summe pro Gerät: 40,62 € netto** (Pos. 1–7, ohne Kleinteile und Versand).
Für 4 Geräte: **162,48 €** zzgl. Marktplatz-Versand für Pos. 7.

### Kleinteile – nicht im Warenkorb

| Artikel | Menge/Gerät | Zweck |
|---|---|---|
| Widerstand **200 kΩ, 1 %** (Metallschicht, ≥ 0,1 W) | 2 | Spannungsteiler für die Akkumessung an A0 |
| Kondensator **100 nF** (Keramik) | 1 | Siebung am ADC-Eingang |
| Litze 0,14–0,25 mm², Schrumpfschlauch | – | Schalter, LED, Akku |

Die beiden Widerstände und der Kondensator sind der einzige Grund für die
Akkuanzeige (Battery Service in der Handy-App, LED-Warnung bei < 15 %). Wer
darauf verzichtet, lässt sie weg – dann meldet die Firmware konstant 0 % und die
Warnschwelle muss im Code abgeschaltet werden.

---

## 2. Alternative Bezugsquellen

| Artikel | Amazon.de / Deutschland | Anmerkung |
|---|---|---|
| Akku 1200 mAh, JST-PH 2.0, 5 mm flach | BerryBase **LP-503562-J**, 5,50 € (auch Amazon **B0C7L82R1F**) | Dünner als die Jauch-Zelle, passt besser neben das Board |
| Akku 2000 mAh, JST-PHR-2 | Eckstein **ZB07009**, 7,95 € | Nur mit externem 500-mA-Lader sinnvoll, sonst zu lange Ladezeit |
| Kippschalter 2-polig mit Mittelstellung AUS | Amazon **B07H5Q76T8** (Heschen MTS-203) | ⚠️ „EIN/EIN"-Typen (MTS-202-Varianten ohne Aus) sind ungeeignet |
| LED 5 mm, 5 V, vorverdrahtet | Amazon **B01M10BR8V** (Lumonic, 10 Stück) | Plus Fassung **B083Q9Q1ZR**. ⚠️ Die verbreiteten 12-V-Typen leuchten an 3,3 V nicht |

⚠️ **RP-SMA vermeiden.** Taoglas GW.22.5151 und Siretta …/RP/… sind
Reverse-Polarity – sie schrauben sich auf die SMA-Buchse, machen aber keinen
Innenleiterkontakt. Immer „SMA male, Standardpolarität" bestellen.

---

## 3. Mechanik

| Bearbeitung | Maß | Ort |
|---|---|---|
| Bohrung Antennendurchführung (SMA-Bulkhead, 1/4"-36 UNS) | Ø **6,5 mm** | Stirnseite (56 mm breit) |
| Bohrung Hauptschalter | Ø **6,35 mm** | Stirnseite |
| Bohrung Betriebs-LED | Ø **6,35 mm** | Deckel oder Stirnseite |
| Ausschnitt USB-C | ca. **10 × 4 mm**, bündig zur Buchse des XIAO | gegenüberliegende Stirnseite |

Einbautiefe des Kippschalters ca. 13–19 mm hinter der Wand – bei 22 mm Innenhöhe
knapp, aber machbar. Flachere Alternative: C&K **7201J51CQE22** Wippschalter
(DigiKey 3729532), Snap-in-Ausschnitt 12,7 × 15,2 mm, freigegeben für 1,57–3,18 mm
Wandstärke.

---

## 4. Verdrahtung

```
Akku +  ──►  Schalter Pol 1  ──►  XIAO BAT+  (Lötpad Unterseite)
Akku −  ──►  Schalter Pol 2  ──►  XIAO BAT−  (Pad näher an der USB-Buchse)

BAT+ (nach dem Schalter) ──[200 kΩ]──┬──► A0 / GPIO1
                                     ├──[200 kΩ]──► GND
                                     └──[100 nF]──► GND

Betriebs-LED  +  ──►  D1 / GPIO2      (Muster kommen aus der Firmware)
Betriebs-LED  −  ──►  GND

U.FL-Buchse XIAO ──► Pigtail ──► SMA-Buchse Gehäusewand ──► Antenne
```

Hinweise:

- **Polarität des Akkus vor dem ersten Anlöten messen.** Bei den XIAO-Pads liegt
  der Minuspol näher an der USB-Buchse. Verpolt zerstört die Zelle das Board sofort.
- Die U.FL-Buchse des XIAO ist mechanisch empfindlich – Pigtail nach dem Aufstecken
  mit einem Tropfen Heißkleber oder einer Kabelbinderschlaufe zugentlasten,
  nicht am Kabel ziehen.
- Der Schalter trennt den Akku, nicht USB. Geladen wird also nur mit
  eingeschaltetem Gerät (der Lader sitzt auf dem XIAO hinter den BAT-Pads).
- Antennenpfad: Beim XIAO ESP32S3 ist die U.FL-Buchse der einzige Antennenanschluss –
  es muss also keine Umschaltung gesetzt werden (anders als beim XIAO ESP32C6,
  der zusätzlich eine Keramikantenne und einen HF-Schalter an GPIO14 hat).

---

## 5. Energiebudget (8-h-Übungstag)

| | Annahme | über 8 h |
|---|---|---|
| ESP32-S3, BLE-Advertising 100 ms, 240 MHz | 25–40 mA | 200–320 mAh |
| dito mit `setCpuFrequencyMhz(80)` | 15–25 mA | 120–200 mAh |
| Betriebs-LED, gepulst (1 Blitz/3 s) | 0,05 mA | 0,4 mAh |

Ladezeit bei den 100 mA des XIAO: **950 mAh ≈ 11 h**, 1200–1400 mAh ≈ 14–16 h,
1900 mAh ≈ 20–22 h. Deshalb die kleine Zelle – sie ist über Nacht sicher wieder
voll und hat für 8 h immer noch drei- bis vierfache Reserve. Wenn eine größere
Zelle gewünscht ist: externer Lader **Adafruit Micro-Lipo USB-C 4410**
(Mouser 485-4410, 5,12 €) mit 500 mA, dann liegt dessen USB-C-Buchse an der
Gehäusewand und die des XIAO bleibt fürs Flashen.

---

## 6. Vorbehalt zu den Preisen

Die Mouser-EU-Preise sind direkt von eu.mouser.com verifiziert.
**DigiKey-Preise und Lagerbestände waren nicht auslesbar** (die Produktseiten
werden per JavaScript gerendert) – die Bestellnummern stimmen, die Preise bitte
im Warenkorb prüfen. Der Preis zu Pos. 5 stammt aus dem server-gerenderten
digikey.com-Spiegel (Qty 1) und ist nur eine Orientierung.
