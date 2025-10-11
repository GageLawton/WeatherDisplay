#include <wiringPiI2C.h>
#include <wiringPi.h>
#include <unistd.h>
#include <iostream>
#include "lcd.h"
#include "weather.h"

int main() {
    const std::string apiKey = "234a6c0a573a4526bbb53739251110";
    const std::string city = "Chicago,US";
    
    std::cout << "[INFO] Starting WeatherDisplay..." << std::endl;

    int fd = wiringPiI2CSetup(0x27); // Use actual I2C address
    if (fd < 0) {
        std::cerr << "[ERROR] Failed to open I2C bus!" << std::endl;
        return 1;
    }

    lcd_init(fd);

    while (true) {
        std::cout << "[INFO] Fetching weather data..." << std::endl;

        Weather w = getWeather(apiKey, city);

        // Debug print to console
        std::cout << "[INFO] Weather Description: " << w.description << std::endl;
        std::cout << "[INFO] Temperature: " << w.tempC << " °C" << std::endl;

        // Prepare LCD display strings
        std::string line1 = "Temp: " + std::to_string((int)w.tempC) + "C";
        std::string line2 = w.description.substr(0, 16); // LCD is 16 chars wide

        lcd_display(fd, line1, line2);

        std::cout << "[INFO] Display updated. Sleeping for 10 min..." << std::endl;
        sleep(600); // Sleep for 10 minutes
    }

    return 0;
}
