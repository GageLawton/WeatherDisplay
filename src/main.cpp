#include "config.h"
#include "lcd.h"
#include "oled.h"
#include "weather.h"
#include "neopixel.h"
#include "ring.h"
#include "celestial.h"

#include <chrono>
#include <ctime>
#include <iostream>
#include <string>
#include <thread>

namespace {

// Translate friendly time placeholders ("HH:MM:SS", "HH:MM") to strftime.
std::string formatTime(const std::string& fmt, std::time_t t) {
    std::tm tm = *std::localtime(&t);
    std::string sf;
    for (size_t i = 0; i < fmt.size(); ) {
        if      (fmt.compare(i, 8, "HH:MM:SS") == 0) { sf += "%H:%M:%S"; i += 8; }
        else if (fmt.compare(i, 5, "HH:MM")    == 0) { sf += "%H:%M";    i += 5; }
        else if (fmt.compare(i, 2, "SS")       == 0) { sf += "%S";       i += 2; }
        else { sf += fmt[i++]; }
    }
    char buf[64];
    std::strftime(buf, sizeof(buf), sf.c_str(), &tm);
    return buf;
}

void drawTime(OLED& oled, const std::string& text, const std::string& scaleSpec) {
    oled.clear();
    if (scaleSpec == "auto") {
        for (int s = 4; s >= 1; --s) {
            if (OLED::textWidth(text, s) <= OLED::WIDTH) {
                int y = (OLED::HEIGHT - OLED::textHeight(s)) / 2;
                oled.drawTextCenteredFit(y, text, s);
                break;
            }
        }
    } else {
        int s = std::atoi(scaleSpec.c_str());
        if (s < 1) s = 1;
        if (s > 4) s = 4;
        int y = (OLED::HEIGHT - OLED::textHeight(s)) / 2;
        oled.drawTextCenteredFit(y, text, s);
    }
}

std::string padTo(const std::string& s, size_t width) {
    if (s.size() >= width) return s.substr(0, width);
    return s + std::string(width - s.size(), ' ');
}

} // namespace

int main() {
    // ---- Load configuration ----
    Config cfg;
    cfg.load("config.json");

    std::cout << "[INFO] ==============================" << std::endl;
    std::cout << "[INFO] Starting WeatherDisplay" << std::endl;
    std::cout << "[INFO] Location:        " << cfg.location
              << " (" << cfg.latitude << ", " << cfg.longitude << ")" << std::endl;
    std::cout << "[INFO] Units:           " << cfg.units << std::endl;
    std::cout << "[INFO] Weather refresh: " << cfg.updateInterval << " seconds" << std::endl;
    std::cout << "[INFO] OLED format:     " << cfg.oledFormat << " (scale " << cfg.oledScale << ")" << std::endl;
    std::cout << "[INFO] LED ring:        " << cfg.ledCount << " pixels @ " << cfg.ledSpiDevice
              << ", brightness " << cfg.ledBrightness
              << ", offset " << cfg.ledOffset
              << ", " << (cfg.ledClockwise ? "clockwise" : "counterclockwise") << std::endl;
    std::cout << "[INFO] ==============================\n" << std::endl;

    // ---- Initialize LCD (HD44780 over I2C @ 0x27) ----
    int lcdFd = lcd_open(0x27);
    if (lcdFd < 0) {
        std::cerr << "[ERROR] Failed to open LCD on I2C bus" << std::endl;
        return 1;
    }
    lcd_init(lcdFd);

    // ---- Initialize OLED (SSD1306 over I2C) ----
    OLED oled;
    if (!oled.begin(cfg.oledI2CAddr)) {
        std::cerr << "[ERROR] Failed to open OLED on I2C bus at 0x"
                  << std::hex << cfg.oledI2CAddr << std::dec << std::endl;
        return 1;
    }

    // ---- Initialize NeoPixel ring (WS2812 over SPI) ----
    NeoPixel ring;
    bool ringOk = ring.begin(cfg.ledCount, cfg.ledSpiDevice);
    if (!ringOk) {
        std::cerr << "[WARN] Failed to open LED ring at " << cfg.ledSpiDevice
                  << " - continuing without ring." << std::endl;
    } else {
        ring.setOrientation(cfg.ledOffset, cfg.ledClockwise);
        ring.setBrightness(cfg.ledBrightness);
        ring.clear();
        ring.show();
    }

    // ---- Configure ring controller ----
    celestial::Observer obs{cfg.latitude, cfg.longitude};
    RingDisplay ringDisplay;
    if (ringOk) ringDisplay.configure(&ring, obs);

    // ---- Main loop: tick once per second ----
    using clock = std::chrono::steady_clock;
    auto lastWeatherFetch = clock::time_point{};
    auto lastRingUpdate   = clock::time_point{};
    std::string lcdLine1 = "Loading...";
    std::string lcdLine2 = "";

    while (true) {
        auto tickStart = clock::now();
        std::time_t now = std::time(nullptr);

        // ---- 1 Hz: refresh OLED time ----
        std::string timeStr = formatTime(cfg.oledFormat, now);
        drawTime(oled, timeStr, cfg.oledScale);
        if (!oled.show()) {
            std::cerr << "[WARN] OLED show() failed" << std::endl;
        }

        // ---- 1/60 Hz: refresh ring ----
        bool needRing = (lastRingUpdate == clock::time_point{}) ||
            std::chrono::duration_cast<std::chrono::seconds>(tickStart - lastRingUpdate).count() >= 60;
        if (needRing && ringOk) {
            ringDisplay.update(now);
            lastRingUpdate = tickStart;
        }

        // ---- 1/N Hz: refresh weather + LCD ----
        bool needWeather = (lastWeatherFetch == clock::time_point{}) ||
            std::chrono::duration_cast<std::chrono::seconds>(tickStart - lastWeatherFetch).count() >= cfg.updateInterval;

        if (needWeather && !cfg.apiKey.empty()) {
            std::cout << "[DEBUG] Fetching weather for \"" << cfg.location << "\"..." << std::endl;
            Weather w = getWeather(cfg.apiKey, cfg.location);
            float temp = (cfg.units == "F") ? (w.tempC * 9.0f / 5.0f) + 32.0f : w.tempC;

            lcdLine1 = "Temp: " + std::to_string((int)temp) + cfg.units;
            lcdLine2 = w.description.substr(0, 16);

            std::cout << "[INFO] Weather: " << lcdLine1 << " / " << lcdLine2 << std::endl;
            lastWeatherFetch = tickStart;
        }

        // Always re-paint the LCD so it doesn't go stale.
        lcd_display(lcdFd, padTo(lcdLine1, 16), padTo(lcdLine2, 16));

        // ---- Sleep until the next 1-second boundary ----
        auto tickEnd  = clock::now();
        auto elapsed  = tickEnd - tickStart;
        auto target   = std::chrono::seconds(1);
        if (elapsed < target) {
            std::this_thread::sleep_for(target - elapsed);
        }
    }

    return 0;  // unreachable
}