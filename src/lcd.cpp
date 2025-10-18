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

// LCD size configuration
#define LCD_WIDTH 16  // Width of the display (e.g., 16 characters)
#define LCD_HEIGHT 2  // Height of the display (e.g., 2 lines)

// Toggle enable to latch data
void lcd_toggle_enable(int fd, int bits) {
    usleep(500);
    wiringPiI2CWrite(fd, (bits | ENABLE));  // Send high bit
    usleep(500);
    wiringPiI2CWrite(fd, (bits & ~ENABLE)); // Send low bit
    usleep(500);
}

// Send a byte to the LCD
void lcd_byte(int fd, int bits, int mode) {
    int bits_high = mode | (bits & 0xF0) | LCD_BACKLIGHT; // Upper nibble
    int bits_low  = mode | ((bits << 4) & 0xF0) | LCD_BACKLIGHT; // Lower nibble

    wiringPiI2CWrite(fd, bits_high);
    lcd_toggle_enable(fd, bits_high);
    wiringPiI2CWrite(fd, bits_low);
    lcd_toggle_enable(fd, bits_low);
}

// Initialize the LCD
void lcd_init(int fd) {
    lcd_byte(fd, 0x33, LCD_CMD);  // Initialize sequence
    lcd_byte(fd, 0x32, LCD_CMD);
    lcd_byte(fd, 0x06, LCD_CMD);  // Cursor move direction
    lcd_byte(fd, 0x0C, LCD_CMD);  // Display ON, Cursor OFF
    lcd_byte(fd, 0x28, LCD_CMD);  // 4-bit mode
    lcd_byte(fd, 0x01, LCD_CMD);  // Clear display
    usleep(500);  // Wait for LCD to clear
}

// Display text on LCD
void lcd_display(int fd, const std::string &line1, const std::string &line2) {
    lcd_byte(fd, 0x01, LCD_CMD);  // Clear display before updating
    usleep(500);

    // Display first line
    lcd_byte(fd, LCD_LINE_1, LCD_CMD);
    for (size_t i = 0; i < LCD_WIDTH && i < line1.length(); i++) {
        lcd_byte(fd, line1[i], LCD_CHR);
    }

    // Display second line
    lcd_byte(fd, LCD_LINE_2, LCD_CMD);
    for (size_t i = 0; i < LCD_WIDTH && i < line2.length(); i++) {
        lcd_byte(fd, line2[i], LCD_CHR);
    }
}
