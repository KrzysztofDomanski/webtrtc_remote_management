#!/bin/bash
# Uninstallation script for macOS (launchd)

set -e

SERVICE_NAME="com.webrtc.remotemgmt"
PLIST_FILE="com.webrtc.remotemgmt.plist"
INSTALL_DIR="/usr/local/bin"
LAUNCHD_DIR="/Library/LaunchDaemons"

echo "Uninstalling WebRTC Remote Management Service for macOS..."

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "Please run as root (use sudo)"
    exit 1
fi

# Unload service
echo "Unloading service..."
launchctl unload "$LAUNCHD_DIR/$PLIST_FILE" || true

# Remove plist file
echo "Removing launchd service..."
rm -f "$LAUNCHD_DIR/$PLIST_FILE"

# Remove executable
echo "Removing executable..."
rm -f "$INSTALL_DIR/webrtc_remote_mgmt"

echo ""
echo "Uninstallation complete!"
