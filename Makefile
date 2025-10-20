# Makefile wrapper for CMake-based build system

# Build directory
BUILD_DIR := build
BIN_DIR := $(BUILD_DIR)/bin
EXECUTABLE := weather_display
OLED_SCRIPT := scripts/oled_display.py

# Default target
all: $(BUILD_DIR)/Makefile
	@$(MAKE) -C $(BUILD_DIR)

# Run the executable after ensuring OLED script is ready
run: all ensure-oled-script
	@echo "🔧 Running $(EXECUTABLE)..."
	@$(BIN_DIR)/$(EXECUTABLE)

# Run OLED display script independently
oled: ensure-oled-script
	@echo "🕒 Running OLED time display script..."
	@python3 $(OLED_SCRIPT)

# Ensure OLED script is executable
ensure-oled-script:
	@echo "🔍 Ensuring $(OLED_SCRIPT) is executable..."
	@test -x $(OLED_SCRIPT) || chmod +x $(OLED_SCRIPT)

# Configure with CMake
$(BUILD_DIR)/Makefile:
	@echo "📁 Creating build directory and configuring with CMake..."
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake ..

# Clean build directory
clean:
	@echo "🧹 Cleaning build directory..."
	@rm -rf $(BUILD_DIR)

.PHONY: all clean run oled ensure-oled-script
