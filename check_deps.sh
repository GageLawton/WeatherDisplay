#!/bin/bash
#
# Author: Gage Lawton
# Updated: 2025-10-17
# Description: Check and install dependencies for WeatherDisplay project.

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
info "📦 Installing build dependencies (g++, curl, cmake, freetype, Adafruit_GFX)..."
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
    libpng-dev \
    libgpiod-dev  # Ensure GPIO headers are available

# Install Adafruit_GFX library manually
info "📦 Installing Adafruit_GFX library manually..."

# Define the path where Adafruit GFX library should be located
ADAFRUIT_GFX_PATH="$SCRIPT_DIR/include/external/Adafruit_GFX"

# Check if the Adafruit GFX directory exists and contains the necessary files
if [ ! -d "$ADAFRUIT_GFX_PATH" ] || [ ! -f "$ADAFRUIT_GFX_PATH/Adafruit_GFX.h" ]; then
    info "📥 Cloning Adafruit_GFX Library..."
    git clone https://github.com/adafruit/Adafruit-GFX-Library.git /tmp/Adafruit-GFX

    # Copy the necessary files into your project
    mkdir -p "$ADAFRUIT_GFX_PATH"
    cp /tmp/Adafruit-GFX/Adafruit_GFX.h "$ADAFRUIT_GFX_PATH/Adafruit_GFX.h"
    cp /tmp/Adafruit-GFX/Adafruit_GFX.cpp "$SCRIPT_DIR/src/Adafruit_GFX.cpp"

    # Clean up the temporary directory
    rm -rf /tmp/Adafruit-GFX
else
    info "✅ Adafruit_GFX library already installed and up to date."
fi

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

# Step 1: Fetch SSD1306_OLED_RPI Library
info "📦 Fetching SSD1306_OLED_RPI library..."
SSD1306_SRC="$SCRIPT_DIR/include/external/ssd1306_oled_rpi"

# If the SSD1306_OLED_RPI directory doesn't exist, clone it
if [ ! -d "$SSD1306_SRC" ]; then
    info "📥 Cloning SSD1306_OLED_RPI library from GitHub..."
    mkdir -p "$SSD1306_SRC"
    git clone https://github.com/gavinlyonsrepo/SSD1306_OLED_RPI.git "$SSD1306_SRC"
else
    info "✅ SSD1306_OLED_RPI library already present."
fi

# Step 2: Compile WeatherDisplay binary with local OLED support
info "🛠️ Compiling WeatherDisplay binary with local OLED support..."

g++ -Wall -O2 -std=c++17 \
    -I"$SCRIPT_DIR/include" \
    -I"$SCRIPT_DIR/include/external/ssd1306_oled_rpi" \
    -I"$SCRIPT_DIR/include/external/Adafruit_GFX" \
    -I/usr/include/nlohmann \
    "$SCRIPT_DIR/src/main.cpp" \
    "$SCRIPT_DIR/src/config.cpp" \
    "$SCRIPT_DIR/src/lcd.cpp" \
    "$SCRIPT_DIR/src/weather.cpp" \
    "$SCRIPT_DIR/src/oled.cpp" \
    "$SCRIPT_DIR/include/external/ssd1306_oled_rpi/Adafruit_SSD1306.cpp" \
    "$SCRIPT_DIR/include/external/Adafruit_GFX/Adafruit_GFX.cpp" \
    -lwiringPi -lcurl -lpthread -o "$BINARY_PATH" \
    -L"$SCRIPT_DIR/include/external/ssd1306_oled_rpi" -lssd1306_oled_rpi

if [ $? -ne 0 ]; then
    error "Compilation failed!"
    exit 1
fi

chmod +x "$BINARY_PATH"
success "✅ Binary compiled: $BINARY_PATH"

# Step 3: Check service file
if [ ! -f "$SERVICE_PATH" ]; then
    error "Service file not found: $SERVICE_PATH"
    exit 1
fi

# Step 4: Copy systemd service file
info "📁 Copying $SERVICE_NAME to $SYSTEMD_DIR..."
sudo cp "$SERVICE_PATH" "$SYSTEMD_DIR"
sudo chmod 644 "$SYSTEMD_DIR/$SERVICE_NAME"

# Step 5: Inject environment variables into systemd unit safely
info "🔧 Injecting environment variables into systemd unit..."
sudo sed -i "/^ExecStart=/i Environment=\"WEATHER_API_KEY=${WEATHER_API_KEY//&/\\&}\"" "$SYSTEMD_DIR/$SERVICE_NAME"
sudo sed -i "/^ExecStart=/i Environment=\"WEATHER_LOCATION=${WEATHER_LOCATION//&/\\&}\"" "$SYSTEMD_DIR/$SERVICE_NAME"

# Step 6: Reload systemd daemon & enable service on boot
info "🔄 Reloading systemd daemon..."
sudo systemctl daemon-reexec
sudo systemctl daemon-reload

info "✅ Enabling service to start on boot..."
sudo systemctl enable "$SERVICE_NAME"

# Step 7: Start or restart the service
info "🚀 Starting service..."
sudo systemctl restart "$SERVICE_NAME"

# Step 8: Show service status
info "📋 Service status:"
sudo systemctl status "$SERVICE_NAME" --no-pager

success "🎉 WeatherDisplay local build & install complete!"
