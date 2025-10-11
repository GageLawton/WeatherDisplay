FROM debian:bookworm

# Install system dependencies and build tools
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

# Build and install WiringPi from source (since it's not in apt anymore)
RUN git clone https://github.com/WiringPi/WiringPi.git /tmp/wiringpi && \
    cd /tmp/wiringpi && ./build && \
    rm -rf /tmp/wiringpi

# Set up working directory
WORKDIR /app

# Copy your local source code into the container
COPY . .

# Compile your program
RUN g++ -o weather main.cpp weather.cpp lcd.cpp -lwiringPi -lcurl

# Run the weather app by default
CMD ["./weather"]
