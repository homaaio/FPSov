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

### Method 1: Pre-built Binaries (Recommended)
1. Download the latest release from the [Releases](../../releases) section
2. Extract the archive to a folder of your choice
3. Run `overlay.exe`

### Method 2: Build from Source

#### Prerequisites
- MSVC (Visual Studio 2022) or MinGW-w64 (GCC)
- Python 3.7+ (for settings utility)
- Windows SDK

#### Compilation

Using MSVC (Developer Command Prompt):
```cmd
cl /O2 /EHsc overlay.cpp /Fe:overlay.exe user32.lib gdi32.lib pdh.lib psapi.lib dwmapi.lib