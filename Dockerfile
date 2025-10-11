# Use arm32v7 Debian Bullseye slim base image (for Raspberry Pi 3/4 32-bit)
FROM arm32v7/debian:bullseye-slim

# Set working directory
WORKDIR /app

# Install build dependencies and required packages
RUN apt-get update && apt-get install -y \
    g++ \
    make \
    libcurl4-openssl-dev \
    wiringpi \
    libi2c-dev \
    libjsoncpp-dev \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Copy source files into container
COPY . .

# Build the binary
RUN g++ -std=c++17 -o weather main.cpp weather.cpp lcd.cpp -lwiringPi -lcurl

# Default command to run your weather binary
CMD ["./weather"]
