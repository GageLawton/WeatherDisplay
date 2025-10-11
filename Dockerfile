FROM debian:bookworm

RUN apt-get update && \
    apt-get install -y g++ wiringpi libcurl4-openssl-dev nlohmann-json3-dev && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN g++ -o weather main.cpp -lwiringPi -lcurl

CMD ["./weather"]
