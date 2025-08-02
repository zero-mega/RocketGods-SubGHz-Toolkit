# Flipper Zero SubGhz Toolkit Makefile

# Application name
APP_NAME = subghz_toolkit

# Application entry point
APP_ENTRY = subghz_toolkit_app

# Application type
APP_TYPE = external

# Application icon
APP_ICON = subghz_toolkit.png

# Application category
APP_CATEGORY = SubGhz

# Application author
APP_AUTHOR = RocketGod

# Application version
APP_VERSION = 1.0

# Application description
APP_DESCRIPTION = Extract SubGhz protocol implementation details and decrypt KeeLoq keys

# Application web URL
APP_WEBURL = https://betaskynet.com

# Source files
SRCS = subghz_toolkit.c

# Include directories
INCLUDES = .

# Compiler flags
CFLAGS = -std=gnu99 -Wall -Wextra -Werror

# Linker flags
LDFLAGS = 

# Dependencies
LIBS = -lfuri -lgui -lstorage -lnotification -lsubghz

# Build directory
BUILD_DIR = build

# FAP file
FAP_FILE = $(BUILD_DIR)/$(APP_NAME).fap

# Default target
all: $(FAP_FILE)

# Create build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compile the application
$(FAP_FILE): $(BUILD_DIR) $(SRCS)
	$(CC) $(CFLAGS) $(INCLUDES:%=-I%) -o $(BUILD_DIR)/$(APP_NAME).elf $(SRCS) $(LDFLAGS) $(LIBS)
	$(STRIP) $(BUILD_DIR)/$(APP_NAME).elf
	$(OBJCOPY) -O binary $(BUILD_DIR)/$(APP_NAME).elf $(BUILD_DIR)/$(APP_NAME).bin

# Clean build files
clean:
	rm -rf $(BUILD_DIR)

# Install to Flipper Zero (if connected)
install: $(FAP_FILE)
	@echo "Installing $(APP_NAME).fap to Flipper Zero..."
	@if [ -d "/run/media/$(USER)/FLIPPER" ]; then \
		cp $(FAP_FILE) "/run/media/$(USER)/FLIPPER/apps/SubGhz/"; \
		echo "Installed to /run/media/$(USER)/FLIPPER/apps/SubGhz/"; \
	elif [ -d "/media/$(USER)/FLIPPER" ]; then \
		cp $(FAP_FILE) "/media/$(USER)/FLIPPER/apps/SubGhz/"; \
		echo "Installed to /media/$(USER)/FLIPPER/apps/SubGhz/"; \
	else \
		echo "Flipper Zero not found. Please connect it and mount as FLIPPER drive."; \
		echo "You can manually copy $(FAP_FILE) to your Flipper Zero's apps/SubGhz/ directory."; \
	fi

# Show help
help:
	@echo "Available targets:"
	@echo "  all      - Build the FAP file"
	@echo "  clean    - Remove build files"
	@echo "  install  - Install to connected Flipper Zero"
	@echo "  help     - Show this help"

.PHONY: all clean install help