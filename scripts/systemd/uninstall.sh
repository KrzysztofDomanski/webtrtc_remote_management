#!/bin/bash
# Uninstallation script for Linux (systemd)

set -e

SERVICE_NAME="webrtc-remote-mgmt"
SERVICE_FILE="webrtc-remote-mgmt.service"
INSTALL_DIR="/usr/local/bin"
SYSTEMD_DIR="/etc/systemd/system"

echo "Uninstalling WebRTC Remote Management Service for Linux..."

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "Please run as root (use sudo)"
    exit 1
fi

# Stop service if running
echo "Stopping service..."
systemctl stop "$SERVICE_NAME" || true

# Disable service
echo "Disabling service..."
systemctl disable "$SERVICE_NAME" || true

# Remove service file
echo "Removing systemd service..."
rm -f "$SYSTEMD_DIR/$SERVICE_FILE"

# Remove executable
echo "Removing executable..."
rm -f "$INSTALL_DIR/webrtc_remote_mgmt"

# Reload systemd
echo "Reloading systemd..."
systemctl daemon-reload

echo ""
echo "Uninstallation complete!"
