#include <wiringPiI2C.h>
#include <wiringPi.h>
#include <unistd.h>
#include <iostream>
#include "lcd.h"
#include "weather.h"

int main() {
    const std::string apiKey = "234a6c0a573a4526bbb53739251110";
    const std::string city = "Chicago,US";
    
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
