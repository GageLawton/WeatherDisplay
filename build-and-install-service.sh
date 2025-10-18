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

# Step 1: Fetch SSD1306 Library
info "📦 Fetching SSD1306 library..."
SSD1306_SRC="$SCRIPT_DIR/include/external/ssd1306"

# Check if SSD1306 folder exists; if not, clone from the repo
if [ ! -d "$SSD1306_SRC" ]; then
    info "📥 Cloning SSD1306 library from GitHub..."
    mkdir -p "$SSD1306_SRC"
    git clone https://github.com/adafruit/Adafruit_SSD1306.git "$SSD1306_SRC"
else
    info "✅ SSD1306 library already present."
fi

# Step 2: Build WeatherDisplay binary (with local SSD1306 source)
info "🛠️ Compiling WeatherDisplay binary with local OLED support..."

g++ -Wall -O2 -std=c++17 \
    -I"$SCRIPT_DIR/include" \
    -I"$SCRIPT_DIR/include/external/Adafruit_SSD1306" \   # Correct path to Adafruit_SSD1306 header files
    "$SCRIPT_DIR/main.cpp" \
    "$SCRIPT_DIR/config.cpp" \
    "$SCRIPT_DIR/lcd.cpp" \
    "$SCRIPT_DIR/weather.cpp" \
    "$SCRIPT_DIR/oled.cpp" \
    "$SCRIPT_DIR/include/external/Adafruit_SSD1306/Adafruit_SSD1306.cpp" \  # Correct path to .cpp files
    -lwiringPi -lcurl -lpthread -o "$BINARY_PATH"

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
sudo sed -i "/^ExecStart=/i Environment=\"WEATHER_LOCATION=${WEATHER_LOCATION//&
