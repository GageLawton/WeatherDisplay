RUN apt-get update && \
    apt-get install -y \
        g++ \
        git \
        make \
        libcurl4-openssl-dev \
        nlohmann-json3-dev \
        libi2c-dev \
        ca-certificates && \
    git clone https://github.com/WiringPi/WiringPi.git /tmp/wiringpi && \
    cd /tmp/wiringpi && ./build && \
    rm -rf /tmp/wiringpi && \
    rm -rf /var/lib/apt/lists/*
