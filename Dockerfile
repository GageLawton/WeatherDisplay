# Use a base image with Raspberry Pi OS
FROM docker.io/arm32v7/debian:bullseye-slim

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    g++ \
    git \
    curl \
    libcurl4-openssl-dev \
    libi2c-dev \
    wiringpi \
    libjsoncpp-dev \
    && rm -rf /var/lib/apt/lists/*

# Clone the repository
WORKDIR /app
RUN git clone https://github.com/GageLawton/WeatherDisplay.git .

# Build the application
RUN cmake . && make

# Set the entry point to run the weather application
ENTRYPOINT ["./weather"]

# Ensure the container has access to I2C and GPIO
USER root
