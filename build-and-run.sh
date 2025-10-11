#!/bin/bash
set -euo pipefail

# Colors for fancy output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

IMAGE_NAME="weather-display"
TEMP_CONTAINER="weather-temp"
BINARY_PATH="./weather"

function info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

function success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

function warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

function error() {
    echo -e "${RED}[ERROR]${NC} $1" >&2
}

function cleanup {
    if podman container exists $TEMP_CONTAINER; then
        warning "Cleaning up temporary container..."
        podman rm -f $TEMP_CONTAINER >/dev/null 2>&1 || true
    fi
}

trap cleanup EXIT

info "Starting WeatherDisplay build & run script"

info "🛠️ Building the container image '$IMAGE_NAME'..."
podman build -t "$IMAGE_NAME" .

info "📦 Creating temporary container '$TEMP_CONTAINER'..."
podman create --name "$TEMP_CONTAINER" "$IMAGE_NAME"

info "📤 Copying binary from container to host at '$BINARY_PATH'..."
podman cp "$TEMP_CONTAINER:/app/weather" "$BINARY_PATH"

info "🧹 Removing temporary container..."
podman rm -f "$TEMP_CONTAINER" >/dev/null

info "✅ Setting executable permissions on '$BINARY_PATH'..."
chmod +x "$BINARY_PATH"

success "Build completed successfully! Running binary now..."

# Run the binary replacing the shell process
exec "$BINARY_PATH"
