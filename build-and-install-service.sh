#!/bin/bash
#
# Author: Gage Lawton
# Updated: 2025-10-17
# Description: Build WeatherDisplay (LCD + OLED) and install it as a systemd service using local ssd1306 library.

set -euo pipefail

# Resolve script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Paths
BINARY_PATH="$SCRIPT_DIR/weather"
SERVICE_NAME="weather-display.service"
SERVICE_PATH="$SCRIPT_DIR/systemd/$SERVICE_NAME"
SYSTEMD_DIR="/etc/systemd/system"

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

info()    { echo -e "${BLUE}[INFO]${NC} $1"; }
success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
error()   { echo -e "${RED}[ERROR]${NC} $1"; }

# Check environment vars
if [ -z "${WEATHER_API_KEY:-}" ] || [ -z "${WEATHER_LOCATION:-}" ]; then
    warning "WEATHER_API_KEY or WEATHER_LOCATION not set. Service may fail to fetch weather."
fi

# Check for root permissions
if [ "$EUID" -ne 0 ]; then
    warning "You are not running as root. You may be prompted for your password."
fi

# Step 0: Install build dependencies
info "📦 Installing build dependencies (g++, curl, cmake, freetype)..."
sudo apt-get update
sudo apt-get install -y \
    g++ \
    git \
    make \
    cmake \
    libcurl4-openssl-dev \
    nlohmann-json3-dev \
    libi2c-dev \
    libfreetype6-dev \
    libfontconfig1-dev \
    libpng-dev

# Step 0b: Install WiringPi if missing
if ! command -v gpio &> /dev/null; then
    info "📦 Installing WiringPi manually..."
    git clone https://github.com/WiringPi/WiringPi.git /tmp/WiringPi
    cd /tmp/WiringPi
    ./build
    cd ~
    rm -rf /tmp/WiringPi
else
    info "✅ WiringPi already installed."
fi

# Step 1: Build WeatherDisplay binary (with local ssd1306 source)
info "🛠️ Compiling WeatherDisplay binary with local OLED support..."

SSD1306_SRC="$SCRIPT_DIR/include/external/ssd1306/src"

g++ -Wall -O2 -std=c++17 \
    -I"$SCRIPT_DIR/include" \
    -I"$SSD1306_SRC" \
    "$SCRIPT_DIR/main.cpp" \
    "$SCRIPT_DIR/config.cpp" \
    "$SCRIPT_DIR/lcd.cpp" \
    "$SCRIPT_DIR/weather.cpp" \
    "$SCRIPT_DIR/oled.cpp" \
    "$SSD1306_SRC/ssd1306_console.cpp" \
    "$SSD1306_SRC/ssd1306_fonts.cpp" \
    "$SSD1306_SRC/ssd1306_i2c.cpp" \
    "$SSD1306_SRC/ssd1306_oled.cpp" \
    -lwiringPi -lcurl -lpthread -o "$BINARY_PATH"

chmod +x "$BINARY_PATH"
success "✅ Binary compiled: $BINARY_PATH"

# Step 2: Check service file
if [ ! -f "$SERVICE_PATH" ]; then
    error "Service file not found: $SERVICE_PATH"
    exit 1
fi

# Step 3: Copy systemd service file
info "📁 Copying $SERVICE_NAME to $SYSTEMD_DIR..."
sudo cp "$SERVICE_PATH" "$SYSTEMD_DIR"
sudo chmod 644 "$SYSTEMD_DIR/$SERVICE_NAME"

# Step 4: Inject environment variables into systemd unit safely
info "🔧 Injecting environment variables into systemd unit..."
sudo sed -i "/^ExecStart=/i Environment=\"WEATHER_API_KEY=${WEATHER_API_KEY//&/\\&}\"" "$SYSTEMD_DIR/$SERVICE_NAME"
sudo sed -i "/^ExecStart=/i Environment=\"WEATHER_LOCATION=${WEATHER_LOCATION//&/\\&}\"" "$SYSTEMD_DIR/$SERVICE_NAME"

# Step 5: Reload systemd daemon & enable service on boot
info "🔄 Reloading systemd daemon..."
sudo systemctl daemon-reexec
sudo systemctl daemon-reload

info "✅ Enabling service to start on boot..."
sudo systemctl enable "$SERVICE_NAME"

# Step 6: Start or restart the service
info "🚀 Starting service..."
sudo systemctl restart "$SERVICE_NAME"

# Step 7: Show service status
info "📋 Service status:"
sudo systemctl status "$SERVICE_NAME" --no-pager

success "🎉 WeatherDisplay local build & install complete!"
