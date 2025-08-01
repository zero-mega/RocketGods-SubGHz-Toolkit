#!/bin/bash

# Simple Flipper Zero SubGhz Toolkit Build Script
# This script builds the application using basic tools

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== Simple Flipper Zero SubGhz Toolkit Build Script ===${NC}"

# Check if we're in the right directory
if [ ! -f "application.fam" ]; then
    echo -e "${RED}Error: application.fam not found. Please run this script from the project root.${NC}"
    exit 1
fi

# Check for required tools
if ! command -v arm-none-eabi-gcc &> /dev/null; then
    echo -e "${RED}Error: arm-none-eabi-gcc not found. Please install ARM GCC toolchain.${NC}"
    echo -e "${YELLOW}On Ubuntu/Debian: sudo apt install gcc-arm-none-eabi${NC}"
    echo -e "${YELLOW}On Arch: sudo pacman -S arm-none-eabi-gcc${NC}"
    exit 1
fi

if ! command -v arm-none-eabi-strip &> /dev/null; then
    echo -e "${RED}Error: arm-none-eabi-strip not found. Please install ARM binutils.${NC}"
    echo -e "${YELLOW}On Ubuntu/Debian: sudo apt install binutils-arm-none-eabi${NC}"
    echo -e "${YELLOW}On Arch: sudo pacman -S arm-none-eabi-binutils${NC}"
    exit 1
fi

if ! command -v arm-none-eabi-objcopy &> /dev/null; then
    echo -e "${RED}Error: arm-none-eabi-objcopy not found. Please install ARM binutils.${NC}"
    exit 1
fi

echo -e "${GREEN}Required tools found.${NC}"

# Create build directory
BUILD_DIR="build"
mkdir -p "$BUILD_DIR"

# Set compiler and tools
CC="arm-none-eabi-gcc"
STRIP="arm-none-eabi-strip"
OBJCOPY="arm-none-eabi-objcopy"

# Compiler flags for ARM Cortex-M4
CFLAGS="-std=gnu99 -Wall -Wextra -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -Os -fdata-sections -ffunction-sections"

# Include directories (you may need to adjust these paths)
INCLUDES="-I. -I/opt/flipperzero-firmware/lib -I/opt/flipperzero-firmware/applications"

# Linker flags
LDFLAGS="-Wl,--gc-sections -Wl,--strip-all"

# Libraries (you may need to adjust these paths)
LIBS="-L/opt/flipperzero-firmware/lib -lfuri -lgui -lstorage -lnotification -lsubghz"

echo -e "${BLUE}Building SubGhz Toolkit...${NC}"

# Compile the application
$CC $CFLAGS $INCLUDES -o "$BUILD_DIR/subghz_toolkit.elf" subghz_toolkit.c $LDFLAGS $LIBS

if [ $? -eq 0 ]; then
    echo -e "${GREEN}Compilation successful!${NC}"
    
    # Strip the binary
    $STRIP "$BUILD_DIR/subghz_toolkit.elf"
    
    # Convert to binary
    $OBJCOPY -O binary "$BUILD_DIR/subghz_toolkit.elf" "$BUILD_DIR/subghz_toolkit.bin"
    
    # Create FAP file (simple binary copy for now)
    cp "$BUILD_DIR/subghz_toolkit.bin" "$BUILD_DIR/subghz_toolkit.fap"
    
    echo -e "${GREEN}FAP file created: $BUILD_DIR/subghz_toolkit.fap${NC}"
    
    # Check if Flipper Zero is connected
    if [ -d "/run/media/$(whoami)/FLIPPER" ]; then
        echo -e "${BLUE}Installing to Flipper Zero...${NC}"
        cp "$BUILD_DIR/subghz_toolkit.fap" "/run/media/$(whoami)/FLIPPER/apps/SubGhz/"
        echo -e "${GREEN}Installed to Flipper Zero!${NC}"
    elif [ -d "/media/$(whoami)/FLIPPER" ]; then
        echo -e "${BLUE}Installing to Flipper Zero...${NC}"
        cp "$BUILD_DIR/subghz_toolkit.fap" "/media/$(whoami)/FLIPPER/apps/SubGhz/"
        echo -e "${GREEN}Installed to Flipper Zero!${NC}"
    else
        echo -e "${YELLOW}Flipper Zero not detected.${NC}"
        echo -e "${YELLOW}You can manually copy $BUILD_DIR/subghz_toolkit.fap to your Flipper Zero's apps/SubGhz/ directory.${NC}"
    fi
else
    echo -e "${RED}Compilation failed!${NC}"
    exit 1
fi

echo -e "${GREEN}Build process completed!${NC}"