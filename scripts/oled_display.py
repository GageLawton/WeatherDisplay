#!/usr/bin/env python3
import time
from luma.core.interface.serial import i2c
from luma.oled.device import ssd1306
from PIL import Image, ImageDraw, ImageFont
from datetime import datetime

def main():
    serial = i2c(port=1, address=0x3C)
    device = ssd1306(serial)

    font = ImageFont.load_default()

    while True:
        width = device.width
        height = device.height
        image = Image.new('1', (width, height))
        draw = ImageDraw.Draw(image)

        now = datetime.now()
        current_time = now.strftime("%H:%M:%S")

        draw.rectangle((0, 0, width, height), outline=0, fill=0)
        draw.text((0, 0), current_time, font=font, fill=255)

        device.display(image)

        time.sleep(1)

if __name__ == "__main__":
    main()
