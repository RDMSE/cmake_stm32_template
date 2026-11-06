# STM32F407VG Target Configuration (Discovery board)
# This file defines the target-specific settings for STM32F407VG

# Target identification
set(TARGET_MCU "STM32F407VG")
set(MCU_FAMILY "STM32F4xx")
set(MCU_DEVICE "STM32F407xx")
set(MCU_DEVICE_LOWER "stm32f407vgtx")
set(MCU_FAMILY_LOWER "stm32f4xx")
set(TARGET_SERIES "STM32F4")
set(TARGET_BOARD "STM32F4DISCOVERY")

# MCU specific definitions
set(MCU_DEFINES
    STM32F407xx
    USE_HAL_DRIVER
    TARGET_STM32F407VG
)

# Compiler flags for Cortex-M4
set(MCU_ARCH_FLAGS
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
)

# Memory layout
set(FLASH_SIZE "1024K")
set(RAM_SIZE "128K")
set(FLASH_ORIGIN "0x08000000")
set(RAM_ORIGIN "0x20000000")

# Linker script
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/linker/STM32F407VGTx_FLASH.ld")

# Startup and system files: used as shipped in STM32CubeF4
set(CMSIS_DEVICE_PATH "${CMAKE_SOURCE_DIR}/third_party/STM32CubeF4/Drivers/CMSIS/Device/ST/STM32F4xx")
set(STARTUP_FILE "${CMSIS_DEVICE_PATH}/Source/Templates/gcc/startup_stm32f407xx.s")
set(SYSTEM_FILE "${CMSIS_DEVICE_PATH}/Source/Templates/system_stm32f4xx.c")

# HAL source path
set(HAL_SOURCE_PATH "${CMAKE_SOURCE_DIR}/third_party/STM32CubeF4/Drivers/STM32F4xx_HAL_Driver")
set(CMSIS_CORE_PATH "${CMAKE_SOURCE_DIR}/third_party/STM32CubeF4/Drivers/CMSIS/Include")

# FreeRTOS port
set(FREERTOS_PORT "GCC/ARM_CM4F")
set(FREERTOS_PATH "${CMAKE_SOURCE_DIR}/third_party/STM32CubeF4/Middlewares/Third_Party/FreeRTOS")

# BSP source files
set(BSP_SOURCES
    "${CMAKE_SOURCE_DIR}/src/bsp/stm32f4xx/bsp_stm32f4_discovery.cpp"
)

# HAL modules to include
set(HAL_MODULES
    hal
    hal_adc
    hal_can
    hal_cortex
    hal_crc
    hal_dac
    hal_dma
    hal_exti
    hal_flash
    hal_gpio
    hal_i2c
    hal_i2s
    hal_irda
    hal_iwdg
    hal_ltdc
    hal_nand
    hal_nor
    hal_pccard
    hal_pcd
    hal_pwr
    hal_rcc
    hal_rng
    hal_rtc
    hal_sai
    hal_sd
    hal_sdram
    hal_smartcard
    hal_spi
    hal_sram
    hal_tim
    hal_uart
    hal_usart
    hal_wwdg
    ll_utils
)

# Include directories
set(TARGET_INCLUDE_DIRS
    "${HAL_SOURCE_PATH}/Inc"
    "${CMSIS_DEVICE_PATH}/Include"
    "${CMSIS_CORE_PATH}"
    "${CMAKE_SOURCE_DIR}/startup/Inc"
    "${CMAKE_SOURCE_DIR}/src/bsp/stm32f4xx"
    "${CMAKE_SOURCE_DIR}/src/bsp/common"
    "${CMAKE_SOURCE_DIR}/src/hal/interfaces"
    "${FREERTOS_PATH}/Source/include"
    "${FREERTOS_PATH}/Source/portable/${FREERTOS_PORT}"
)

# Target specific compile options
set(TARGET_COMPILE_OPTIONS
    ${MCU_ARCH_FLAGS}
    -Wall
    -Wextra
    -Wpedantic
    -Wno-unused-parameter
    -Wno-missing-field-initializers
    -ffunction-sections
    -fdata-sections
)

# Target specific link options
set(TARGET_LINK_OPTIONS
    ${MCU_ARCH_FLAGS}
    -T${LINKER_SCRIPT}
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    -Wl,-Map=${CMAKE_BINARY_DIR}/${PROJECT_NAME}.map
    --specs=nano.specs
    --specs=nosys.specs
)

# OpenOCD configuration
set(OPENOCD_INTERFACE "stlink")
set(OPENOCD_TARGET "stm32f4x")
set(OPENOCD_CONFIG
    "-f interface/${OPENOCD_INTERFACE}.cfg"
    "-f target/${OPENOCD_TARGET}.cfg"
)

# Debug probe settings
set(DEBUG_PROBE_TYPE "ST-Link")
set(DEBUG_PROBE_SPEED "4000")  # kHz
