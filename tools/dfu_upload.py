#!/usr/bin/env python
"""Robuster serieller DFU-Upload fuer das T-Echo.

Wird von platformio.ini als `upload_command` eingehaengt und ersetzt den
direkten Aufruf von adafruit-nrfutil, weil dessen CLI drei Probleme hat:

1. Kein DFU-Reset: das Board-Profil `nrf52840_dk_adafruit` setzt kein
   `use_1200bps_touch`, also redet nrfutil mit der App-CDC statt mit dem
   Bootloader ("No data received on serial port"). PlatformIOs eingebautes
   `wait_for_upload_port` hilft nicht, weil macOS App (PID 0x8029) und
   Bootloader (PID 0x0029/0x002A) denselben Geraetenamen gibt - es wartet 5 s
   auf einen *neuen* Portnamen, der nie kommt.

2. Ein einzelnes verspaetetes ACK zerstoert den Transfer: nrfutil wartet nur
   1 s (ACK_PACKET_TIMEOUT), setzt danach die HCI-Sequenznummer auf 0 zurueck
   und sendet einfach weiter. Der Bootloader sieht dann Pakete ausser der
   Reihe, bricht ab und resettet -> "Attempting to use a port that is not
   open" mitten im Flashen. Hier wird das ACK-Timeout hochgesetzt und ein
   Timeout sofort zum Abbruch mit anschliessendem Neuversuch.

3. Falsche Exit-Codes: nrfutils `serial`-Kommando gibt bei Fehlern `False`
   zurueck statt einen Exit-Code != 0, weshalb PlatformIO einen gescheiterten
   Upload als [SUCCESS] meldet.

Was das Skript NICHT reparieren kann: dieses Board verliert waehrend des DFU
sporadisch die USB-Verbindung ("Device not configured", ACK bleibt an
zufaelliger Stelle aus). Das ist ein Hardware-/Verbindungsproblem (Kabel, Hub,
Stromversorgung).

Der entscheidende Punkt beim Zeitverhalten: dieser Bootloader nimmt serielles
DFU nur in einem kurzen Fenster direkt nach seinem Start an (gemessen: Start
0,6 s danach klappt, Minuten danach nie). Danach bleibt er als CDC-Port
sichtbar, antwortet aber nicht mehr - und eine abgebrochene Sitzung laesst sich
nicht fortsetzen. Deshalb wird hier nach dem DFU-Einsprung ohne Umwege
losgeflasht und nach einem Fehlschlag unterschieden:

* Geraet ist vom Bus verschwunden -> der Bootloader startet neu, sofort nochmal
* Geraet bleibt sichtbar -> er haengt; dann wird zum Reset-Taster aufgefordert
  und in dem Moment geflasht, in dem der Bootloader wieder hochkommt (per Hand
  ist dieses Fenster nicht zu treffen)

Aufruf: dfu_upload.py -pkg <firmware.zip> [-p <port>]
"""

import argparse
import glob
import os
import site
import sys
from time import sleep

VID = 0x239A
# Die App enumeriert mit USB_PID=0x8029 (build_flags des Boards), der
# Bootloader je nach DFU-Einsprung mit 0x0029 oder 0x002A - deshalb gilt alles
# ausser der App-PID als Bootloader.
PID_APPLICATION = 0x8029

TOUCH_BAUD = 1200
DFU_BAUD = 115200
DFU_TIMEOUT = 15.0
SETTLE_TIMEOUT = 10.0
APP_BOOT_TIMEOUT = 10.0
# Feines Raster: nach dem Bootloader-Start zaehlt jede Zehntelsekunde, weil
# sein DFU-Fenster nur kurz offen ist.
POLL = 0.02
# Grosszuegiger als nrfutils 1 s: nach dem Start-Paket wartet nrfutil nur
# (Imagegroesse/4096)*89,7 ms auf das Loeschen des Flash, ein Single-Bank-Erase
# kann laenger dauern. Laenger als hier hat in der Praxis nichts gerettet - ein
# haengender Bootloader antwortet auch nach 30 s nicht mehr.
ACK_TIMEOUT = 10.0

# Notbremse gegen Endlosschleifen, wenn USB-Abbrueche den Bootloader immer
# wieder neu starten. Der normale Ausstieg ist, dass niemand mehr Reset
# drueckt - solange gedrueckt wird, wird weiter versucht.
ATTEMPTS = 10
DEVICE_TIMEOUT = 10.0
RESTART_TIMEOUT = 5.0
# So lange wird auf den Tasterdruck gewartet, wenn der Bootloader haengt.
RESET_WAIT = 120.0


def die(message):
    sys.stderr.write("dfu_upload: %s\n" % message)
    sys.exit(1)


def load_nrfutil():
    """Macht das site-packages von tool-adafruit-nrfutil importierbar."""
    core_dir = os.environ.get(
        "PLATFORMIO_CORE_DIR", os.path.expanduser("~/.platformio"))
    packages_dir = os.environ.get(
        "PLATFORMIO_PACKAGES_DIR", os.path.join(core_dir, "packages"))
    candidates = sorted(glob.glob(
        os.path.join(packages_dir, "tool-adafruit-nrfutil*", "site-packages")))
    if not candidates:
        die("tool-adafruit-nrfutil nicht in %s gefunden" % packages_dir)

    contrib_dir = candidates[-1]
    site.addsitedir(contrib_dir)
    sys.path.insert(0, contrib_dir)


def find_device(list_ports):
    for port in list_ports.comports():
        if port.vid == VID:
            return port
    return None


def is_bootloader(device):
    return device.pid != PID_APPLICATION


def enter_dfu(Serial, list_ports, hint_port):
    """Bringt das Board in den DFU-Modus und gibt den stabilen Port zurueck."""
    device = wait_for_device(list_ports, DEVICE_TIMEOUT)
    if device is None:
        if hint_port:
            print("Kein nRF52-Geraet (VID 0x239A) gefunden, nutze %s"
                  % hint_port)
            return hint_port
        die("kein nRF52-Geraet (VID 0x239A) gefunden - USB-Kabel pruefen")

    if is_bootloader(device):
        print("Board ist im DFU-Modus (%s, PID 0x%04X)"
              % (device.device, device.pid or 0))
    else:
        print("Reset via %d bps auf %s (-> DFU)" % (TOUCH_BAUD, device.device))
        try:
            handle = Serial(port=device.device, baudrate=TOUCH_BAUD)
            handle.setDTR(False)
            handle.close()
        except Exception as exc:
            print("  (Port beim Touch geschlossen: %s)" % exc)

        device = wait_for_bootloader(list_ports, DFU_TIMEOUT)
        if device is None:
            die("Bootloader kam nicht hoch - Reset-Taster antippen und erneut "
                "hochladen")
        print("Bootloader da: %s (PID 0x%04X)"
              % (device.device, device.pid or 0))

    if not wait_until_openable(Serial, device.device, SETTLE_TIMEOUT):
        print("Warnung: Port %s wurde nicht stabil" % device.device)
    return device.device


def wait_for_device(list_ports, timeout):
    """Wartet auf das Board - nach einem USB-Abbruch braucht es einen Moment."""
    elapsed = 0.0
    while elapsed < timeout:
        device = find_device(list_ports)
        if device is not None:
            return device
        sleep(0.25)
        elapsed += 0.25
    return None


def wait_for_bootloader(list_ports, timeout):
    elapsed = 0.0
    while elapsed < timeout:
        device = find_device(list_ports)
        if device is not None and is_bootloader(device):
            return device
        sleep(POLL)
        elapsed += POLL
    return None


def wait_for_reset(Serial, list_ports, timeout):
    """Fordert einen Tasterdruck an und faengt den Bootloader-Start ab."""
    print("")
    print(">>> Bitte jetzt den Reset-Taster antippen (oder USB kurz abziehen "
          "und wieder anstecken). <<<")
    print("    Es wird sofort weitergeflasht, sobald der Bootloader "
          "hochkommt.")

    elapsed = 0.0
    while elapsed < timeout:
        if find_device(list_ports) is None:
            break
        sleep(POLL)
        elapsed += POLL
    else:
        die("kein Reset erkannt")

    device = wait_for_bootloader(list_ports, DFU_TIMEOUT)
    if device is None:
        die("Board ist nach dem Reset nicht im Bootloader aufgetaucht")

    print("Bootloader da: %s (PID 0x%04X)"
          % (device.device, device.pid or 0))
    wait_until_openable(Serial, device.device, SETTLE_TIMEOUT)
    return device.device


def wait_for_application(list_ports, timeout):
    """Prueft, ob der Bootloader nach dem Flashen die neue App startet."""
    elapsed = 0.0
    while elapsed < timeout:
        device = find_device(list_ports)
        if device is not None and not is_bootloader(device):
            return True
        sleep(0.5)
        elapsed += 0.5
    return False


def wait_until_openable(Serial, port_name, timeout):
    """Wartet, bis der Port sich oeffnen laesst - so kurz wie moeglich."""
    elapsed = 0.0
    while elapsed < timeout:
        try:
            Serial(port=port_name, baudrate=DFU_BAUD).close()
            return True
        except Exception:
            sleep(POLL)
            elapsed += POLL
    return False


def bootloader_restarted(list_ports, timeout):
    """Prueft, ob das Board nach einem Fehlschlag vom Bus verschwindet.

    Nur dann startet der Bootloader neu und ein weiterer Versuch hat Sinn -
    ein haengender Bootloader bleibt sichtbar und antwortet trotzdem nie
    wieder.
    """
    elapsed = 0.0
    while elapsed < timeout:
        if find_device(list_ports) is None:
            return True
        sleep(0.25)
        elapsed += 0.25
    return False


def flash_once(package, port):
    from nordicsemi.dfu.dfu import Dfu
    from nordicsemi.dfu.dfu_transport import DfuEvent
    from nordicsemi.dfu.dfu_transport_serial import DfuTransportSerial

    DfuTransportSerial.ACK_PACKET_TIMEOUT = ACK_TIMEOUT

    sent = {"packets": 0}

    def on_progress(progress=0, done=False, log_message=""):
        sent["packets"] = progress
        sys.stdout.write("#")
        sys.stdout.flush()

    def on_timeout(**kwargs):
        # Nicht weitersenden: nrfutil wuerde mit zurueckgesetzter
        # Sequenznummer weiterfunken und den Bootloader zum Abbruch bringen.
        raise RuntimeError("ACK nach %.1f s ausgeblieben (bei Paket %d)"
                           % (ACK_TIMEOUT, sent["packets"]))

    backend = DfuTransportSerial(port, DFU_BAUD, False, True, 0)
    backend.register_events_callback(DfuEvent.PROGRESS_EVENT, on_progress)
    backend.register_events_callback(DfuEvent.TIMEOUT_EVENT, on_timeout)

    Dfu(package, dfu_transport=backend).dfu_send_images()
    sys.stdout.write("\n")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-pkg", "--package", required=True)
    parser.add_argument("-p", "--port", default=None)
    args = parser.parse_args()

    if not os.path.isfile(args.package):
        die("DFU-Paket %s nicht gefunden" % args.package)

    load_nrfutil()
    from serial import Serial
    from serial.tools import list_ports

    port = enter_dfu(Serial, list_ports, args.port)

    for attempt in range(1, ATTEMPTS + 1):
        print("Flashe %s -> %s (Versuch %d/%d)"
              % (os.path.basename(args.package), port, attempt, ATTEMPTS))
        try:
            flash_once(args.package, port)
        except Exception as exc:
            print("\nVersuch %d fehlgeschlagen: %s" % (attempt, exc))
            if attempt == ATTEMPTS:
                die("Upload nach %d Versuchen fehlgeschlagen" % ATTEMPTS)
            if bootloader_restarted(list_ports, RESTART_TIMEOUT):
                # Der Abbruch hat den Bootloader neu gestartet: sein
                # DFU-Fenster ist gerade offen, also sofort nochmal.
                print("Bootloader hat sich neu gestartet - sofort nochmal.")
                device = wait_for_bootloader(list_ports, DFU_TIMEOUT)
                if device is None:
                    die("Board ist nicht wieder aufgetaucht")
                port = device.device
                wait_until_openable(Serial, port, SETTLE_TIMEOUT)
            else:
                port = wait_for_reset(Serial, list_ports, RESET_WAIT)
            continue

        print("Firmware geflasht.")
        if wait_for_application(list_ports, APP_BOOT_TIMEOUT):
            print("Board laeuft mit der neuen Firmware.")
        else:
            print("Hinweis: Board haengt noch im Bootloader - Reset-Taster "
                  "einmal antippen, dann startet die neue Firmware.")
        return 0

    return 1


if __name__ == "__main__":
    sys.exit(main())
