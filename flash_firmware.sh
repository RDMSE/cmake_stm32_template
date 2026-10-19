#!/bin/bash

# Script to flash the firmware via ST-Link (OpenOCD) or SEGGER J-Link
#
# Usage: ./flash_firmware.sh [-t target] [-p probe] [path/to/firmware.elf]
#   -t target   stm32l432kc (default) or stm32f407vg
#   -p probe    auto (default), jlink or stlink
#               auto picks J-Link when a SEGGER probe is connected (including
#               an on-board ST-Link reflashed as J-Link OB), ST-Link otherwise
#
# Examples:
#   ./flash_firmware.sh
#   ./flash_firmware.sh -t stm32f407vg
#   ./flash_firmware.sh -p jlink build-demo/NUCLEO-L432KC_firmware


set -e

TARGET="stm32l432kc"
PROBE="auto"

while getopts "t:p:h" opt; do
    case $opt in
        t) TARGET="$OPTARG" ;;
        p) PROBE="$OPTARG" ;;
        *) sed -n '3,14p' "$0"; exit 1 ;;
    esac
done
shift $((OPTIND - 1))

# board name (firmware file name), J-Link device and OpenOCD target per target
case "$TARGET" in
    stm32l432kc)
        BOARD="NUCLEO-L432KC"
        JLINK_DEVICE="STM32L432KC"
        OPENOCD_TARGET="stm32l4x"
        ;;
    stm32f407vg)
        BOARD="STM32F4DISCOVERY"
        JLINK_DEVICE="STM32F407VG"
        OPENOCD_TARGET="stm32f4x"
        ;;
    *)
        echo " Unknown target: $TARGET (use stm32l432kc or stm32f407vg)"
        exit 1
        ;;
esac

echo " $JLINK_DEVICE Firmware Flash Script"
echo "===================================="

# verify if firmware exists
FIRMWARE_PATH="${1:-build/${BOARD}_firmware}"
if [ ! -f "$FIRMWARE_PATH" ]; then
    echo " Firmware not found at: $FIRMWARE_PATH"
    echo " Please run: cmake --build build --config Debug"
    exit 1
fi

echo " Firmware found: $FIRMWARE_PATH"
echo " Size: $(ls -lh "$FIRMWARE_PATH" | awk '{print $5}')"

if [ "$PROBE" = "auto" ]; then
    # 1366 is SEGGER's USB vendor ID
    if lsusb -d 1366: &> /dev/null; then
        PROBE="jlink"
    else
        PROBE="stlink"
    fi
fi

case "$PROBE" in
    jlink)
        if ! command -v JLinkExe &> /dev/null; then
            echo " JLinkExe is not installed!"
            echo " Install the J-Link Software Pack from https://www.segger.com/downloads/jlink/"
            exit 1
        fi

        echo "🔗 Connecting to $JLINK_DEVICE via J-Link..."

        JLINK_TMP=$(mktemp -d)
        trap 'rm -rf "$JLINK_TMP"' EXIT

        # J-Link picks the file format from the extension, and the ELF has none:
        # prefer the .hex generated next to it, otherwise link it as .elf.
        # A .bin carries no address, so it is not accepted here.
        case "$FIRMWARE_PATH" in
            *.hex|*.elf) JLINK_FILE="$FIRMWARE_PATH" ;;
            *.bin)
                echo " .bin files have no load address, use the ELF or the .hex"
                exit 1
                ;;
            *)
                if [ -f "$FIRMWARE_PATH.hex" ]; then
                    JLINK_FILE="$FIRMWARE_PATH.hex"
                else
                    JLINK_FILE="$JLINK_TMP/firmware.elf"
                    ln -s "$(realpath "$FIRMWARE_PATH")" "$JLINK_FILE"
                fi
                ;;
        esac

        printf 'loadfile %s\nr\ng\nexit\n' "$JLINK_FILE" > "$JLINK_TMP/flash.jlink"

        # JLinkExe returns 0 even when a command fails, so check that the
        # download itself answered O.K.
        JLINK_OUTPUT=$(JLinkExe -device "$JLINK_DEVICE" -if SWD -speed 4000 -autoconnect 1 \
                       -NoGui 1 -ExitOnError 1 -CommanderScript "$JLINK_TMP/flash.jlink" 2>&1) || true
        if ! sed -n '/^Downloading file/,/^J-Link>/p' <<< "$JLINK_OUTPUT" | grep -q "^O.K.$"; then
            echo "$JLINK_OUTPUT"
            echo "❌ Error flashing firmware"
            exit 1
        fi
        ;;
    stlink)
        if ! command -v openocd &> /dev/null; then
            echo " OpenOCD is not installed!"
            echo " Install with: sudo apt install openocd"
            exit 1
        fi

        echo "🔗 Connecting to $JLINK_DEVICE via ST-Link..."

        if ! openocd \
            -f interface/stlink.cfg \
            -f target/${OPENOCD_TARGET}.cfg \
            -c "program $FIRMWARE_PATH verify reset exit"; then
            echo "❌ Error flashing firmware"
            exit 1
        fi
        ;;
    *)
        echo " Unknown probe: $PROBE (use auto, jlink or stlink)"
        exit 1
        ;;
esac

echo " Firmware flashed successfully!"
echo " The microcontroller has been reset and is running the new firmware"
