#!/usr/bin/env python3
from luma.core.interface.serial import i2c
from luma.oled.device import ssd1306
from PIL import Image, ImageDraw, ImageFont
from datetime import datetime
import time

def main():
    serial = i2c(port=1, address=0x3C)  # Adjust if your OLED uses a different address
    device = ssd1306(serial)
    
    # Load a larger font (adjust path & size as needed)
    font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 20)

    while True:
        width = device.width
        height = device.height
        image = Image.new('1', (width, height))
        draw = ImageDraw.Draw(image)

        now = datetime.now()
        current_time = now.strftime("%H:%M:%S")

        # Clear the display
        draw.rectangle((0, 0, width, height), outline=0, fill=0)

        # Calculate centered position
        (font_width, font_height) = font.getsize(current_time)
        x = (width - font_width) // 2
        y = (height - font_height) // 2

        # Draw the time
        draw.text((x, y), current_time, font=font, fill=255)

        # Display image
        device.display(image)

        time.sleep(1)

if __name__ == "__main__":
    main()
