#!/bin/bash
# Installation script for macOS (launchd)

set -e

SERVICE_NAME="com.webrtc.remotemgmt"
PLIST_FILE="com.webrtc.remotemgmt.plist"
BUILD_DIR="../../build"
INSTALL_DIR="/usr/local/bin"
LAUNCHD_DIR="/Library/LaunchDaemons"

echo "Installing WebRTC Remote Management Service for macOS..."

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "Please run as root (use sudo)"
    exit 1
fi

# Build the project if not already built
if [ ! -f "$BUILD_DIR/src/webrtc_remote_mgmt" ]; then
    echo "Building project..."
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    cmake ..
    cmake --build .
    cd -
fi

# Copy executable
echo "Installing executable to $INSTALL_DIR..."
cp "$BUILD_DIR/src/webrtc_remote_mgmt" "$INSTALL_DIR/"
chmod +x "$INSTALL_DIR/webrtc_remote_mgmt"

# Copy plist file
echo "Installing launchd service..."
cp "$PLIST_FILE" "$LAUNCHD_DIR/"

# Load service
echo "Loading service..."
launchctl load "$LAUNCHD_DIR/$PLIST_FILE"

echo ""
echo "Installation complete!"
echo ""
echo "To check status:"
echo "  sudo launchctl list | grep $SERVICE_NAME"
echo ""
echo "To view logs:"
echo "  tail -f /var/log/webrtc-remote-mgmt.log"
