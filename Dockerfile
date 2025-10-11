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

# Set the working directory
WORKDIR /app

# Copy source code into the container
COPY . .

# Compile your weather display app
RUN g++ -o weather main.cpp weather.cpp lcd.cpp -lwiringPi -lcurl

# Default command when container runs
CMD ["./weather"]
