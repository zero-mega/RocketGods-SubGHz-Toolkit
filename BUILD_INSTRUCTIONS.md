# Flipper Zero SubGhz Toolkit - Build Instructions

This document explains how to build and install the SubGhz Toolkit as a Flipper Application Package (FAP) for your Flipper Zero.

## Prerequisites

### Option 1: Full Flipper Zero SDK (Recommended)

1. **Install the Flipper Zero SDK:**
   ```bash
   # Clone the firmware repository
   git clone --recursive https://github.com/flipperdevices/flipperzero-firmware.git
   cd flipperzero-firmware
   
   # Install dependencies (Ubuntu/Debian)
   sudo apt update
   sudo apt install -y git wget libusb-1.0-0-dev libudev-dev
   sudo apt install -y python3 python3-pip python3-setuptools
   sudo apt install -y gcc-arm-none-eabi binutils-arm-none-eabi
   
   # Set up the environment
   export FLIPPER_SDK_PATH=$(pwd)
   ```

2. **Build using the SDK:**
   ```bash
   # From the project directory
   ./build.sh
   ```

### Option 2: Basic ARM Toolchain

If you don't want to install the full SDK, you can use just the ARM toolchain:

1. **Install ARM GCC toolchain:**
   ```bash
   # Ubuntu/Debian
   sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi
   
   # Arch Linux
   sudo pacman -S arm-none-eabi-gcc arm-none-eabi-binutils
   
   # macOS (using Homebrew)
   brew install arm-none-eabi-gcc
   ```

2. **Build using basic tools:**
   ```bash
   # From the project directory
   ./build_simple.sh
   ```

## Building the Application

### Method 1: Using the Full SDK (Recommended)

```bash
# Make sure you're in the project directory
cd /path/to/subghz-toolkit

# Set the SDK path (if not already set)
export FLIPPER_SDK_PATH=/path/to/flipperzero-firmware

# Build the application
./build.sh
```

### Method 2: Using Basic Toolchain

```bash
# Make sure you're in the project directory
cd /path/to/subghz-toolkit

# Build the application
./build_simple.sh
```

### Method 3: Manual Build

If the automated scripts don't work, you can build manually:

```bash
# Create build directory
mkdir -p build

# Compile (adjust paths as needed)
arm-none-eabi-gcc \
  -std=gnu99 \
  -Wall -Wextra \
  -mcpu=cortex-m4 \
  -mthumb \
  -mfloat-abi=hard \
  -mfpu=fpv4-sp-d16 \
  -Os \
  -fdata-sections \
  -ffunction-sections \
  -I. \
  -I/path/to/flipperzero-firmware/lib \
  -o build/subghz_toolkit.elf \
  subghz_toolkit.c \
  -L/path/to/flipperzero-firmware/lib \
  -lfuri -lgui -lstorage -lnotification -lsubghz \
  -Wl,--gc-sections -Wl,--strip-all

# Strip and convert to binary
arm-none-eabi-strip build/subghz_toolkit.elf
arm-none-eabi-objcopy -O binary build/subghz_toolkit.elf build/subghz_toolkit.fap
```

## Installing to Flipper Zero

### Automatic Installation

The build scripts will automatically attempt to install the FAP to your connected Flipper Zero if it's mounted as a USB drive.

### Manual Installation

1. **Connect your Flipper Zero** to your computer via USB
2. **Enable USB Mass Storage** on your Flipper Zero:
   - Go to Settings → System → USB → USB Mass Storage
3. **Copy the FAP file** to your Flipper Zero:
   ```bash
   # The FAP should be copied to the apps/SubGhz/ directory
   cp build/subghz_toolkit.fap /path/to/FLIPPER/apps/SubGhz/
   ```

### Installation Locations

The FAP file should be placed in one of these directories on your Flipper Zero:
- `apps/SubGhz/subghz_toolkit.fap` (recommended)
- `apps/SubGhz/` (any subdirectory)

## Troubleshooting

### Common Issues

1. **"arm-none-eabi-gcc not found"**
   - Install the ARM GCC toolchain (see Prerequisites section)

2. **"Flipper SDK not found"**
   - Set the `FLIPPER_SDK_PATH` environment variable
   - Or install the full Flipper Zero firmware

3. **"Permission denied"**
   - Make sure the build scripts are executable: `chmod +x build*.sh`

4. **"Library not found"**
   - Make sure you have the correct paths to the Flipper Zero libraries
   - The full SDK method is more reliable for this

5. **"Flipper Zero not detected"**
   - Make sure USB Mass Storage is enabled on your Flipper Zero
   - Check if the device is mounted at `/run/media/$(whoami)/FLIPPER` or `/media/$(whoami)/FLIPPER`

### Build Verification

After building, you should have these files in the `build/` directory:
- `subghz_toolkit.elf` - ELF binary
- `subghz_toolkit.bin` - Raw binary
- `subghz_toolkit.fap` - Flipper Application Package

### Testing the Installation

1. **Disconnect your Flipper Zero** from USB
2. **Navigate to Applications** on your Flipper Zero
3. **Find "SubGhz" category**
4. **Launch "SubGhz Toolkit"**

## File Structure

```
subghz-toolkit/
├── application.fam          # Application manifest
├── subghz_toolkit.c        # Main source code
├── subghz_toolkit.png      # Application icon
├── build.sh                # Full SDK build script
├── build_simple.sh         # Basic toolchain build script
├── Makefile                # Alternative build method
├── BUILD_INSTRUCTIONS.md   # This file
├── README.md              # Project description
└── build/                 # Build output directory
    ├── subghz_toolkit.elf
    ├── subghz_toolkit.bin
    └── subghz_toolkit.fap
```

## Support

If you encounter issues:
1. Check the troubleshooting section above
2. Make sure you have the latest version of the Flipper Zero firmware
3. Try the full SDK method if the basic toolchain doesn't work
4. Check the Flipper Zero documentation: https://docs.flipperzero.one/

## License

This project is licensed under the same terms as the Flipper Zero firmware.