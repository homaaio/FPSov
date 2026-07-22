#!/bin/bash
echo "============================================"
echo "Building FPS Overlay for Linux"
echo "============================================"

BUILD_DIR="../build/linux"
RELEASE_DIR="../release/linux"
VERSION="1.0.0"

mkdir -p "$BUILD_DIR"
mkdir -p "$RELEASE_DIR"

echo "[1/5] Compiling overlay..."
g++ -std=c++17 -O2 -static \
    src/linux/overlay.cpp \
    -o "$BUILD_DIR/fps-overlay" \
    -lX11 -lXext -pthread

echo "[2/5] Compiling settings core..."
g++ -std=c++17 -O2 -shared -fPIC \
    src/linux/settings_core.cpp \
    -o "$BUILD_DIR/libsettings_core.so"

echo "[3/5] Copying Python settings..."
cp src/linux/settings.py "$BUILD_DIR/fps-overlay-settings"
chmod +x "$BUILD_DIR/fps-overlay-settings"

echo "[4/5] Creating .desktop file..."
cat > "$BUILD_DIR/fps-overlay.desktop" << EOF
[Desktop Entry]
Name=FPS Overlay
Comment=System monitoring overlay
Exec=fps-overlay
Type=Application
Categories=Utility;System;
Terminal=false
EOF

echo "[5/5] Creating release package..."
tar -czf "$RELEASE_DIR/FPS-Overlay-Linux-v${VERSION}.tar.gz" \
    -C "$BUILD_DIR" .

# linuxdeploy --appdir AppDir --output appimage

echo ""
echo "Build complete!"
echo "Release: $RELEASE_DIR/FPS-Overlay-Linux-v${VERSION}.tar.gz"
