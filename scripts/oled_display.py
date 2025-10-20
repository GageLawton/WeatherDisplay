#!/usr/bin/env python3
from luma.core.interface.serial import i2c
from luma.oled.device import ssd1306
from PIL import Image, ImageDraw, ImageFont
from datetime import datetime

def main():
    serial = i2c(port=1, address=0x3C)  # Adjust if your OLED uses a different address
    device = ssd1306(serial)
    
    width = device.width
    height = device.height
    image = Image.new('1', (width, height))
    draw = ImageDraw.Draw(image)
    font = ImageFont.load_default()
    
    now = datetime.now()
    current_time = now.strftime("%H:%M:%S")
    
    draw.rectangle((0, 0, width, height), outline=0, fill=0)
    draw.text((0, 0), current_time, font=font, fill=255)
    
    device.display(image)

if __name__ == "__main__":
    main()
