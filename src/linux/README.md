# FPS Overlay for Linux

System monitoring overlay displaying FPS, CPU, and GPU usage.

## Features
- Real-time FPS detection via XRandR
- CPU usage from /proc/stat
- GPU usage (AMD/Intel via sysfs, NVIDIA via `nvidia-smi`)
- Customizable through INI file or graphical utility (Python/Tk)
- Draggable, borderless, always-on-top window
- Low resource usage
- No external dependencies beyond X11

## Dependencies
- A C++17 compiler (g++)
- X11 development headers: `libx11-dev libxext-dev` (Debian/Ubuntu) or
  `libX11-devel libXext-devel` (Fedora)
- `python3` and `python3-tk` if you want to use the graphical settings tool

Debian/Ubuntu:
```bash
sudo apt-get install build-essential libx11-dev libxext-dev python3 python3-tk
```

## Building
From this directory (`src/linux`):
```bash
make
```
This builds `fps-overlay` and `libsettings_core.so`.

Alternatively, build everything (Windows + Linux release archives) from the
repository root with:
```bash
./scripts/build_linux.sh
```

## Running
```bash
./fps-overlay
```
A default `overlay.ini` is created next to the binary on first run. Edit it
directly, or use the graphical settings tool:
```bash
python3 settings.py
```

## Uninstall (if installed via `make install`)
```bash
sudo make uninstall
```
