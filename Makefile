# Top-level Makefile - thin wrapper around CMake.

BUILD_DIR  := build
BIN        := $(BUILD_DIR)/bin/weather_display

.PHONY: all run clean install-service

all: $(BIN)

$(BIN): $(BUILD_DIR)/Makefile
	@$(MAKE) -C $(BUILD_DIR)

$(BUILD_DIR)/Makefile:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake ..

run: all
	@echo "🔧 Running weather_display..."
	@$(BIN)

install-service: all
	@$(MAKE) -C $(BUILD_DIR) install-service

clean:
	@rm -rf $(BUILD_DIR)