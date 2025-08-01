#!/bin/bash

# Official Flipper Zero External App Build Script
# Based on: https://developer.flipper.net/flipperzero/doxygen/apps_on_sd_card.html

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== Official Flipper Zero External App Build Script ===${NC}"

# Check if we're in the right directory
if [ ! -f "application.fam" ]; then
    echo -e "${RED}Error: application.fam not found. Please run this script from the project root.${NC}"
    exit 1
fi

# Check if Flipper Zero firmware is available
if [ -z "$FLIPPER_FIRMWARE_PATH" ]; then
    echo -e "${YELLOW}Warning: FLIPPER_FIRMWARE_PATH not set.${NC}"
    echo -e "${YELLOW}Please set FLIPPER_FIRMWARE_PATH to your Flipper Zero firmware installation.${NC}"
    echo -e "${YELLOW}Example: export FLIPPER_FIRMWARE_PATH=/path/to/flipperzero-firmware${NC}"
    
    # Try to find common firmware locations
    if [ -d "/opt/flipperzero-firmware" ]; then
        export FLIPPER_FIRMWARE_PATH="/opt/flipperzero-firmware"
        echo -e "${GREEN}Found firmware at /opt/flipperzero-firmware${NC}"
    elif [ -d "$HOME/flipperzero-firmware" ]; then
        export FLIPPER_FIRMWARE_PATH="$HOME/flipperzero-firmware"
        echo -e "${GREEN}Found firmware at $HOME/flipperzero-firmware${NC}"
    else
        echo -e "${RED}Could not find Flipper Zero firmware. Please install it first.${NC}"
        echo -e "${YELLOW}Visit: https://docs.flipperzero.one/development/firmware/setting-up-the-development-environment${NC}"
        exit 1
    fi
fi

# Check if firmware tools are available
if [ ! -f "$FLIPPER_FIRMWARE_PATH/scripts/fbt" ]; then
    echo -e "${RED}Error: Flipper Build Tool (fbt) not found at $FLIPPER_FIRMWARE_PATH/scripts/fbt${NC}"
    exit 1
fi

echo -e "${GREEN}Using Flipper firmware at: $FLIPPER_FIRMWARE_PATH${NC}"

# Create build directory
BUILD_DIR="build"
mkdir -p "$BUILD_DIR"

# Get current directory name (should match appid)
CURRENT_DIR=$(basename $(pwd))
APP_ID=$(grep 'appid=' application.fam | cut -d'"' -f2)

if [ "$CURRENT_DIR" != "$APP_ID" ]; then
    echo -e "${YELLOW}Warning: Directory name ($CURRENT_DIR) doesn't match appid ($APP_ID)${NC}"
    echo -e "${YELLOW}This might cause issues with the build process.${NC}"
fi

# Build the application using the official Flipper Build Tool
echo -e "${BLUE}Building external app using Flipper Build Tool...${NC}"
cd "$FLIPPER_FIRMWARE_PATH"

# Build the FAP for external apps
./scripts/fbt fap_dist -p "$APP_ID" -d "$(pwd)/../$(basename $(pwd))"

# Check if build was successful
if [ $? -eq 0 ]; then
    echo -e "${GREEN}Build successful!${NC}"
    
    # Find the built FAP file
    FAP_FILE=$(find . -name "${APP_ID}.fap" -type f | head -1)
    
    if [ -n "$FAP_FILE" ]; then
        echo -e "${GREEN}FAP file created: $FAP_FILE${NC}"
        
        # Copy to project directory
        cp "$FAP_FILE" "../$(basename $(pwd))/$BUILD_DIR/"
        echo -e "${GREEN}FAP copied to: $BUILD_DIR/${APP_ID}.fap${NC}"
        
        # Show file size
        FAP_SIZE=$(stat -c%s "$FAP_FILE")
        echo -e "${GREEN}FAP file size: ${FAP_SIZE} bytes${NC}"
        
        # Check if Flipper Zero is connected
        if [ -d "/run/media/$(whoami)/FLIPPER" ]; then
            echo -e "${BLUE}Installing to Flipper Zero...${NC}"
            cp "$FAP_FILE" "/run/media/$(whoami)/FLIPPER/apps/SubGhz/"
            echo -e "${GREEN}Installed to Flipper Zero!${NC}"
        elif [ -d "/media/$(whoami)/FLIPPER" ]; then
            echo -e "${BLUE}Installing to Flipper Zero...${NC}"
            cp "$FAP_FILE" "/media/$(whoami)/FLIPPER/apps/SubGhz/"
            echo -e "${GREEN}Installed to Flipper Zero!${NC}"
        else
            echo -e "${YELLOW}Flipper Zero not detected.${NC}"
            echo -e "${YELLOW}You can manually copy $BUILD_DIR/${APP_ID}.fap to your Flipper Zero's apps/SubGhz/ directory.${NC}"
        fi
        
        # Show installation instructions
        echo -e "${BLUE}=== Installation Instructions ===${NC}"
        echo -e "${GREEN}1. Connect your Flipper Zero to your computer${NC}"
        echo -e "${GREEN}2. Enable USB Mass Storage on your Flipper Zero${NC}"
        echo -e "${GREEN}3. Copy $BUILD_DIR/${APP_ID}.fap to your Flipper Zero's apps/SubGhz/ directory${NC}"
        echo -e "${GREEN}4. Disconnect and navigate to Applications → SubGhz → SubGhz Toolkit${NC}"
        
    else
        echo -e "${RED}Error: Could not find built FAP file${NC}"
        exit 1
    fi
else
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi

echo -e "${GREEN}Build process completed!${NC}"