# cmake_stm32_template

[![CI/CD](https://github.com/rdmeneze/cmake_stm32_template/actions/workflows/ci.yml/badge.svg)](https://github.com/rdmeneze/cmake_stm32_template/actions/workflows/ci.yml)
[![Firmware Build](https://img.shields.io/badge/firmware-STM32L432KC-blue)]()
[![FreeRTOS](https://img.shields.io/badge/FreeRTOS-v10.3.1-green)]()
[![Google Test](https://img.shields.io/badge/tests-GoogleTest-orange)]()
[![ARM GCC](https://img.shields.io/badge/toolchain-ARM%20GCC%2014.2-red)]()

# TODO

- [x] move third_party/STM32CubeL4 to use https://github.com/STMicroelectronics/STM32CubeL4/blob/master/README.md as submodule
- [x] add freertos
- [x] add googletest
- [x] mock hardware functions
- [x] add CI/CD

# Usage
## 1. Clone the repository
```bash
    git clone https://github.com/rdmeneze/cmake_stm32_template.git
```

## 2. Initialize submodules
```bash
    git submodule update --init --recursive
```

## 3. Build the project

### With Docker (no toolchain needed on the machine)
```bash
    ./docker/build.sh                          # STM32L432KC, Debug
    ./docker/build.sh -t stm32f401vc --demo    # STM32F401C-DISCO, crash demo
    ./docker/build.sh -t stm32f407vg -b Release
    ./docker/build.sh --tests                  # host unit tests
```
The build image (Ubuntu 24.04 + Arm GNU Toolchain 14.2) is built on first use.
The output goes to `build/<target>-<build_type>[-demo]` and is identical to a
native build. Flashing runs on the host (it needs the USB probe).

### Native
Requires the Arm GNU Toolchain 14.2 in `/opt` or in `ARM_GCC_TOOLCHAIN_PATH`.
```bash
    cmake -B build -GNinja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-gcc-toolchain.cmake \
          -DTARGET_BOARD=stm32l432kc   # or stm32f401vc, stm32f407vg
    cmake --build build
```

## 4. Quick Start
```bash
# Build firmware
cmake --build build --config Release

# Run tests
./run_tests.sh

# Flash firmware (J-Link or ST-Link, detected automatically)
./flash_firmware.sh -t stm32l432kc build/stm32l432kc-debug/NUCLEO-L432KC_firmware

# Debug
./debug_firmware.sh
```
