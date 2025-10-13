#!/bin/bash
set -euo pipefail

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

IMAGE_NAME="weather-display"
TEMP_CONTAINER="weather-temp"
BINARY_PATH="./weather"

function info() { echo -e "${BLUE}[INFO]${NC} $1"; }
function success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
function warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
function error() { echo -e "${RED}[ERROR]${NC} $1" >&2; }

# Cleanup function
function cleanup {
    if podman container exists "$TEMP_CONTAINER"; then
        warning "Cleaning up temporary container..."
        podman rm -f "$TEMP_CONTAINER" >/dev/null 2>&1 || true
    fi
}
trap cleanup EXIT

# Verify environment variables exist
if [ -z "${WEATHER_API_KEY:-}" ] || [ -z "${WEATHER_LOCATION:-}" ]; then
    error "WEATHER_API_KEY or WEATHER_LOCATION is not set on the Pi. Please export them before running."
    exit 1
fi

info "🔧 Starting WeatherDisplay build and deployment script"

# Step 1: Build container
info "🛠️ Building the container image '$IMAGE_NAME'..."
podman build -t "$IMAGE_NAME" .

# Step 2: Run container temporarily to compile binary
info "📦 Creating temporary container '$TEMP_CONTAINER' to build binary..."
podman create --name "$TEMP_CONTAINER" \
    -e WEATHER_API_KEY="$WEATHER_API_KEY" \
    -e WEATHER_LOCATION="$WEATHER_LOCATION" \
    "$IMAGE_NAME"

# Step 3: Copy binary out of container
info "📤 Copying binary from container to host at '$BINARY_PATH'..."
podman cp "$TEMP_CONTAINER:/app/weather" "$BINARY_PATH"

# Step 4: Remove temporary container
info "🧹 Removing temporary container..."
podman rm -f "$TEMP_CONTAINER" >/dev/null

# Step 5: Make binary executable
info "✅ Setting executable permissions on '$BINARY_PATH'..."
chmod +x "$BINARY_PATH"

success "🎉 WeatherDisplay binary is ready and compiled with Pi environment variables!"
