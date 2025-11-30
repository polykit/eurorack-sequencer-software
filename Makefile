# Makefile for Arduino Nano Sequencer
# Uses arduino-cli for compilation and upload

# Configuration
ARDUINO_CLI ?= arduino-cli
BOARD_FQBN = arduino:avr:nano
SKETCH_DIR = .
SKETCH = $(SKETCH_DIR)/eurorack-sequencer-software.ino
PORT ?= /dev/ttyUSB0
BAUD_RATE ?= 115200

# Default target
.PHONY: all
all: compile

# Compile the sketch
.PHONY: compile
compile:
	@echo "Compiling $(SKETCH)..."
	$(ARDUINO_CLI) compile -vt --fqbn $(BOARD_FQBN) $(SKETCH_DIR)

# Upload to Arduino
.PHONY: upload
upload: compile
	@echo "Uploading to $(PORT)..."
	$(ARDUINO_CLI) upload -vt -p $(PORT) --fqbn $(BOARD_FQBN) $(SKETCH_DIR)

# Compile and upload in one step
.PHONY: build-upload
build-upload: upload

# Open serial monitor
.PHONY: monitor
monitor:
	@echo "Opening serial monitor on $(PORT)..."
	$(ARDUINO_CLI) monitor -p $(PORT) --config $(BAUD_RATE)

# Clean build artifacts
.PHONY: clean
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(SKETCH_DIR)/build

# List available ports
.PHONY: ports
ports:
	$(ARDUINO_CLI) board list

# Check if arduino-cli is installed
.PHONY: check
check:
	@which $(ARDUINO_CLI) > /dev/null || (echo "Error: $(ARDUINO_CLI) not found. Install it from https://arduino.github.io/arduino-cli/" && exit 1)
	@echo "✓ $(ARDUINO_CLI) found"

# Help target
.PHONY: help
help:
	@echo "Available targets:"
	@echo "  make compile      - Compile the sketch"
	@echo "  make upload       - Compile and upload to Arduino (uses PORT=$(PORT))"
	@echo "  make build-upload - Same as upload"
	@echo "  make monitor      - Open serial monitor (uses PORT=$(PORT), BAUD_RATE=$(BAUD_RATE))"
	@echo "  make ports        - List available serial ports"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make check        - Verify arduino-cli is installed"
	@echo ""
	@echo "Override PORT: make upload PORT=/dev/ttyACM0"
	@echo "Override ARDUINO_CLI: make compile ARDUINO_CLI=arduino-cli"

