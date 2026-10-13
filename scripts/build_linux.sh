#!/bin/bash
set -euo pipefail

echo "============================================"
echo "Building FPS Overlay for Linux"
echo "============================================"

# Resolve paths relative to this script's location so the build works
# regardless of the directory it's invoked from.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

BUILD_DIR="$ROOT_DIR/build/linux"
RELEASE_DIR="$ROOT_DIR/release/linux"
VERSION="1.0.0"

mkdir -p "$BUILD_DIR"
mkdir -p "$RELEASE_DIR"

echo "[1/6] Compiling overlay..."
# Note: intentionally NOT using -static here. Fully static-linking against
# X11/glibc is unreliable on most distros (static X11 libs usually aren't
# installed, and statically-linked glibc has known issues with NSS/DNS).
g++ -std=c++17 -O2 \
    "$ROOT_DIR/src/linux/overlay.cpp" \
    -o "$BUILD_DIR/fps-overlay" \
    -lX11 -lXext -pthread

echo "[2/6] Compiling settings core..."
g++ -std=c++17 -O2 -shared -fPIC \
    "$ROOT_DIR/src/linux/settings_core.cpp" \
    -o "$BUILD_DIR/libsettings_core.so"

echo "[3/6] Copying Python settings..."
cp "$ROOT_DIR/src/linux/settings.py" "$BUILD_DIR/fps-overlay-settings"
chmod +x "$BUILD_DIR/fps-overlay-settings"

echo "[4/6] Copying Makefile..."
cp "$ROOT_DIR/src/linux/Makefile" "$BUILD_DIR/Makefile"

echo "[5/6] Creating .desktop file..."
cat > "$BUILD_DIR/fps-overlay.desktop" << EOF
[Desktop Entry]
Name=FPS Overlay
Comment=System monitoring overlay
Exec=fps-overlay
Type=Application
Categories=Utility;System;
Terminal=false
EOF

echo "[6/6] Creating release package..."
tar -czf "$RELEASE_DIR/FPS-Overlay-Linux-v${VERSION}.tar.gz" \
    -C "$BUILD_DIR" .

echo ""
echo "Build complete!"
echo "Release: $RELEASE_DIR/FPS-Overlay-Linux-v${VERSION}.tar.gz"
