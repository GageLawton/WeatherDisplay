#include "lcd.h"
#include <wiringPiI2C.h>
#include <unistd.h>
#include <string>
#include <iostream>

#define LCD_CHR  1
#define LCD_CMD  0

#define LCD_LINE_1 0x80
#define LCD_LINE_2 0xC0

#define LCD_BACKLIGHT 0x08
#define ENABLE 0b00000100

void lcd_toggle_enable(int fd, int bits) {
    usleep(500);
    wiringPiI2CWrite(fd, (bits | ENABLE));
    usleep(500);
    wiringPiI2CWrite(fd, (bits & ~ENABLE));
    usleep(500);
}

void lcd_byte(int fd, int bits, int mode) {
    int bits_high = mode | (bits & 0xF0) | LCD_BACKLIGHT;
    int bits_low  = mode | ((bits << 4) & 0xF0) | LCD_BACKLIGHT;

    wiringPiI2CWrite(fd, bits_high);
    lcd_toggle_enable(fd, bits_high);
    wiringPiI2CWrite(fd, bits_low);
    lcd_toggle_enable(fd, bits_low);
}

void lcd_init(int fd) {
    lcd_byte(fd, 0x33, LCD_CMD);
    lcd_byte(fd, 0x32, LCD_CMD);
    lcd_byte(fd, 0x06, LCD_CMD);
    lcd_byte(fd, 0x0C, LCD_CMD);
    lcd_byte(fd, 0x28, LCD_CMD);
    lcd_byte(fd, 0x01, LCD_CMD);
    usleep(500);
}

void lcd_display(int fd, const std::string &line1, const std::string &line2) {
    lcd_byte(fd, LCD_LINE_1, LCD_CMD);
    for (char c : line1)
        lcd_byte(fd, c, LCD_CHR);

    lcd_byte(fd, LCD_LINE_2, LCD_CMD);
    for (char c : line2)
        lcd_byte(fd, c, LCD_CHR);
}
