#!/bin/bash
# Installation script for Linux (systemd)

set -e

SERVICE_NAME="webrtc-remote-mgmt"
SERVICE_FILE="webrtc-remote-mgmt.service"
BUILD_DIR="../build"
INSTALL_DIR="/usr/local/bin"
SYSTEMD_DIR="/etc/systemd/system"

echo "Installing WebRTC Remote Management Service for Linux..."

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

# Copy service file
echo "Installing systemd service..."
cp "$SERVICE_FILE" "$SYSTEMD_DIR/"

# Reload systemd
echo "Reloading systemd..."
systemctl daemon-reload

# Enable service
echo "Enabling service..."
systemctl enable "$SERVICE_NAME"

echo ""
echo "Installation complete!"
echo ""
echo "To start the service:"
echo "  sudo systemctl start $SERVICE_NAME"
echo ""
echo "To check status:"
echo "  sudo systemctl status $SERVICE_NAME"
echo ""
echo "To view logs:"
echo "  sudo journalctl -u $SERVICE_NAME -f"
