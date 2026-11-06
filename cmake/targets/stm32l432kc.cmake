# STM32L432KC Target Configuration
# This file defines the target-specific settings for STM32L432KC

# Target identification
set(TARGET_MCU "STM32L432KC")
set(MCU_FAMILY "STM32L4xx")
set(MCU_DEVICE "STM32L432xx")
set(MCU_DEVICE_LOWER "stm32l432kcux")
set(MCU_FAMILY_LOWER "stm32l4xx")
set(TARGET_SERIES "STM32L4")
set(TARGET_BOARD "NUCLEO-L432KC")

# MCU specific definitions
set(MCU_DEFINES 
    STM32L432xx
    USE_HAL_DRIVER
    TARGET_STM32L432KC
)

# Compiler flags for Cortex-M4
set(MCU_ARCH_FLAGS 
    -mcpu=cortex-m4 
    -mthumb 
    -mfpu=fpv4-sp-d16 
    -mfloat-abi=hard
)

# Memory layout
set(FLASH_SIZE "256K")
set(RAM_SIZE "64K")
set(FLASH_ORIGIN "0x08000000")
set(RAM_ORIGIN "0x20000000")

# Memory layout
set(FLASH_SIZE "256K")
set(RAM_SIZE "64K")

# Linker script
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/linker/STM32L432KCUX_FLASH.ld")

# Compiler flags for Cortex-M4

# Startup and system files
set(STARTUP_FILE "${CMAKE_SOURCE_DIR}/startup/Src/startup_stm32l432kcux.s")
set(SYSTEM_FILE "${CMAKE_SOURCE_DIR}/startup/Src/system_stm32l4xx.c")

# HAL source path
set(HAL_SOURCE_PATH "${CMAKE_SOURCE_DIR}/third_party/STM32CubeL4/Drivers/STM32L4xx_HAL_Driver")
set(CMSIS_DEVICE_PATH "${CMAKE_SOURCE_DIR}/third_party/STM32CubeL4/Drivers/CMSIS/Device/ST/STM32L4xx")
set(CMSIS_CORE_PATH "${CMAKE_SOURCE_DIR}/third_party/STM32CubeL4/Drivers/CMSIS/Include")

# FreeRTOS port
set(FREERTOS_PORT "GCC/ARM_CM4F")
set(FREERTOS_PATH "${CMAKE_SOURCE_DIR}/third_party/STM32CubeL4/Middlewares/Third_Party/FreeRTOS")

# BSP source files
set(BSP_SOURCES
    "${CMAKE_SOURCE_DIR}/src/bsp/stm32l4xx/bsp_stm32l432kc.cpp"
)

# HAL modules to include (can be customized per target)
set(HAL_MODULES
    hal
    hal_adc
    hal_can
    hal_comp
    hal_cortex
    hal_crc
    hal_cryp
    hal_dac
    hal_dfsdm
    hal_dma
    hal_exti
    hal_flash
    hal_gpio
    hal_i2c
    hal_irda
    hal_iwdg
    hal_lcd
    hal_lptim
    hal_nand
    hal_nor
    hal_opamp
    hal_pcd
    hal_pwr
    hal_qspi
    hal_rcc
    hal_rng
    hal_rtc
    hal_sai
    hal_sd
    hal_smartcard
    hal_smbus
    hal_spi
    hal_sram
    hal_swpmi
    hal_tim
    hal_tsc
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
    "${CMAKE_SOURCE_DIR}/src/bsp/stm32l4xx"
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
set(OPENOCD_TARGET "stm32l4x")
set(OPENOCD_CONFIG 
    "-f interface/${OPENOCD_INTERFACE}.cfg"
    "-f target/${OPENOCD_TARGET}.cfg"
)

# Debug probe settings
set(DEBUG_PROBE_TYPE "ST-Link")
set(DEBUG_PROBE_SPEED "4000")  # kHz
