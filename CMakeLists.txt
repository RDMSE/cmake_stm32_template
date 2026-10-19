cmake_minimum_required(VERSION 3.21)
set(CMAKE_VERBOSE_MAKEFILE ON)

# Target board selection
if(NOT DEFINED TARGET_BOARD)
    set(TARGET_BOARD "stm32l432kc" CACHE STRING "Target board selection")
    message(STATUS "No target board specified, defaulting to: ${TARGET_BOARD}")
endif()

message(STATUS "Building for target board: ${TARGET_BOARD}")

# Load target-specific configuration
set(TARGET_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/targets/${TARGET_BOARD}.cmake")
if(NOT EXISTS ${TARGET_CONFIG_FILE})
    message(FATAL_ERROR "Target configuration file not found: ${TARGET_CONFIG_FILE}")
endif()

include(${TARGET_CONFIG_FILE})

# Define project name based on target
project(${TARGET_BOARD}_firmware C CXX ASM)

# Standard language versions
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED OFF)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_CXX_EXTENSIONS OFF)

# Check for required STM32Cube library
if(MCU_FAMILY STREQUAL "STM32L4xx")
    set(STM32CUBE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/STM32CubeL4")
elseif(MCU_FAMILY STREQUAL "STM32F4xx")
    set(STM32CUBE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/STM32CubeF4")
else()
    message(FATAL_ERROR "Unsupported MCU family: ${MCU_FAMILY}")
endif()

if(NOT EXISTS "${STM32CUBE_DIR}")
    message(FATAL_ERROR "STM32Cube library not found at: ${STM32CUBE_DIR}
                         Please run: git submodule update --init --recursive")
endif()

# Version detection from Git
find_package(Git REQUIRED)

message(STATUS "Using Git executable: ${GIT_EXECUTABLE}")
message(STATUS "Using CMake version: ${CMAKE_VERSION}")

execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --tags --match "v*"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE GIT_RESULT
    OUTPUT_VARIABLE APP_VERSION
    ERROR_VARIABLE GIT_ERROR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# Fallback to short hash if no valid tag found
if(NOT APP_VERSION OR GIT_RESULT)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE APP_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    set(APP_VERSION "v0.0.0-${APP_VERSION}")
endif()

if(NOT APP_VERSION)
    set(APP_VERSION "v0.0.0-unknown")
    message(WARNING "Failed to determine version from Git. Using default: ${APP_VERSION}")
endif()

message(STATUS "App version: ${APP_VERSION}")

# Generate version header
configure_file(
    ${CMAKE_SOURCE_DIR}/version.h.in
    ${CMAKE_BINARY_DIR}/version.h
    @ONLY
)

add_custom_target(generate_version_header DEPENDS ${CMAKE_BINARY_DIR}/version.h)

# Directory structure
set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(HAL_DIR ${STM32CUBE_DIR}/Drivers/${MCU_FAMILY}_HAL_Driver)
set(CMSIS_DIR ${STM32CUBE_DIR}/Drivers/CMSIS)
set(STARTUP_DIR ${CMAKE_SOURCE_DIR}/startup)
set(FREERTOS_DIR ${STM32CUBE_DIR}/Middlewares/Third_Party/FreeRTOS/Source)

message(STATUS "Source directories:")
message(STATUS "  SRC_DIR = ${SRC_DIR}")
message(STATUS "  HAL_DIR = ${HAL_DIR}")
message(STATUS "  CMSIS_DIR = ${CMSIS_DIR}")
message(STATUS "  STARTUP_DIR = ${STARTUP_DIR}")
message(STATUS "  LINKER_SCRIPT = ${LINKER_SCRIPT}")

# Common compilation flags (from target configuration)
set(COMMON_FLAGS ${TARGET_COMPILE_OPTIONS})

# Include CMake modules (must be after defining flags)
include(${CMAKE_SOURCE_DIR}/cmake/hal_sources.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/freertos_sources.cmake)

# Family-independent headers for the code shared by all STM32 targets
# (hal/stm32, platform/cortex_m, ...): they forward to the family headers
file(CONFIGURE OUTPUT ${CMAKE_BINARY_DIR}/stm32_hal.h
     CONTENT "#pragma once\n#include \"${MCU_FAMILY_LOWER}_hal.h\"\n")
file(CONFIGURE OUTPUT ${CMAKE_BINARY_DIR}/stm32_device.h
     CONTENT "#pragma once\n#include \"${MCU_FAMILY_LOWER}.h\"\n")

# emb_lib (submodule): only the portable modules used by the firmware
set(EMBLIB_DIR ${CMAKE_SOURCE_DIR}/third_party/emb_lib)
set(EMBLIB_SOURCES
    ${EMBLIB_DIR}/modules/string_builder.c
    ${EMBLIB_DIR}/modules/emblib_assert.c
    ${EMBLIB_DIR}/modules/emblib_crash_record.c
)

# Crash demo: triggers one kind of crash per boot (see src/app/crash_demo.h)
option(CRASH_DEMO "Build the crash diagnostics demo" OFF)

# Include directories
set(HAL_INCLUDES
    ${HAL_DIR}/Inc
    ${STARTUP_DIR}/Inc
    ${CMSIS_DIR}/Device/ST/${MCU_FAMILY}/Include
    ${CMSIS_DIR}/Include
    ${CMSIS_DIR}/Core/Include
    ${CMAKE_BINARY_DIR}
    ${SRC_DIR}/hal/interfaces
    ${SRC_DIR}/hal/stm32
    ${SRC_DIR}/bsp/common
    ${SRC_DIR}/app
    ${SRC_DIR}/platform/cortex_m
    ${EMBLIB_DIR}/modules
    ${FREERTOS_INCLUDES}
)

# Debug/Release specific flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND COMMON_FLAGS -O0 -g3)
    list(APPEND MCU_DEFINES DEBUG)
else()
    list(APPEND COMMON_FLAGS -Os -g0)
    list(APPEND MCU_DEFINES NDEBUG)
endif()

# Common flags for all builds
list(APPEND COMMON_FLAGS
    -Wall
    -Wextra
    -Wpedantic
    -Wno-unused-parameter
    -fdata-sections
    -ffunction-sections
)

# Apply flags to both C and CXX
add_compile_options(${COMMON_FLAGS})

# Apply MCU definitions with proper -D prefixes
foreach(define IN LISTS MCU_DEFINES)
    add_compile_options(-D${define})
endforeach()

# Linker flags
set(LINKER_FLAGS
    ${COMMON_FLAGS}
    -T${LINKER_SCRIPT}
    -Wl,-Map=${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.map,--cref
    -Wl,--gc-sections
    --specs=nano.specs
)

string(JOIN " " LINKER_FLAGS_STR ${LINKER_FLAGS})
set(CMAKE_EXE_LINKER_FLAGS ${LINKER_FLAGS_STR})

# CMake modules already included above

# Collect source files
set(APP_SOURCES
    ${SRC_DIR}/app/portable_led_example.cpp
    ${SRC_DIR}/app/freertos_it.c
    ${SRC_DIR}/hal/stm32/hal_gpio_stm32.c
    ${SRC_DIR}/bsp/common/bsp_common.cpp
    ${SRC_DIR}/platform/cortex_m/crash_diag.c
    ${SRC_DIR}/platform/cortex_m/crash_diag_freertos.c
    ${SRC_DIR}/syscalls.c
    ${EMBLIB_SOURCES}
)

if(CRASH_DEMO)
    list(APPEND APP_SOURCES ${SRC_DIR}/app/crash_demo.cpp)
    # the demo relies on resets: never stop on a breakpoint
    add_compile_definitions(CRASH_DEMO CRASH_DIAG_BREAK_ON_FAULT=0)
endif()

# Add BSP source (from target configuration)
list(APPEND APP_SOURCES ${BSP_SOURCES})

set(ALL_SOURCES
    ${APP_SOURCES}
    ${HAL_SOURCES}
    ${FREERTOS_SOURCES}
    ${STARTUP_FILE}
    ${SYSTEM_FILE}
)

# Create executable
add_executable(${CMAKE_PROJECT_NAME} ${ALL_SOURCES})

# Include directories
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${HAL_INCLUDES})

# Dependencies
add_dependencies(${CMAKE_PROJECT_NAME} generate_version_header)

# Generate binary and hex files
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.hex
    COMMENT "Generating binary and hex files"
)

# Print memory usage
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
    COMMENT "Memory usage:"
)

# Build summary
message(STATUS "Build configuration summary:")
message(STATUS "  Target Board: ${TARGET_BOARD}")
message(STATUS "  MCU Family: ${MCU_FAMILY}")
message(STATUS "  MCU Device: ${MCU_DEVICE}")
message(STATUS "  Flash Size: ${FLASH_SIZE}")
message(STATUS "  RAM Size: ${RAM_SIZE}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  HAL Modules: ${HAL_MODULES}")

# Testing support (only if not cross-compiling)
if(NOT CMAKE_CROSSCOMPILING)
    enable_testing()
    add_subdirectory(tests)
endif()
