#!/bin/bash
#
# Author: Gage Lawton
# Date Written: 2025-10-12
# Description: Build WeatherDisplay and install it as a systemd service directly on the Raspberry Pi.

set -euo pipefail

# Paths
BINARY_PATH="./weather"
SOURCE_FILES=("main.cpp" "weather.cpp" "lcd.cpp" "config.cpp")
SERVICE_NAME="weather-display.service"
SERVICE_PATH="./systemd/$SERVICE_NAME"
SYSTEMD_DIR="/etc/systemd/system"

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

function info() { echo -e "${BLUE}[INFO]${NC} $1"; }
function success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
function warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
function error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Ensure required environment variables exist
if [ -z "${WEATHER_API_KEY:-}" ] || [ -z "${WEATHER_LOCATION:-}" ]; then
    warning "WEATHER_API_KEY or WEATHER_LOCATION not set. Service may fail to fetch weather."
fi

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    warning "You are not running as root. You may be prompted for your password."
fi

# Step 1: Build binary
info "🛠️ Compiling WeatherDisplay binary..."
g++ -Wall -O2 -std=c++11 -I./include "${SOURCE_FILES[@]}" -lwiringPi -lcurl -o "$BINARY_PATH"
chmod +x "$BINARY_PATH"
success "✅ Binary compiled successfully: $BINARY_PATH"

# Step 2: Verify service file exists
if [ ! -f "$SERVICE_PATH" ]; then
    error "Service file $SERVICE_NAME not found in $SERVICE_PATH."
    exit 1
fi

# Step 3: Copy service file
info "📁 Copying $SERVICE_NAME to $SYSTEMD_DIR..."
sudo cp "$SERVICE_PATH" "$SYSTEMD_DIR"
sudo chmod 644 "$SYSTEMD_DIR/$SERVICE_NAME"

# Step 4: Inject environment variables
info "🔧 Injecting environment variables into service..."
sudo sed -i "/^ExecStart=/i Environment=\"WEATHER_API_KEY=${WEATHER_API_KEY}\"" "$SYSTEMD_DIR/$SERVICE_NAME"
sudo sed -i "/^ExecStart=/i Environment=\"WEATHER_LOCATION=${WEATHER_LOCATION}\"" "$SYSTEMD_DIR/$SERVICE_NAME"

# Step 5: Reload systemd and enable service
info "🔄 Reloading systemd daemon..."
sudo systemctl daemon-reexec
sudo systemctl daemon-reload
info "✅ Enabling service to start on boot..."
sudo systemctl enable "$SERVICE_NAME"

# Step 6: Start/restart service
info "🚀 Starting $SERVICE_NAME..."
sudo systemctl restart "$SERVICE_NAME"

# Step 7: Show service status
info "📋 Service status:"
sudo systemctl status "$SERVICE_NAME" --no-pager

success "🎉 WeatherDisplay build and service installation complete!"
