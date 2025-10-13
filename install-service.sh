#!/bin/bash
#
# Author: Gage Lawton
# Date Written: 2025-10-11
# Last Updated: 2025-10-12
# Description: Script to install and configure the WeatherDisplay systemd service
#              directly on the Raspberry Pi.

set -euo pipefail

SERVICE_NAME="weather-display.service"
SERVICE_PATH="./systemd/$SERVICE_NAME"
SYSTEMD_DIR="/etc/systemd/system"

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

function info() { echo -e "${BLUE}[INFO]${NC} $1"; }
function success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
function warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
function error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Check if running as root (necessary for systemctl and copying to /etc)
if [ "$EUID" -ne 0 ]; then
    warning "You are not running as root. You may be prompted for your password."
fi

# Verify service file exists
if [ ! -f "$SERVICE_PATH" ]; then
    error "Service file $SERVICE_NAME not found in $SERVICE_PATH."
    exit 1
fi

# Copy service file
info "Copying $SERVICE_NAME to $SYSTEMD_DIR..."
sudo cp "$SERVICE_PATH" "$SYSTEMD_DIR"

# Set proper permissions
sudo chmod 644 "$SYSTEMD_DIR/$SERVICE_NAME"

# Reload systemd
info "Reloading systemd daemon..."
sudo systemctl daemon-reexec
sudo systemctl daemon-reload

# Enable service to start at boot
info "Enabling $SERVICE_NAME to run on boot..."
sudo systemctl enable "$SERVICE_NAME"

# Start service
info "Starting $SERVICE_NAME..."
sudo systemctl start "$SERVICE_NAME"

# Show service status
info "Displaying $SERVICE_NAME status:"
sudo systemctl status "$SERVICE_NAME" --no-pager

success "🎉 WeatherDisplay service installation complete!"
