# FreeRTOS sources configuration for STM32L432KC (Cortex-M4F)

set(FREERTOS_DIR ${CMAKE_SOURCE_DIR}/third_party/STM32CubeL4/Middlewares/Third_Party/FreeRTOS/Source)

# FreeRTOS core sources
set(FREERTOS_SOURCES
    ${FREERTOS_DIR}/tasks.c
    ${FREERTOS_DIR}/queue.c
    ${FREERTOS_DIR}/list.c
    ${FREERTOS_DIR}/timers.c
    ${FREERTOS_DIR}/event_groups.c
    ${FREERTOS_DIR}/stream_buffer.c
    ${FREERTOS_DIR}/croutine.c
)

# FreeRTOS portable layer for ARM Cortex-M4F with GCC
set(FREERTOS_PORT_SOURCES
    ${FREERTOS_DIR}/portable/GCC/ARM_CM4F/port.c
    ${FREERTOS_DIR}/portable/MemMang/heap_4.c
)

# FreeRTOS CMSIS-RTOS wrapper (optional, disabled for now due to configuration issues)
# set(FREERTOS_CMSIS_SOURCES
#     ${FREERTOS_DIR}/CMSIS_RTOS_V2/cmsis_os2.c
# )

# FreeRTOS include directories
set(FREERTOS_INCLUDES
    ${FREERTOS_DIR}/include
    ${FREERTOS_DIR}/portable/GCC/ARM_CM4F
    # ${FREERTOS_DIR}/CMSIS_RTOS_V2  # Disabled for now
)

# Create FreeRTOS object library
add_library(freertos_sources OBJECT
    ${FREERTOS_SOURCES}
    ${FREERTOS_PORT_SOURCES}
    # ${FREERTOS_CMSIS_SOURCES}  # Disabled for now
)

# Set include directories for FreeRTOS
target_include_directories(freertos_sources PRIVATE
    ${FREERTOS_INCLUDES}
    ${CMAKE_SOURCE_DIR}/third_party/STM32CubeL4/Drivers/CMSIS/Device/ST/STM32L4xx/Include
    ${CMAKE_SOURCE_DIR}/third_party/STM32CubeL4/Drivers/CMSIS/Include
    ${CMAKE_SOURCE_DIR}/third_party/STM32CubeL4/Drivers/STM32L4xx_HAL_Driver/Inc
    ${STARTUP_DIR}/Inc
    ${SRC_DIR}
    # FreeRTOSConfig.h maps configASSERT to emblib_assert
    ${CMAKE_SOURCE_DIR}/third_party/emb_lib/modules
)

# Compilation options for FreeRTOS
target_compile_options(freertos_sources PRIVATE
    ${COMMON_FLAGS}
    -Wall
    -Wextra
    -ffunction-sections
    -fdata-sections
)

# Compilation definitions for FreeRTOS
target_compile_definitions(freertos_sources PRIVATE
    STM32L432xx
    USE_HAL_DRIVER
)

message(STATUS "FreeRTOS sources configured")
message(STATUS "FreeRTOS DIR = ${FREERTOS_DIR}")
