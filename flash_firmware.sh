#!/bin/bash

# Script to flash the firmware on STM32L432KC via ST-Link
# Usage: ./flash_firmware.sh [path/to/firmware.elf]


set -e

echo " STM32L432KC Firmware Flash Script"
echo "===================================="

# verify if firmware exists
FIRMWARE_PATH="${1:-build/NUCLEO-L432KC_firmware}"
if [ ! -f "$FIRMWARE_PATH" ]; then
    echo " Firmware not found at: $FIRMWARE_PATH"
    echo " Please run: cmake --build build --config Debug"
    exit 1
fi

echo " Firmware found: $FIRMWARE_PATH"
echo " Size: $(ls -lh $FIRMWARE_PATH | awk '{print $5}')"

# Verify if OpenOCD is available
if ! command -v openocd &> /dev/null; then
    echo " OpenOCD is not installed!"
    echo " Install with: sudo apt install openocd"
    exit 1
fi

echo "🔗 Connecting to STM32L432KC via ST-Link..."

# Flash the firmware
openocd \
    -f interface/stlink.cfg \
    -f target/stm32l4x.cfg \
    -c "program $FIRMWARE_PATH verify reset exit"

if [ $? -eq 0 ]; then
    echo " Firmware flashed successfully!"
    echo " The microcontroller has been reset and is running the new firmware"
else
    echo "❌ Error flashing firmware"
    exit 1
fi
