# Makefile wrapper for CMake-based build system

# === Build paths ===
BUILD_DIR := build
BIN_DIR := $(BUILD_DIR)/bin
EXECUTABLE := weather_display

# === Python script paths ===
OLED_SCRIPT := scripts/oled_display.py
LED_SCRIPT := scripts/led_celestial_display.py

# === Default target ===
all: $(BUILD_DIR)/Makefile
	@$(MAKE) -C $(BUILD_DIR)

# === Run full system (LED + C++ app) ===
run: all ensure-oled-script ensure-led-script
	@echo "🌙 Starting LED celestial display..."
	@python3 $(LED_SCRIPT) &

	@echo "🔧 Running $(EXECUTABLE)..."
	@$(BIN_DIR)/$(EXECUTABLE)

# === Run OLED display script independently ===
oled: ensure-oled-script
	@echo "🕒 Running OLED time display script..."
	@python3 $(OLED_SCRIPT)

# === Run LED celestial display script independently ===
led: ensure-led-script
	@echo "🌙 Running LED celestial display script..."
	@python3 $(LED_SCRIPT) &

# === Ensure OLED script is executable ===
ensure-oled-script:
	@echo "🔍 Ensuring $(OLED_SCRIPT) is executable..."
	@test -x $(OLED_SCRIPT) || chmod +x $(OLED_SCRIPT)

# === Ensure LED script is executable ===
ensure-led-script:
	@echo "🔍 Ensuring $(LED_SCRIPT) is executable..."
	@test -x $(LED_SCRIPT) || chmod +x $(LED_SCRIPT)

# === Configure with CMake ===
$(BUILD_DIR)/Makefile:
	@echo "📁 Creating build directory and configuring with CMake..."
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake ..

# === Clean build directory ===
clean:
	@echo "🧹 Cleaning build directory..."
	@rm -rf $(BUILD_DIR)

# === Stop running Python scripts (LED or OLED) ===
stop:
	@echo "🛑 Stopping background Python scripts..."
	@pkill -f $(LED_SCRIPT) || true
	@pkill -f $(OLED_SCRIPT) || true

.PHONY: all clean run oled led stop ensure-oled-script ensure-led-script
