# FPS Overlay for Linux

System monitoring overlay displaying FPS, CPU, and GPU usage.

## Features
- Real-time FPS detection via XRandR
- CPU usage from /proc/stat
- GPU usage (NVIDIA/AMD support)
- Customizable through INI file
- Draggable window
- Low resource usage
- No external dependencies beyond X11

## Building
```bash
cd linux
make
