#!/bin/bash

# Manual debug script for STM32L432KC
# Usage: ./debug_firmware.sh

set -e

echo "STM32L432KC Debug Script"
echo "=========================="

# Verify if firmware exists
FIRMWARE_PATH="${1:-build/NUCLEO-L432KC_firmware}"
if [ ! -f "$FIRMWARE_PATH" ]; then
    echo "Firmware not found at: $FIRMWARE_PATH"
    echo "Please run: cmake --build build --config Debug"
    exit 1
fi

echo "Firmware: $FIRMWARE_PATH"

# verify GDB ARM
GDB_PATH="/opt/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi/bin/arm-none-eabi-gdb"
if [ ! -f "$GDB_PATH" ]; then
    echo "GDB ARM not found at: $GDB_PATH"
    echo "Please check the ARM toolchain installation"
    exit 1
fi

echo "🔧 GDB ARM: $GDB_PATH"

# Verify if OpenOCD is available
if ! command -v openocd &> /dev/null; then
    echo "OpenOCD not installed!"
    exit 1
fi

echo "Starting OpenOCD server..."

# Start OpenOCD on background
openocd -f interface/stlink.cfg -f target/stm32l4x.cfg &
OPENOCD_PID=$!

# Wait for OpenOCD to initialize
sleep 3

echo "Starting GDB debug session..."
echo ""
echo "Useful GDB commands:"
echo "   (gdb) break main          # Breakpoint at main function"
echo "   (gdb) continue           # Continue execution"
echo "   (gdb) step               # Step into"
echo "   (gdb) next               # Step over"
echo "   (gdb) info registers     # View registers"
echo "   (gdb) x/16x 0x20000000   # View RAM memory"
echo "   (gdb) quit               # Quit"
echo ""

# Cleanup function
cleanup() {
    echo "Finalizing OpenOCD..."
    kill $OPENOCD_PID 2>/dev/null || true
    exit
}

# Trap for cleanup on exit
trap cleanup EXIT INT TERM

# Start GDB with automatic commands
$GDB_PATH \
    -ex "target extended-remote localhost:3333" \
    -ex "monitor reset init" \
    -ex "monitor halt" \
    -ex "load" \
    -ex "break main" \
    -ex "continue" \
    $FIRMWARE_PATH
