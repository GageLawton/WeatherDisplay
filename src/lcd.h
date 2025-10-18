#ifndef LCD_H
#define LCD_H

#include <string>

// Function declarations for initializing and displaying on the LCD
void lcd_init(int fd);  // Initialize the LCD
void lcd_display(int fd, const std::string &line1, const std::string &line2);  // Display text on the LCD

#endif // LCD_H
