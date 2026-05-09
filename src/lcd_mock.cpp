// Mac-only stub for the LCD driver. Prints what would have been sent.
// Not compiled on the Pi (the real lcd.cpp links against wiringPi).
#include "lcd.h"
#include <cstdio>
#include <string>

int lcd_open(int address) {
    std::printf("[lcd-mock] open  addr=0x%02x\n", address);
    return 1; // pretend-success
}

void lcd_init(int fd) {
    std::printf("[lcd-mock] init  fd=%d\n", fd);
}

void lcd_display(int fd, const std::string& line1, const std::string& line2) {
    std::printf("[lcd-mock] fd=%d  line1=\"%s\"  line2=\"%s\"\n",
                fd, line1.c_str(), line2.c_str());
}