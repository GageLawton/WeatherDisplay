#include <iostream>
#include <string>
#include "weather.h"
#include "lcd.h"

int main() {
    std::string apiKey = "YOUR_API_KEY";
    std::string city = "YOUR_CITY";

    std::string weather = getWeather(apiKey, city);
    lcd_init(0x27);  // or whatever your LCD I2C address is
    lcd_display(0x27, "Weather:", weather);

    return 0;
}
