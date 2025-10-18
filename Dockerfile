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

# Copy your app source code into the container
COPY . .

# Clone and build SSD1306_OLED_RPI library
RUN git clone https://github.com/gavinlyonsrepo/SSD1306_OLED_RPI.git /tmp/ssd1306 && \
    mkdir -p /tmp/ssd1306/build && \
    cd /tmp/ssd1306/build && \
    cmake .. && \
    make

# Compile your app linking to the SSD1306 static library
RUN g++ -Wall -O2 -std=c++17 \
    -I"$SCRIPT_DIR/include" \
    -I"$SSD1306_SRC" \
    "$SCRIPT_DIR/main.cpp" \
    "$SCRIPT_DIR/config.cpp" \
    "$SCRIPT_DIR/lcd.cpp" \            # your LCD code
    "$SCRIPT_DIR/weather.cpp" \
    "$SCRIPT_DIR/oled.cpp" \           # your OLED wrapper code
    "$SSD1306_SRC/ssd1306_console.cpp" \
    "$SSD1306_SRC/ssd1306_fonts.cpp" \
    "$SSD1306_SRC/ssd1306_i2c.cpp" \
    "$SSD1306_SRC/ssd1306_oled.cpp" \
    -lwiringPi -lcurl -lpthread -o "$BINARY_PATH"

# Make sure the binary is executable
RUN chmod +x /app/weather

# Default command
CMD ["./weather"]
