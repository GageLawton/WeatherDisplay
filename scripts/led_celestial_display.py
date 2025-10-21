import time
import json
import os
from datetime import datetime
from astral import LocationInfo
from astral.sun import sun
from astral.moon import phase as moon_phase
from zoneinfo import ZoneInfo  # Standard timezone support in Python 3.9+
import board
import neopixel

# === Load Config Safely ===
script_dir = os.path.dirname(os.path.abspath(__file__))
project_root = os.path.abspath(os.path.join(script_dir, ".."))
config_path = os.path.join(project_root, "config.json")

print(f"🔧 Loading config from: {config_path}")
if not os.path.exists(config_path):
    raise FileNotFoundError(f"Config file not found at {config_path}")

with open(config_path, "r") as f:
    config = json.load(f)

# === Extract Config ===
loc = config["location"]
led_cfg = config["led"]
timezone = ZoneInfo(loc["timezone"])  # e.g., "America/New_York"

# Setup LocationInfo for Astral
LOCATION = LocationInfo(
    name=loc["city"],
    region=loc["region"],
    timezone=loc["timezone"],
    latitude=loc["latitude"],
    longitude=loc["longitude"]
)

# Setup LEDs
LED_PIN = getattr(board, f"D{led_cfg['pin']}")  # e.g., board.D18
NUM_LEDS = led_cfg["count"]
BRIGHTNESS = led_cfg["brightness"]

pixels = neopixel.NeoPixel(LED_PIN, NUM_LEDS, brightness=BRIGHTNESS, auto_write=False)

# === Helper Functions ===
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

def moon_mode(now):
    phase_val = moon_phase(now)  # 0 to 29.53
    illumination = max(0, min(100, (1 - abs(phase_val - 15) / 15) * 100))  # rough estimate

    led_count = int((illumination / 100.0) * NUM_LEDS)
    direction = "waxing" if phase_val < 15 else "waning"

    pixels.fill((0, 0, 0))
    for i in range(led_count):
        index = i if direction == "waxing" else NUM_LEDS - 1 - i
        pixels[index] = (80, 80, 255)  # cool moonlight color
    pixels.show()

# === Main Loop ===
if __name__ == "__main__":
    print("🌙 Starting LED celestial display...")
    while True:
        now = datetime.now(tz=timezone)
        sun_times = sun(LOCATION.observer, date=now.date(), tzinfo=timezone)
        sunrise = sun_times["sunrise"]
        sunset = sun_times["sunset"]

        if is_daytime(now, sunrise, sunset):
            sun_mode(now, sunrise, sunset)
        else:
            moon_mode(now)

        time.sleep(60)  # Update every minute
