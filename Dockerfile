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

# Set working directory
WORKDIR /app

# Copy source code into the container
COPY . .

# Compile the WeatherDisplay app with local SSD1306 source files (no -lssd1306)
RUN g++ -Wall -O2 -std=c++17 \
    -I"/app/include" \
    -I"/app/include/external/ssd1306/src" \
    -I"/app/include/external/ssd1306/src/ssd1306_hal/linux" \
    /app/main.cpp \
    /app/config.cpp \
    /app/lcd.cpp \
    /app/weather.cpp \
    /app/oled.cpp \
    /app/include/external/ssd1306/src/ssd1306_console.cpp \
    /app/include/external/ssd1306/src/ssd1306_fonts.c \
    /app/include/external/ssd1306/src/ssd1306_generic.c \
    /app/include/external/ssd1306/src/ssd1306_hal/linux/platform.c \
    -lwiringPi -lcurl -lpthread -o /app/weather

# Ensure the binary is executable
RUN chmod +x /app/weather

# Default command when container runs
CMD ["./weather"]
