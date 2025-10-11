#include "lcd.cpp"
#include "weather.cpp"
#include <iostream>
#include <unistd.h>
#include <wiringPiI2C.h>

int main() {
    const std::string apiKey = "YOUR_API_KEY";
    const std::string city = "Chicago";

    int fd = wiringPiI2CSetup(0x27);
    lcd_init(fd);

    while (true) {
        auto weather = getWeather(apiKey, city);

        std::string temp = "Temp: " + std::to_string((int)weather.tempC) + "C";
        std::string desc = weather.description.substr(0, 16); // fit LCD width

        lcd_display(fd, temp, desc);
        sleep(600); // update every 10 min
    }

    return 0;
}
