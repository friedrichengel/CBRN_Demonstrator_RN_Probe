# Sicherheitshinweise / Security Policy

## Geltungsbereich

Dieses Projekt ist ein **privates Ausbildungs-Hilfsmittel**. Die Firmware
verwandelt einen LilyGo T-Echo in einen Bluetooth-Sender, der ein
Konfigurationspaket (Nuklid, Sender-Nr, "Dosisleistung") ausstrahlt. Das Gerät
**misst keine Strahlung und erzeugt keine**. Es steuert keine
sicherheitsrelevanten Systeme und hat keinen Internetzugriff.

Zugehörige Repositories mit eigener Security Policy:

- [CBRN_Demonstrator_RN_Sonde](https://github.com/friedrichengel/CBRN_Demonstrator_RN_Sonde) (Sonden-Firmware)
- [CBRN_Demonstrator_RN_App](https://github.com/friedrichengel/CBRN_Demonstrator_RN_App) (Android-App)

## Bekannte, bewusst akzeptierte Eigenschaften

- Der GATT-Dienst `0x1900` ist **ohne Pairing, ohne Bonding und ohne
  Verschlüsselung** les- und schreibbar. Jedes BLE-Gerät in Funkreichweite kann
  einen Trainings-Strahler umkonfigurieren (Sender-Nr, Nuklid, Dosiswert,
  Einheit, Aktiv, Reichweite). Das ist gewollt: Im Übungsbetrieb sollen
  Ausbilder die Geräte schnell und ohne Kopplungsdialog per App einstellen
  können. Schlimmstenfalls zeigt die Sonde einen falschen Übungswert an.
- Das Advertising ist **unverschlüsselt und unsigniert**. Der XOR-Prüfwert in
  Byte 9 der Manufacturer-Data erkennt nur Übertragungsfehler, keine
  Manipulation. Ein beliebiger BLE-Sender kann Strahler-Pakete nachbilden.
- **Keine PIN in dieser Firmware.** Die in Sonde und App hinterlegte
  Diagnose-PIN (`DIAG_PIN` in der `config.h` der Sonde, identisch in der App)
  existiert im Strahler nicht. Sie schützt dort lediglich die Anzeige der
  Simulationsmechanik vor Übungsteilnehmern und ist kein Sicherheitsmerkmal.
  Sie ist in den jeweiligen Repositories als bewusst akzeptiert dokumentiert.
- Der **USB-Bootloader (Adafruit nRF52 DFU)** nimmt serielles DFU ohne
  Authentifizierung an. Wer physischen USB-Zugriff hat, kann beliebige Firmware
  aufspielen. Das entspricht dem Auslieferungszustand des T-Echo.
- Die Konfiguration liegt **unverschlüsselt** im internen LittleFS
  (`/strahler.bin`). Sie enthält keine schutzwürdigen Daten.

## Was dieses Projekt nicht enthält

- keine Zugangsdaten, Schlüssel oder Zertifikate
- keine Telemetrie, keine Netzwerkverbindung außer BLE
- keine personenbezogenen Daten; die auf dem Kontakt-Bildschirm angezeigten
  Angaben sind die öffentlich bekannten Kontaktdaten eines Ortsverbands

## Meldung von Schwachstellen

Wenn du dennoch ein Problem findest, das über die oben genannten Punkte
hinausgeht, melde es bitte über ein
[GitHub-Issue](https://github.com/friedrichengel/CBRN_Demonstrator_RN_Probe/issues)
oder, falls es vertraulich sein soll, über die GitHub-Funktion
„Report a vulnerability" (Security Advisories) dieses Repositories.
