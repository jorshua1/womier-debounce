#!/usr/bin/env python3
"""
Womier V-K66 RGB Controller - Cross-Platform (Linux & Windows) CLI
Reverse-engineered from USB wire captures and OEM Shinetek drivers.

Device: Womier V-K66 (China Resource Semico Co., Ltd.)
VID: 0x1A2C, PID: 0x9206
Protocol: HID Feature Report (Endpoint 0, Report ID 7)
Payload: [0x07, 0xFF, 0xFF, MODE, SPEED, BRIGHTNESS, 0x00, 0x00]
"""

import os
import glob
import sys
import platform

VENDOR_ID = 0x1A2C
PRODUCT_ID = 0x9206

MODES = {
    0: ("steady", "Fijo / Estático (Steady)"),
    1: ("custom", "Modo personalizado (Custom)"),
    2: ("breathing", "Respiración (Breathing)"),
    3: ("press_destroy", "Reactivo (Press & destroy)"),
    4: ("neon_stream", "Onda Neón (Neon stream)"),
    5: ("streamer", "Transmitiendo (Streamer)"),
    6: ("flowing", "Flujo de color (Flowing light)"),
    7: ("dripping", "Goteo y ondas (Dripping ripples)"),
    8: ("reactive", "Reactivo puro (Brilliant point - Oscuro en reposo)"),
    9: ("flash_away", "Destello expansivo (Flash away)"),
    10: ("shadow", "Sombra al pulsar (Shadow disappear)"),
    11: ("ripples", "Ondas brillantes (Ripples shining)"),
    12: ("rich", "Efecto Rich and honored"),
    13: ("marquee", "Marquesina (Marquee effect)"),
    14: ("storm", "Tormenta giratoria (Rotating storm)"),
    15: ("serpentine", "Carrera serpenteante (Serpentine)"),
    16: ("stars", "Estrellas titilantes (Stars twinkle)"),
    17: ("snake", "Víbora retro (Retro snake)"),
    18: ("diagonal", "Transformación diagonal"),
    19: ("sine", "Onda senoidal (Sine wave)"),
}

def get_hidioc_sfeature_num(length):
    IOC_NRBITS = 8
    IOC_TYPEBITS = 8
    IOC_SIZEBITS = 14
    IOC_DIRBITS = 2

    IOC_NRSHIFT = 0
    IOC_TYPESHIFT = IOC_NRSHIFT + IOC_NRBITS
    IOC_SIZESHIFT = IOC_TYPESHIFT + IOC_TYPEBITS
    IOC_DIRSHIFT = IOC_SIZESHIFT + IOC_SIZEBITS

    IOC_WRITE = 1
    IOC_READ = 2

    return ((IOC_WRITE | IOC_READ) << IOC_DIRSHIFT) | \
           (ord('H') << IOC_TYPESHIFT) | \
           (0x06 << IOC_NRSHIFT) | \
           (length << IOC_SIZESHIFT)

def find_linux_hidraw():
    for hidraw in sorted(glob.glob('/sys/class/hidraw/hidraw*')):
        uevent_file = os.path.join(hidraw, 'device', 'uevent')
        if os.path.exists(uevent_file):
            with open(uevent_file, 'r') as f:
                content = f.read()
            if f'{VENDOR_ID:04X}' in content and f'{PRODUCT_ID:04X}' in content:
                rdesc_file = os.path.join(hidraw, 'device', 'report_descriptor')
                if os.path.exists(rdesc_file):
                    with open(rdesc_file, 'rb') as f:
                        rdesc = f.read()
                    if b'\x06\x01\xff' in rdesc:
                        return f'/dev/{os.path.basename(hidraw)}'
    return None

def send_feature_report(payload):
    system = platform.system()

    if system == "Linux":
        import fcntl
        node = find_linux_hidraw()
        if not node:
            print("[!] Error: No se encontró el nodo Womier V-K66 en /dev/hidraw*")
            sys.exit(1)

        req = get_hidioc_sfeature_num(len(payload))
        try:
            with open(node, 'rb+', buffering=0) as f:
                fcntl.ioctl(f.fileno(), req, payload)
            return node
        except PermissionError:
            print(f"[!] Permiso denegado para abrir {node}.")
            print("[*] Ejecuta con sudo o instala la regla udev: ./womier_ctl.py setup-udev")
            sys.exit(1)

    elif system == "Windows":
        try:
            import hid
        except ImportError:
            print("[!] En Windows se requiere la librería 'hidapi'.")
            print("[*] Instálala ejecutando: pip install hidapi")
            sys.exit(1)

        device = None
        for dev_info in hid.enumerate(VENDOR_ID, PRODUCT_ID):
            # Interface 1 or UsagePage 0xFF01
            if dev_info.get('usage_page') == 0xFF01 or dev_info.get('interface_number') == 1:
                device = hid.device()
                device.open_path(dev_info['path'])
                break

        if not device:
            # Fallback to direct VID/PID
            device = hid.device()
            try:
                device.open(VENDOR_ID, PRODUCT_ID)
            except Exception as e:
                print(f"[!] Error abriendo teclado Womier en Windows: {e}")
                sys.exit(1)

        device.send_feature_report(payload)
        device.close()
        return "Windows HID Device"

    else:
        print(f"[!] Sistema operativo no soportado: {system}")
        sys.exit(1)

def send_rgb(mode=5, speed=4, brightness=3):
    payload = bytes([0x07, 0xFF, 0xFF, mode & 0xFF, speed & 0xFF, brightness & 0xFF, 0x00, 0x00])
    target = send_feature_report(payload)
    name = MODES.get(mode, ("desconocido", "Modo personalizado"))[1]
    print(f"[✓] Dispositivo: {target}")
    print(f"[✓] Modo {mode}: {name}")
    print(f"[✓] Brillo: {brightness}/4 | Velocidad: {speed}/5")

def print_modes():
    print("Modos de Iluminación Disponibles en Womier V-K66:")
    print("=" * 65)
    for idx, (alias, desc) in sorted(MODES.items()):
        print(f"  {idx:2d} | {alias:15s} | {desc}")
    print("=" * 65)

def resolve_mode(val):
    val = val.lower()
    if val.isdigit():
        idx = int(val)
        if idx in MODES:
            return idx
    for idx, (alias, _) in MODES.items():
        if val in alias:
            return idx
    return None

def main():
    if len(sys.argv) < 2:
        print("Uso:")
        print(f"  {sys.argv[0]} list                         (Lista todos los 20 modos)")
        print(f"  {sys.argv[0]} <modo|número> [brillo] [vel] (Ej: storm, shadow, 8, reactive)")
        print(f"  {sys.argv[0]} brightness <0-4>             (Cambia solo el brillo)")
        print(f"  {sys.argv[0]} speed <1-5>                  (Cambia la velocidad)")
        if platform.system() == "Linux":
            print(f"  {sys.argv[0]} setup-udev                   (Instala regla udev para no usar sudo)")
        return

    cmd = sys.argv[1].lower()

    if cmd == "list":
        print_modes()
        return

    if cmd == "setup-udev" and platform.system() == "Linux":
        rule_path = "/etc/udev/rules.d/99-womier.rules"
        rule_content = 'SUBSYSTEM=="hidraw", ATTRS{idVendor}=="1a2c", ATTRS{idProduct}=="9206", MODE="0666", TAG+="uaccess"\n'
        print("[*] Para instalar la regla udev ejecuta:")
        print(f"    echo '{rule_content.strip()}' | sudo tee {rule_path}")
        print("    sudo udevadm control --reload-rules && sudo udevadm trigger")
        return

    if cmd in ("brightness", "brillo") and len(sys.argv) > 2:
        b = max(0, min(4, int(sys.argv[2])))
        send_rgb(mode=5, speed=4, brightness=b)
        return

    if cmd in ("speed", "velocidad") and len(sys.argv) > 2:
        s = max(1, min(5, int(sys.argv[2])))
        send_rgb(mode=5, speed=s, brightness=3)
        return

    target_arg = sys.argv[2] if cmd == "mode" and len(sys.argv) > 2 else cmd
    arg_offset = 2 if cmd == "mode" else 1

    mode_idx = resolve_mode(target_arg)
    if mode_idx is not None:
        b = int(sys.argv[arg_offset + 1]) if len(sys.argv) > arg_offset + 1 else 3
        s = int(sys.argv[arg_offset + 2]) if len(sys.argv) > arg_offset + 2 else 4
        send_rgb(mode=mode_idx, speed=s, brightness=b)
        return

    print(f"[!] Modo o comando no reconocido: '{cmd}'.")
    print(f"[*] Usa '{sys.argv[0]} list' para ver todos los nombres disponibles.")

if __name__ == '__main__':
    main()
