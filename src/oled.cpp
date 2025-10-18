#include <iostream>
#include <chrono>
#include <ctime>
#include <thread>
#include "Adafruit_SSD1306.h"

SSD1306 display;

void showTime() {
    while (true) {
        // Clear the display buffer
        display.clear();

        // Get current time
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::tm* ptm = std::localtime(&now_time);

        // Format time string HH:MM:SS
        char timeStr[9];
        std::strftime(timeStr, sizeof(timeStr), "%H:%M:%S", ptm);

        // Print time at (x=0, y=0)
        display.print(timeStr);

        // Update the physical display
        display.display();

        // Sleep for 1 second
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

int main() {
    // Initialize the display
    if (!display.init()) {
        std::cerr << "OLED initialization failed!" << std::endl;
        return 1;
    }

    // Start the time display in a separate thread
    std::thread timeThread(showTime);

    // Join the timeThread (it will run indefinitely)
    timeThread.join();

    return 0;
}
