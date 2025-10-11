#ifndef LCD_H
#define LCD_H

#include <string>

void lcd_init(int addr);
void lcd_display(int addr, const std::string& line1, const std::string& line2);

#endif
