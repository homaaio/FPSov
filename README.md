# FPSov - System Overlay for Windows/Linux

Lightweight system monitoring overlay for games and applications on Windows and Linux. Displays FPS, CPU and GPU usage in a transparent window above all other programs.

## Features

- Low resource usage - native C++ (Win32 API on Windows, X11/Xlib on Linux)
- Transparent interface - adjustable opacity and positioning
- Flexible configuration - via INI file or graphical utility (Python)
- Always on top - works above games and fullscreen applications
- High DPI support - correct display on any monitor
- Mouse dragging - can be moved anywhere on screen

## Displayed Metrics

- **FPS** - refresh rate / frame rate estimate for the active window
- **CPU** - processor usage (percentage)
- **GPU** - graphics card usage (percentage)

## System Requirements

### Windows
- Windows 10 / 11 (x64)
- CPU with PDH (Performance Data Helper) support
- GPU with DXGI support (DirectX 11/12)

### Linux
- X11 (Xorg or Xwayland)
- `libX11`, `libXext`
- CPU usage via `/proc/stat`; GPU usage via sysfs (AMD/Intel) or `nvidia-smi` (NVIDIA), if available
- `python3-tk` for the optional graphical settings tool

## Installation

### Windows
1. Download the latest release from the Releases section
2. Extract the archive to a folder of your choice
3. Run `fps-overlay.exe`

### Linux
1. Download the latest release from the Releases section, or build from
   source (see `src/linux/README.md`)
2. Extract the archive to a folder of your choice
3. Run `./fps-overlay`

Both platforms create a default `overlay.ini` next to the executable on
first run, and can be customized either by editing that file directly or
through the included Python/Tk settings GUI.

## Building from source

- Windows: `scripts\build_windows.bat`
- Linux: `scripts/build_linux.sh` (or `cd src/linux && make`)
