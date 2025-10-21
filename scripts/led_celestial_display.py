import time
import json
import os
from datetime import datetime
from astral import LocationInfo
from astral.sun import sun
from astral.moon import phase as moon_phase  # for moon phase
import board
import neopixel

# === Load Config ===
script_dir = os.path.dirname(os.path.abspath(__file__))
project_root = os.path.abspath(os.path.join(script_dir, ".."))
config_path = os.path.join(project_root, "config.json")

with open(config_path, "r") as f:
    config = json.load(f)

# Location and LED config
loc = config["location"]
led_cfg = config["led"]

# Setup LocationInfo
LOCATION = LocationInfo(
    loc["city"],
    loc["region"],
    loc["timezone"],
    latitude=loc["latitude"],
    longitude=loc["longitude"]
)

# LED Setup
LED_PIN = getattr(board, f"D{led_cfg['pin']}")  # e.g. board.D18
NUM_LEDS = led_cfg["count"]
BRIGHTNESS = led_cfg["brightness"]

pixels = neopixel.NeoPixel(LED_PIN, NUM_LEDS, brightness=BRIGHTNESS, auto_write=False)

# === Utility Functions ===
def is_daytime(now, sunrise, sunset):
    return sunrise <= now <= sunset

def get_sun_led_index(now, sunrise, sunset):
    total_seconds = (sunset - sunrise).total_seconds()
    elapsed = (now - sunrise).total_seconds()
    position = int((elapsed / total_seconds) * NUM_LEDS)
    return max(0, min(position, NUM_LEDS - 1))

def sun_mode(now, sunrise, sunset):
    led_index = get_sun_led_index(now, sunrise, sunset)
    warm_colors = [(255, 204, 0), (255, 153, 0), (255, 102, 0), (255, 69, 0), (255, 85, 0)]
    color = warm_colors[led_index % len(warm_colors)]

    pixels.fill((0, 0, 0))
    pixels[led_index] = color
    pixels.show()

def moon_mode():
    now = datetime.now(tz=LOCATION.timezone)
    # Moon phase returns degrees 0-29.53; waxing if phase < 15, waning otherwise
    phase_value = moon_phase(now)
    illum = max(0, min(100, (1 - abs(phase_value - 15)/15) * 100))  # approximate illumination %
    led_count = int((illum / 100.0) * NUM_LEDS)

    direction = "waxing" if phase_value < 15 else "waning"
    pixels.fill((0, 0, 0))

    for i in range(led_count):
        index = i if direction == "waxing" else NUM_LEDS - 1 - i
        pixels[index] = (80, 80, 255)  # cool blue/white hue
    
    pixels.show()

# === Main Loop ===
if __name__ == "__main__":
    print("🌙 Starting LED celestial display...")
    while True:
        now = datetime.now(tz=LOCATION.timezone)
        sun_times = sun(LOCATION.observer, date=now.date(), tzinfo=LOCATION.timezone)
        sunrise = sun_times["sunrise"]
        sunset = sun_times["sunset"]

        if is_daytime(now, sunrise, sunset):
            sun_mode(now, sunrise, sunset)
        else:
            moon_mode()

        time.sleep(60)  # Update every minute
