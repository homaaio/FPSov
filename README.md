# System Overlay for Windows

Lightweight system monitoring overlay for games and applications on Windows. Displays FPS, CPU and GPU usage in a transparent window above all other programs.

## Features

- Low resource usage - written in C++ using Win32 API
- Transparent interface - adjustable opacity and positioning
- Flexible configuration - via INI file or graphical utility (Python)
- Always on top - works above games and fullscreen applications
- High DPI support - correct display on any monitor
- Mouse dragging - can be moved anywhere on screen

## Displayed Metrics

- **FPS** - frame rate of the active window/game
- **CPU** - processor usage (percentage)
- **GPU** - graphics card usage (percentage)

## System Requirements

- Windows 10 / 11 (x64)
- CPU with PDH (Performance Data Helper) support
- GPU with DXGI support (DirectX 11/12)

## Installation
1. Download the latest release from the Releases section
2. Extract the archive to a folder of your choice
3. Run `overlay.exe`
