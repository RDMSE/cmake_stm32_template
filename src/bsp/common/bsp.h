#pragma once

/**
 * @file bsp.h
 * @brief Board Support Package Interface
 * 
 * This file defines the common BSP interface that must be implemented
 * by each target MCU/board combination.
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Forward declarations
typedef struct bsp_config bsp_config_t;
typedef struct bsp_pin_config bsp_pin_config_t;
typedef struct bsp_clock_config bsp_clock_config_t;

/**
 * @brief MCU Family Types
 */
typedef enum {
    BSP_MCU_FAMILY_STM32L4XX,
    BSP_MCU_FAMILY_STM32F4XX,
    BSP_MCU_FAMILY_STM32H7XX,
    BSP_MCU_FAMILY_STM32G4XX,
    BSP_MCU_FAMILY_STM32WB55,
    BSP_MCU_FAMILY_UNKNOWN
} bsp_mcu_family_t;

/**
 * @brief Pin Function Types
 */
typedef enum {
    BSP_PIN_FUNCTION_LED,
    BSP_PIN_FUNCTION_BUTTON,
    BSP_PIN_FUNCTION_UART_TX,
    BSP_PIN_FUNCTION_UART_RX,
    BSP_PIN_FUNCTION_SPI_SCK,
    BSP_PIN_FUNCTION_SPI_MISO,
    BSP_PIN_FUNCTION_SPI_MOSI,
    BSP_PIN_FUNCTION_SPI_CS,
    BSP_PIN_FUNCTION_I2C_SCL,
    BSP_PIN_FUNCTION_I2C_SDA,
    BSP_PIN_FUNCTION_DEBUG_TX,
    BSP_PIN_FUNCTION_DEBUG_RX,
    BSP_PIN_FUNCTION_CUSTOM
} bsp_pin_function_t;

/**
 * @brief Pin Configuration Structure
 */
struct bsp_pin_config {
    const char* name;           // Pin name (e.g., "LED_USER", "BUTTON_1")
    const char* port_name;      // GPIO port (e.g., "GPIOB")
    uint16_t pin;              // Pin number
    bsp_pin_function_t function; // Pin function
    uint32_t alternate_function; // AF number (if applicable)
    bool active_high;          // true if active high, false if active low
};

/**
 * @brief Clock Configuration Structure
 */
struct bsp_clock_config {
    uint32_t hsi_frequency_hz;
    uint32_t hse_frequency_hz;
    uint32_t lsi_frequency_hz;
    uint32_t lse_frequency_hz;
    uint32_t target_sysclk_hz;
    uint32_t target_hclk_hz;
    uint32_t target_pclk1_hz;
    uint32_t target_pclk2_hz;
    bool hse_bypass;
    bool lse_bypass;
};

/**
 * @brief BSP Configuration Structure
 */
struct bsp_config {
    const char* board_name;
    const char* mcu_name;
    bsp_mcu_family_t mcu_family;
    uint32_t flash_size_kb;
    uint32_t ram_size_kb;
    uint32_t pin_count;
    const bsp_pin_config_t* pins;
    const bsp_clock_config_t* clock_config;
    void* mcu_specific_data;
};

// =============================================================================
// BSP API Functions (must be implemented by each target)
// =============================================================================

/**
 * @brief Initialize the BSP
 * @return 0 on success, negative error code on failure
 */
int bsp_init(void);

/**
 * @brief Get BSP configuration
 * @return Pointer to BSP configuration structure
 */
const bsp_config_t* bsp_get_config(void);

/**
 * @brief Initialize system clocks
 * @return 0 on success, negative error code on failure
 */
int bsp_init_system_clock(void);

/**
 * @brief Initialize debug/console UART
 * @return 0 on success, negative error code on failure
 */
int bsp_init_debug_uart(void);

/**
 * @brief Write to the debug UART by polling its registers (no HAL, no interrupts)
 *
 * Safe to call from fault handlers. Data is dropped while the UART is not initialized.
 * @param data bytes to send
 * @param len number of bytes
 */
void bsp_debug_write(const char* data, size_t len);

/**
 * @brief Wait until the debug UART has sent everything (bounded wait)
 */
void bsp_debug_flush(void);

/**
 * @brief Cause of the last reset, read from the reset flags on the first call
 *        (the flags are cleared afterwards)
 * @return Static string describing the reset cause
 */
const char* bsp_get_reset_cause(void);

/**
 * @brief Get pin configuration by name
 * @param pin_name Pin name to search for
 * @return Pointer to pin configuration or NULL if not found
 */
const bsp_pin_config_t* bsp_get_pin_config(const char* pin_name);

/**
 * @brief Get pin configuration by function
 * @param function Pin function to search for
 * @param instance Instance number (0 for first match, 1 for second, etc.)
 * @return Pointer to pin configuration or NULL if not found
 */
const bsp_pin_config_t* bsp_get_pin_config_by_function(bsp_pin_function_t function, uint8_t instance);

/**
 * @brief Configure a pin according to BSP configuration
 * @param pin_config Pin configuration to apply
 * @return 0 on success, negative error code on failure
 */
int bsp_configure_pin(const bsp_pin_config_t* pin_config);

/**
 * @brief Get MCU family
 * @return MCU family type
 */
bsp_mcu_family_t bsp_get_mcu_family(void);

/**
 * @brief Get MCU name string
 * @return MCU name string
 */
const char* bsp_get_mcu_name(void);

/**
 * @brief Get board name string
 * @return Board name string
 */
const char* bsp_get_board_name(void);

/**
 * @brief Delay function (milliseconds)
 * @param ms Delay in milliseconds
 */
void bsp_delay_ms(uint32_t ms);

/**
 * @brief Get system tick count
 * @return Current system tick count
 */
uint32_t bsp_get_tick(void);

#ifdef __cplusplus
}
#endif
