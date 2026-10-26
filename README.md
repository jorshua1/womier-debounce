# Womier V-K66 Linux & Windows Toolkit

Cross-platform low-latency mechanical switch debounce filter and native RGB lighting controller for the **Womier V-K66** mechanical keyboard (China Resource Semico Co., Ltd., `1A2C:9206`).

---

## Features

1. **Low-Latency Debounce Filter (Linux & Windows)**:
   * Eliminates key chatter (double typing, especially on `Enter`) caused by aging or noisy mechanical switches.
   * **Zero latency on key press**: Emits `KeyDown` instantly upon contact.
   * **Modifier-safe**: Whitelists `Shift`, `Alt`, `Ctrl`, `Meta`, `Tab`, and `CapsLock` to avoid dropping keys or breaking shortcuts like `Shift + Enter` or `Alt + Tab`.
   * **Micro-footprint**: Consumes under 400 KB of RAM with 0% CPU utilization.

2. **Native RGB Lighting Control (No OEM Software Needed)**:
   * Controls lighting directly via USB HID Feature Reports without opening heavy proprietary software.
   * Fully supports all 20 factory lighting modes, speed control, and brightness levels.
   * Provides a "true dark / stealth" mode (`reactive`) to keep the keyboard dark during idle.

---

## Repository Structure

```
├── womier_ctl.py           # Cross-platform (Linux & Windows) RGB control CLI
├── linux/                  # Linux debounce daemon and udev configuration
│   ├── womier-debounce.c   # Low-latency evdev/uinput daemon
│   ├── womier-debounce.service # systemd background service
│   ├── 99-womier.rules     # udev rule for non-root USB HID access
│   └── Makefile            # Build and installation script
├── windows/                # Windows debounce utility
│   ├── womier-debounce.c   # Lightweight Win32 low-level keyboard hook
│   └── build.bat           # MinGW / MSVC build script
└── docs/
    └── protocol.md         # Reverse-engineered USB protocol specification
```

---

## 1. RGB Lighting Control (`womier_ctl.py`)

### Linux Setup

1. Install the udev rule to allow non-root access:
   ```bash
   ./womier_ctl.py setup-udev
   ```
   *(Or copy `linux/99-womier.rules` to `/etc/udev/rules.d/` and run `sudo udevadm control --reload-rules && sudo udevadm trigger`).*

2. Usage:
   ```bash
   # List all 20 animation modes
   ./womier_ctl.py list

   # Switch animation by name or number
   ./womier_ctl.py storm
   ./womier_ctl.py shadow
   ./womier_ctl.py neon_stream
   ./womier_ctl.py 5

   # Set keyboard to dark in idle (lights only pressed keys)
   ./womier_ctl.py reactive

   # Adjust brightness (0 to 4) and speed (1 to 5)
   ./womier_ctl.py brightness 2
   ./womier_ctl.py speed 3
   ```

### Windows Setup

1. Install the Python HID library:
   ```cmd
   pip install hidapi
   ```
2. Run `womier_ctl.py` in Command Prompt or PowerShell:
   ```cmd
   python womier_ctl.py storm
   python womier_ctl.py reactive
   python womier_ctl.py list
   ```

---

## 2. Mechanical Switch Debounce Daemon

### Linux (Systemd Service)

#### Building & Installing:
```bash
cd linux
make
sudo make install
sudo systemctl enable --now womier-debounce.service
```

#### Verifying:
```bash
systemctl status womier-debounce.service
journalctl -u womier-debounce.service -f
```

---

### Windows (Low-Level Hook)

#### Building:
Run `windows/build.bat` using GCC (MinGW-w64) or Visual Studio Command Prompt (`cl.exe`).

Or compile manually with GCC:
```cmd
gcc -O2 -Wall windows/womier-debounce.c -o windows/womier-debounce.exe -mwindows
```

#### Running at Startup:
1. Compile `womier-debounce.exe`.
2. Press `Win + R`, type `shell:startup`, and press `Enter`.
3. Create a shortcut to `womier-debounce.exe` in that folder.
4. The utility runs silently in the background without opening a terminal window.

---

## Technical Details

For complete USB transfer parameters, report structures, and byte offsets, see [docs/protocol.md](docs/protocol.md).

## License

MIT License.
