#include <iostream>
#include "lcd.h"
#include "weather.h"

int main() {
    const std::string apiKey = "YOUR_API_KEY";
    const std::string city = "YOUR_CITY";
    
    int fd = wiringPiI2CSetup(0x27); // or whatever address
    if (fd < 0) {
        std::cerr << "Failed to open I2C\n";
        return 1;
    }
    lcd_init(fd);

    while (true) {
        Weather w = getWeather(apiKey, city);
        std::string line1 = "Temp: " + std::to_string((int)w.tempC) + "C";
        std::string line2 = w.description.substr(0,16);
        lcd_display(fd, line1, line2);
        sleep(600);
    }

    return 0;
}
