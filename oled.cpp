#include "oled.h"
#include "ssd1306_i2c.h"
#include <nano_gfx.h>
#include <chrono>
#include <ctime>
#include <thread>
#include <iostream>

static SSD1306 oled(128, 64);

void startOLEDClock() {
    if (!oled.begin(SSD1306_I2C_ADDRESS, -1)) {
        std::cerr << "[ERROR] Failed to initialize OLED clock display!" << std::endl;
        return;
    }

    oled.clear();
    oled.setFont(Font_6x8);

    std::thread([&oled]() {
        while (true) {
            oled.clear();

            // Get current time
            auto now = std::chrono::system_clock::now();
            std::time_t now_time = std::chrono::system_clock::to_time_t(now);
            std::tm* ptm = std::localtime(&now_time);

            char timeStr[16];
            std::strftime(timeStr, sizeof(timeStr), "%H:%M:%S", ptm);

            // Center text: 6 pixels width per char * 8 chars
            int x = (128 - 6 * 8) / 2;
            int y = (64 - 8) / 2;

            oled.setCursor(x, y);
            oled.print(timeStr);
            oled.display();

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }).detach();
}
