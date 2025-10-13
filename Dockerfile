# Base image
FROM debian:bookworm

# Install build tools and libraries
RUN apt-get update && \
    apt-get install -y \
        g++ \
        git \
        make \
        libcurl4-openssl-dev \
        nlohmann-json3-dev \
        libi2c-dev \
        ca-certificates && \
    rm -rf /var/lib/apt/lists/*

# Build WiringPi manually (avoiding 'sudo' issues)
RUN git clone https://github.com/WiringPi/WiringPi.git /tmp/wiringpi && \
    cd /tmp/wiringpi/wiringPi && make && make install && \
    cd /tmp/wiringpi/devLib && make && make install && \
    rm -rf /tmp/wiringpi

# Set working directory
WORKDIR /app

# Copy source code into the container
COPY . .

# Compile your WeatherDisplay app
RUN g++ -Wall -O2 -std=c++11 -I./include \
    main.cpp weather.cpp lcd.cpp config.cpp \
    -lwiringPi -lcurl -o weather

# Ensure the binary is executable
RUN chmod +x weather

# Default command when container runs
CMD ["./weather"]
