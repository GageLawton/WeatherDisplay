# Base image
FROM debian:bookworm

# Install build tools and required libraries
RUN apt-get update && \
    apt-get install -y \
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
        ca-certificates && \
    rm -rf /var/lib/apt/lists/*

# Build WiringPi manually
RUN git clone https://github.com/WiringPi/WiringPi.git /tmp/wiringpi && \
    cd /tmp/wiringpi/wiringPi && make && make install && \
    cd /tmp/wiringpi/devLib && make && make install && \
    rm -rf /tmp/wiringpi

# Build ssd1306 OLED library
RUN git clone https://github.com/lexus2k/ssd1306.git /tmp/ssd1306 && \
    cd /tmp/ssd1306 && mkdir build && cd build && \
    cmake .. && make -j4 && make install && \
    rm -rf /tmp/ssd1306

# Set working directory
WORKDIR /app

# Copy source code into the container
COPY . .

# Compile the WeatherDisplay app with OLED support
