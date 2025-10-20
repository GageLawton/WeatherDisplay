# Makefile wrapper for CMake-based build system

# Build directory
BUILD_DIR := build
BIN_DIR := $(BUILD_DIR)/bin
EXECUTABLE := weather_display

# Default target
all: $(BUILD_DIR)/Makefile
	@$(MAKE) -C $(BUILD_DIR)

# Run the executable
run: all
	@echo "🔧 Running $(EXECUTABLE)..."
	@$(BIN_DIR)/$(EXECUTABLE)

# Configure with CMake
$(BUILD_DIR)/Makefile:
	@echo "📁 Creating build directory and configuring with CMake..."
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake ..

# Clean build directory
clean:
	@echo "🧹 Cleaning build directory..."
	@rm -rf $(BUILD_DIR)

.PHONY: all clean run
