#ifndef LCD_H
#define LCD_H

#include <string>

int  lcd_open(int address);
void lcd_init(int fd);
void lcd_display(int fd, const std::string &line1, const std::string &line2);

#endif // LCD_H
