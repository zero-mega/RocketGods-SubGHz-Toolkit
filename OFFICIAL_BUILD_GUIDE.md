# Official Flipper Zero External App Build Guide

This guide follows the [official Flipper Zero documentation](https://developer.flipper.net/flipperzero/doxygen/apps_on_sd_card.html) for building external applications that can be installed on the SD card.

## Overview

The SubGhz Toolkit is configured as an **external application** (`FlipperAppType.EXTERNAL`) which means:
- It can be installed on the SD card
- It doesn't require firmware modification
- It can be easily distributed and installed

## Prerequisites

### 1. Install Flipper Zero Firmware Development Environment

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
export FLIPPER_FIRMWARE_PATH=$(pwd)
```

### 2. Verify Installation

```bash
# Check if Flipper Build Tool is available
ls -la $FLIPPER_FIRMWARE_PATH/scripts/fbt

# Should show the fbt script
```

## Building the Application

### Method 1: Using the Official Build Script (Recommended)

```bash
# Navigate to your project directory
cd /path/to/subghz-toolkit

# Set the firmware path (if not already set)
export FLIPPER_FIRMWARE_PATH=/path/to/flipperzero-firmware

# Build using the official script
./build_official.sh
```

### Method 2: Manual Build Using FBT

```bash
# Navigate to the firmware directory
cd $FLIPPER_FIRMWARE_PATH

# Build the external app
./scripts/fbt fap_dist -p subghz_toolkit -d /path/to/your/project
```

### Method 3: Using the Flipper Zero CLI

If you have the Flipper Zero CLI installed:

```bash
# Install the CLI (if not already installed)
pip install flipperzero-cli

# Build the app
flipper build fap --app-dir /path/to/subghz-toolkit
```

## Application Configuration

The `application.fam` file is already correctly configured:

```python
App(
    appid="subghz_toolkit",                    # Unique app identifier
    name="SubGhz Toolkit",                     # Display name
    apptype=FlipperAppType.EXTERNAL,           # External app (SD card installable)
    entry_point="subghz_toolkit_app",          # Main function name
    requires=[                                  # Required system services
        "gui",
        "storage", 
        "notification",
        "subghz",
    ],
    stack_size=8 * 1024,                       # Stack size in bytes
    fap_icon="subghz_toolkit.png",             # App icon
    fap_category="SubGhz",                     # App category
    fap_author="RocketGod",                    # Author name
    fap_weburl="https://betaskynet.com",       # Author website
    fap_version=(1, 0),                        # Version number
    fap_description="Extract SubGhz protocol implementation details and decrypt KeeLoq keys",
)
```

## Installation

### Automatic Installation

The build script will automatically attempt to install the FAP to your connected Flipper Zero.

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

## File Structure

```
subghz-toolkit/
├── application.fam              # Application manifest (required)
├── subghz_toolkit.c            # Main source code (required)
├── subghz_toolkit.png          # Application icon (required)
├── build_official.sh           # Official build script
├── build.sh                    # Alternative build script
├── build_simple.sh             # Basic toolchain build script
├── Makefile                    # Alternative build method
├── OFFICIAL_BUILD_GUIDE.md     # This file
├── BUILD_INSTRUCTIONS.md       # Detailed build instructions
├── README.md                   # Project description
└── build/                      # Build output directory
    └── subghz_toolkit.fap      # Flipper Application Package
```

## Troubleshooting

### Common Build Issues

1. **"Flipper firmware not found"**
   ```bash
   # Set the firmware path
   export FLIPPER_FIRMWARE_PATH=/path/to/flipperzero-firmware
   ```

2. **"fbt not found"**
   ```bash
   # Make sure you're in the firmware directory
   cd $FLIPPER_FIRMWARE_PATH
   ls -la scripts/fbt
   ```

3. **"Permission denied"**
   ```bash
   # Make build scripts executable
   chmod +x build*.sh
   ```

4. **"Directory name doesn't match appid"**
   - The directory name should match the `appid` in `application.fam`
   - Rename your directory to `subghz_toolkit` if needed

### Common Installation Issues

1. **"Flipper Zero not detected"**
   - Make sure USB Mass Storage is enabled on your Flipper Zero
   - Check if the device is mounted at `/run/media/$(whoami)/FLIPPER` or `/media/$(whoami)/FLIPPER`

2. **"App doesn't appear"**
   - Make sure the FAP is in the correct directory (`apps/SubGhz/`)
   - Check that the file extension is `.fap`
   - Restart your Flipper Zero

## Development Workflow

### 1. Make Changes
Edit the source code in `subghz_toolkit.c`

### 2. Build
```bash
./build_official.sh
```

### 3. Install
The build script will automatically install to your connected Flipper Zero

### 4. Test
Disconnect your Flipper Zero and test the application

### 5. Iterate
Repeat the process for further development

## Distribution

To distribute your FAP:

1. **Build the application** using the official method
2. **Share the `.fap` file** with others
3. **Users can install** by copying the `.fap` file to their Flipper Zero's `apps/SubGhz/` directory

## Official Documentation References

- [Apps on SD Card](https://developer.flipper.net/flipperzero/doxygen/apps_on_sd_card.html)
- [Flipper Build Tool](https://docs.flipperzero.one/development/firmware/fbt)
- [Application Development](https://docs.flipperzero.one/development/application-development)

## Support

If you encounter issues:
1. Check the troubleshooting section above
2. Make sure you have the latest version of the Flipper Zero firmware
3. Check the official Flipper Zero documentation
4. Verify your `application.fam` configuration matches the official format