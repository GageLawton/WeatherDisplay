#!/bin/bash

IMAGE_NAME="weather-display"
CONTAINER_NAME="weather-runner"

echo "🚧 Building the Docker image: $IMAGE_NAME"
podman build -t $IMAGE_NAME .

echo "🚀 Running the container: $CONTAINER_NAME"
podman run --rm -it \
  --name $CONTAINER_NAME \
  --device /dev/i2c-1 \
  --group-add i2c \
  --privileged \
  $IMAGE_NAME
