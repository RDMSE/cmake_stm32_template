/**
 * @file portable_led_example.cpp
 * @brief Portable LED Blink Example
 * 
 * This example demonstrates how to write portable application code
 * that works across different STM32 microcontrollers using the
 * BSP and HAL abstraction layers.
 */

extern "C" {
#include "bsp.h"
#include "hal_gpio.h"
#include "hal_rcc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "crash_diag.h"
}

#ifdef CRASH_DEMO
#include "crash_demo.h"
#endif

#include <cstring>

// Application configuration
#define LED_TASK_STACK_SIZE     256
#define LED_TASK_PRIORITY       1
#define LED_BLINK_DELAY_MS      500

// Global variables
static TaskHandle_t led_task_handle = nullptr;
static hal_gpio_port_t* led_port = nullptr;
static const bsp_pin_config_t* led_pin_config = nullptr;

/**
 * @brief Portable LED initialization function
 * 
 * This function finds and configures the user LED pin using BSP,
 * making it portable across different boards.
 */
static int portable_led_init(void) {
    // Get LED pin configuration from BSP
    led_pin_config = bsp_get_pin_config_by_function(BSP_PIN_FUNCTION_LED, 0);
    if (!led_pin_config) {
        // No LED defined in BSP - application can't continue
        return -1;
    }

    // Get GPIO port handle
    led_port = hal_gpio_get_port(led_pin_config->port_name);
    if (!led_port) {
        return -2;
    }

    // Enable GPIO port clock
    if (hal_gpio_enable_port_clock(led_port) != 0) {
        return -3;
    }

    // Configure LED pin
    hal_gpio_init_t gpio_config = {0};
    gpio_config.pin = (1U << led_pin_config->pin);
    gpio_config.mode = HAL_GPIO_MODE_OUTPUT_PP;
    gpio_config.pull = HAL_GPIO_NOPULL;
    gpio_config.speed = HAL_GPIO_SPEED_LOW;
    gpio_config.alternate = 0;

    if (hal_gpio_init(led_port, &gpio_config) != 0) {
        return -4;
    }

    // Set initial LED state (off)
    hal_gpio_pin_state_t initial_state = led_pin_config->active_high ? 
                                         HAL_GPIO_PIN_RESET : HAL_GPIO_PIN_SET;
    hal_gpio_write_pin(led_port, (1U << led_pin_config->pin), initial_state);

    return 0;
}

/**
 * @brief Portable LED control function
 * 
 * @param state true to turn LED on, false to turn off
 */
static void portable_led_set(bool state) {
    if (!led_port || !led_pin_config) {
        return;
    }

    hal_gpio_pin_state_t gpio_state;
    
    // Handle active high/low logic
    if (led_pin_config->active_high) {
        gpio_state = state ? HAL_GPIO_PIN_SET : HAL_GPIO_PIN_RESET;
    } else {
        gpio_state = state ? HAL_GPIO_PIN_RESET : HAL_GPIO_PIN_SET;
    }

    hal_gpio_write_pin(led_port, (1U << led_pin_config->pin), gpio_state);
}

/**
 * @brief Portable LED toggle function
 */
static void portable_led_toggle(void) {
    if (!led_port || !led_pin_config) {
        return;
    }

    hal_gpio_toggle_pin(led_port, (1U << led_pin_config->pin));
}

/**
 * @brief LED task - blinks the user LED
 */
static void led_task(void* pvParameters) {
    (void)pvParameters;
    
    bool led_state = false;

    while (1) {
        // Toggle LED state
        led_state = !led_state;
        portable_led_set(led_state);

        // Wait for specified delay
        vTaskDelay(pdMS_TO_TICKS(LED_BLINK_DELAY_MS));
    }
}

/**
 * @brief Alternative LED task using toggle function
 */
static void led_toggle_task(void* pvParameters) {
    (void)pvParameters;

    while (1) {
        // Simply toggle the LED
        portable_led_toggle();

        // Wait for specified delay
        vTaskDelay(pdMS_TO_TICKS(LED_BLINK_DELAY_MS));
    }
}

/**
 * @brief Print system information (portable across targets)
 */
static void print_system_info(void) {
    const bsp_config_t* bsp_config = bsp_get_config();
    
    // In a real application, you'd use printf or a logging system
    // Here we just show how to access portable system information
    const char* board_name = bsp_get_board_name();
    const char* mcu_name = bsp_get_mcu_name();
    bsp_mcu_family_t family = bsp_get_mcu_family();
    
    (void)board_name;  // Suppress unused variable warnings
    (void)mcu_name;
    (void)family;
    (void)bsp_config;
    
    // Example: Debug output would be:
    // printf("Running on: %s board with %s MCU\n", board_name, mcu_name);
    // printf("Flash: %lu KB, RAM: %lu KB\n", bsp_config->flash_size_kb, bsp_config->ram_size_kb);
}

/**
 * @brief Application initialization
 * 
 * This function demonstrates portable application setup.
 */
int portable_app_init(void) {
    // Initialize BSP (board-specific)
    if (bsp_init() != 0) {
        return -1;
    }

    // Report what made the previous run reset, now that the debug UART is up
    crash_diag_report_previous();

    // Print system information
    print_system_info();

    // Initialize LED hardware
    if (portable_led_init() != 0) {
        return -2;
    }

    // Create LED task
    BaseType_t result = xTaskCreate(
        led_toggle_task,        // Task function
        "LED_Task",             // Task name
        LED_TASK_STACK_SIZE,    // Stack size
        NULL,                   // Parameters
        LED_TASK_PRIORITY,      // Priority
        &led_task_handle        // Task handle
    );

    if (result != pdPASS) {
        return -3;
    }

#ifdef CRASH_DEMO
    if (crash_demo_start() != 0) {
        return -4;
    }
#endif

    return 0;
}

/**
 * @brief Main application entry point
 * 
 * This main function is portable across all supported targets.
 */
extern "C" int main(void) {
    // First thing: catch faults and asserts from here on, even during init
    crash_diag_init();

    // Initialize portable application
    if (portable_app_init() != 0) {
        // Error handling - could flash LED in pattern to indicate error
        while (1) {
            // Error state - halt execution
        }
    }

    // Start FreeRTOS scheduler
    vTaskStartScheduler();

    // Should never reach here
    while (1) {
        // Scheduler failed to start
    }

    return 0;
}
