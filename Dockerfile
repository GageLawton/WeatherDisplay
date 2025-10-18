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
    make && \
    ls -lh /tmp/ssd1306/build  # Verify library exists

# Step: Compile your app linking to the SSD1306 static library and other necessary files
RUN g++ -Wall -O2 -std=c++17 \
    -I"$SCRIPT_DIR/include" \
    -I"$SCRIPT_DIR/include/external/ssd1306_oled_rpi" \  # Ensure this is correct
    "$SCRIPT_DIR/src/main.cpp" \
    "$SCRIPT_DIR/src/config.cpp" \
    "$SCRIPT_DIR/src/lcd.cpp" \
    "$SCRIPT_DIR/src/weather.cpp" \
    "$SCRIPT_DIR/src/oled.cpp" \
    "$SCRIPT_DIR/include/external/ssd1306_oled_rpi/Adafruit_SSD1306.cpp" \  # Link the .cpp explicitly
    -lwiringPi -lcurl -lpthread -o "$BINARY_PATH" \
    -L"$SCRIPT_DIR/include/external/ssd1306_oled_rpi" -lssd1306_oled_rpi  # Ensure the path to .a file is included


# Make sure the binary is executable
RUN chmod +x /app/weather

# Default command
CMD ["./weather"]
