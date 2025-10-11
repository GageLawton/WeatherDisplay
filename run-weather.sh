#!/bin/bash
set -euo pipefail

IMAGE_NAME="weather-display"
TEMP_CONTAINER="weather-temp"
I2C_DEVICE="/dev/i2c-1"

echo "🚧 Building Docker image: $IMAGE_NAME"
podman build -t $IMAGE_NAME .

echo "🛠 Creating temporary container to extract binary"
podman create --name $TEMP_CONTAINER $IMAGE_NAME

echo "📦 Copying binary out of container"
podman cp $TEMP_CONTAINER:/app/weather ./weather

echo "🗑 Removing temporary container"
podman rm $TEMP_CONTAINER

echo "🔒 Fixing permissions on $I2C_DEVICE (requires sudo)"
if [ -e "$I2C_DEVICE" ]; then
  echo "⚠️  Temporarily setting $I2C_DEVICE permissions to 666 (read/write for all)"
  sudo chmod 666 $I2C_DEVICE
else
  echo "❌ Device $I2C_DEVICE not found! Exiting."
  exit 1
fi

echo "✅ Making binary executable"
chmod +x ./weather

echo "🚀 Running container with device and permissions"
podman run --rm -it \
  --device $I2C_DEVICE:$I2C_DEVICE:rwm \
  --group-add i2c \
  --userns=keep-id \
  --entrypoint /app/weather \
  $IMAGE_NAME
