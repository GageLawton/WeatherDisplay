#!/bin/bash

SERVICE_NAME="weather-display.service"
SERVICE_PATH="./$SERVICE_NAME"
SYSTEMD_DIR="/etc/systemd/system"

# Exit if service file does not exist
if [ ! -f "$SERVICE_PATH" ]; then
    echo "❌ Service file $SERVICE_NAME not found in current directory."
    exit 1
fi

# Copy to systemd directory
echo "📁 Copying $SERVICE_NAME to $SYSTEMD_DIR..."
sudo cp "$SERVICE_PATH" "$SYSTEMD_DIR"

# Reload systemd
echo "🔄 Reloading systemd..."
sudo systemctl daemon-reexec
sudo systemctl daemon-reload

# Enable service
echo "✅ Enabling weather-display service to run on boot..."
sudo systemctl enable weather-display.service

# Start service
echo "🚀 Starting weather-display service..."
sudo systemctl start weather-display.service

# Show service status
echo "📋 Showing service status:"
sudo systemctl status weather-display.service --no-pager

echo "✅ Installation complete."
