#!/bin/bash

# Flipper Zero SubGhz Toolkit Build Script
# This script builds the application using the Flipper Zero SDK

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== Flipper Zero SubGhz Toolkit Build Script ===${NC}"

# Check if we're in the right directory
if [ ! -f "application.fam" ]; then
    echo -e "${RED}Error: application.fam not found. Please run this script from the project root.${NC}"
    exit 1
fi

# Check if Flipper Zero SDK is available
if [ -z "$FLIPPER_SDK_PATH" ]; then
    echo -e "${YELLOW}Warning: FLIPPER_SDK_PATH not set.${NC}"
    echo -e "${YELLOW}Please set FLIPPER_SDK_PATH to your Flipper Zero SDK installation.${NC}"
    echo -e "${YELLOW}Example: export FLIPPER_SDK_PATH=/path/to/flipperzero-firmware${NC}"
    
    # Try to find common SDK locations
    if [ -d "/opt/flipperzero-firmware" ]; then
        export FLIPPER_SDK_PATH="/opt/flipperzero-firmware"
        echo -e "${GREEN}Found SDK at /opt/flipperzero-firmware${NC}"
    elif [ -d "$HOME/flipperzero-firmware" ]; then
        export FLIPPER_SDK_PATH="$HOME/flipperzero-firmware"
        echo -e "${GREEN}Found SDK at $HOME/flipperzero-firmware${NC}"
    else
        echo -e "${RED}Could not find Flipper Zero SDK. Please install it first.${NC}"
        echo -e "${YELLOW}Visit: https://docs.flipperzero.one/development/firmware/setting-up-the-development-environment${NC}"
        exit 1
    fi
fi

# Check if SDK tools are available
if [ ! -f "$FLIPPER_SDK_PATH/scripts/fbt" ]; then
    echo -e "${RED}Error: Flipper Build Tool (fbt) not found at $FLIPPER_SDK_PATH/scripts/fbt${NC}"
    exit 1
fi

echo -e "${GREEN}Using Flipper SDK at: $FLIPPER_SDK_PATH${NC}"

# Create build directory
BUILD_DIR="build"
mkdir -p "$BUILD_DIR"

# Build the application
echo -e "${BLUE}Building SubGhz Toolkit...${NC}"
cd "$FLIPPER_SDK_PATH"
./scripts/fbt fap_dist -p subghz_toolkit -d "$(pwd)/../$(basename $(pwd))"

# Check if build was successful
if [ $? -eq 0 ]; then
    echo -e "${GREEN}Build successful!${NC}"
    
    # Find the built FAP file
    FAP_FILE=$(find . -name "subghz_toolkit.fap" -type f | head -1)
    
    if [ -n "$FAP_FILE" ]; then
        echo -e "${GREEN}FAP file created: $FAP_FILE${NC}"
        
        # Copy to project directory
        cp "$FAP_FILE" "../$(basename $(pwd))/$BUILD_DIR/"
        echo -e "${GREEN}FAP copied to: $BUILD_DIR/subghz_toolkit.fap${NC}"
        
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
            echo -e "${YELLOW}You can manually copy $BUILD_DIR/subghz_toolkit.fap to your Flipper Zero's apps/SubGhz/ directory.${NC}"
        fi
    else
        echo -e "${RED}Error: Could not find built FAP file${NC}"
        exit 1
    fi
else
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi

echo -e "${GREEN}Build process completed!${NC}"