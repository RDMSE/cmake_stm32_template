/**
 * @file bsp_stm32l432kc.cpp
 * @brief Board Support Package for STM32L432KC (Nucleo-32)
 * 
 * This file implements the BSP interface for the STM32L432KC microcontroller
 * as used on the Nucleo-L432KC development board.
 */

#include "../common/bsp.h"
#include "../../hal/interfaces/hal_gpio.h"
#include "../../hal/interfaces/hal_rcc.h"

#include <cstring>

extern "C" {
#include "stm32l4xx_hal.h"
#include "stm32l4xx_hal_rcc.h"
#include "stm32l4xx_hal_gpio.h"
#include "emblib_assert.h"
}

#define DEBUG_UART              USART2
#define DEBUG_UART_BAUDRATE     115200U
// bounded busy-wait: a byte at 115200 baud takes ~87 us, this is well above it
#define DEBUG_UART_TIMEOUT      100000U

// =============================================================================
// Pin Configuration for STM32L432KC Nucleo-32
// =============================================================================

static const bsp_pin_config_t stm32l432kc_pins[] = {
    // User LED (LD3) - Green LED on PB3
    {
        .name = "LED_USER",
        .port_name = "GPIOB",
        .pin = 3,
        .function = BSP_PIN_FUNCTION_LED,
        .alternate_function = 0,
        .active_high = true
    },
    
    // Debug UART (Virtual COM Port via ST-Link)
    {
        .name = "DEBUG_TX",
        .port_name = "GPIOA",
        .pin = 2,
        .function = BSP_PIN_FUNCTION_DEBUG_TX,
        .alternate_function = 7,  // AF7: USART2_TX
        .active_high = true
    },
    {
        .name = "DEBUG_RX", 
        .port_name = "GPIOA",
        .pin = 15,
        .function = BSP_PIN_FUNCTION_DEBUG_RX,
        .alternate_function = 3,  // AF3: USART2_RX
        .active_high = true
    },
    
    // SPI1 Pins (Arduino compatible)
    {
        .name = "SPI1_SCK",
        .port_name = "GPIOA",
        .pin = 5,
        .function = BSP_PIN_FUNCTION_SPI_SCK,
        .alternate_function = 5,  // AF5: SPI1_SCK
        .active_high = true
    },
    {
        .name = "SPI1_MISO",
        .port_name = "GPIOA",
        .pin = 6,
        .function = BSP_PIN_FUNCTION_SPI_MISO,
        .alternate_function = 5,  // AF5: SPI1_MISO
        .active_high = true
    },
    {
        .name = "SPI1_MOSI",
        .port_name = "GPIOA",
        .pin = 7,
        .function = BSP_PIN_FUNCTION_SPI_MOSI,
        .alternate_function = 5,  // AF5: SPI1_MOSI
        .active_high = true
    },
    
    // I2C1 Pins (Arduino compatible)
    {
        .name = "I2C1_SCL",
        .port_name = "GPIOB",
        .pin = 6,
        .function = BSP_PIN_FUNCTION_I2C_SCL,
        .alternate_function = 4,  // AF4: I2C1_SCL
        .active_high = true
    },
    {
        .name = "I2C1_SDA",
        .port_name = "GPIOB",
        .pin = 7,
        .function = BSP_PIN_FUNCTION_I2C_SDA,
        .alternate_function = 4,  // AF4: I2C1_SDA
        .active_high = true
    }
};

// =============================================================================
// Clock Configuration for STM32L432KC
// =============================================================================

static const bsp_clock_config_t stm32l432kc_clock_config = {
    .hsi_frequency_hz = 16000000,   // 16 MHz HSI
    .hse_frequency_hz = 0,          // No external crystal
    .lsi_frequency_hz = 32000,      // ~32 kHz LSI
    .lse_frequency_hz = 0,          // No external 32.768 kHz crystal
    .target_sysclk_hz = 80000000,   // 80 MHz system clock
    .target_hclk_hz = 80000000,     // 80 MHz AHB clock
    .target_pclk1_hz = 80000000,    // 80 MHz APB1 clock
    .target_pclk2_hz = 80000000,    // 80 MHz APB2 clock
    .hse_bypass = false,
    .lse_bypass = false
};

// =============================================================================
// BSP Configuration Structure
// =============================================================================

static const bsp_config_t stm32l432kc_config = {
    .board_name = "NUCLEO-L432KC",
    .mcu_name = "STM32L432KC",
    .mcu_family = BSP_MCU_FAMILY_STM32L4XX,
    .flash_size_kb = 256,           // 256 KB Flash
    .ram_size_kb = 64,              // 64 KB SRAM1
    .pin_count = sizeof(stm32l432kc_pins) / sizeof(stm32l432kc_pins[0]),
    .pins = stm32l432kc_pins,
    .clock_config = &stm32l432kc_clock_config,
    .mcu_specific_data = NULL
};

// =============================================================================
// BSP Implementation Functions
// =============================================================================

int bsp_init(void) {
    // Initialize HAL library
    if (HAL_Init() != HAL_OK) {
        return -1;
    }
    
    // Initialize system clocks
    if (bsp_init_system_clock() != 0) {
        return -2;
    }
    
    // Initialize debug UART
    if (bsp_init_debug_uart() != 0) {
        return -3;
    }
    
    return 0;
}

const bsp_config_t* bsp_get_config(void) {
    return &stm32l432kc_config;
}

int bsp_init_system_clock(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    // Configure the main internal regulator output voltage
    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
        return -1;
    }

    // Initialize the RCC Oscillators according to the specified parameters
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 1;
    RCC_OscInitStruct.PLL.PLLN = 10;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
    RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
    RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        return -2;
    }

    // Initialize the CPU, AHB and APB buses clocks
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
        return -3;
    }

    return 0;
}

int bsp_init_debug_uart(void) {
    // Enable USART2 clock for debug output
    __HAL_RCC_USART2_CLK_ENABLE();
    
    // Configure GPIO pins for USART2
    const bsp_pin_config_t* tx_pin = bsp_get_pin_config("DEBUG_TX");
    const bsp_pin_config_t* rx_pin = bsp_get_pin_config("DEBUG_RX");
    
    if (tx_pin && rx_pin) {
        bsp_configure_pin(tx_pin);
        bsp_configure_pin(rx_pin);
    }

    // 8N1, oversampling by 16, clocked from PCLK1 (reset default of USART2SEL)
    DEBUG_UART->CR1 = 0;
    DEBUG_UART->BRR = (HAL_RCC_GetPCLK1Freq() + DEBUG_UART_BAUDRATE / 2U) / DEBUG_UART_BAUDRATE;
    DEBUG_UART->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;

    return 0;
}

void bsp_debug_write(const char* data, size_t len) {
    if (!data || !(DEBUG_UART->CR1 & USART_CR1_UE)) {
        return;
    }

    for (size_t i = 0; i < len; i++) {
        uint32_t timeout = DEBUG_UART_TIMEOUT;
        while (!(DEBUG_UART->ISR & USART_ISR_TXE) && --timeout) {
        }
        DEBUG_UART->TDR = (uint8_t) data[i];
    }
}

void bsp_debug_flush(void) {
    if (!(DEBUG_UART->CR1 & USART_CR1_UE)) {
        return;
    }

    uint32_t timeout = DEBUG_UART_TIMEOUT;
    while (!(DEBUG_UART->ISR & USART_ISR_TC) && --timeout) {
    }
}

const char* bsp_get_reset_cause(void) {
    static const char* cause = nullptr;

    if (!cause) {
        const uint32_t csr = RCC->CSR;

        // a watchdog or software reset also sets PINRSTF, so check those first
        if (csr & RCC_CSR_LPWRRSTF) {
            cause = "low-power";
        } else if (csr & RCC_CSR_IWDGRSTF) {
            cause = "independent watchdog";
        } else if (csr & RCC_CSR_WWDGRSTF) {
            cause = "window watchdog";
        } else if (csr & RCC_CSR_SFTRSTF) {
            cause = "software";
        } else if (csr & RCC_CSR_BORRSTF) {
            cause = "power-on/brown-out";
        } else if (csr & RCC_CSR_FWRSTF) {
            cause = "firewall";
        } else if (csr & RCC_CSR_OBLRSTF) {
            cause = "option byte loader";
        } else if (csr & RCC_CSR_PINRSTF) {
            cause = "reset pin";
        } else {
            cause = "unknown";
        }

        RCC->CSR |= RCC_CSR_RMVF;
    }

    return cause;
}

#ifdef USE_FULL_ASSERT
// STM32 HAL assert_param(): a wrong parameter to the HAL is a programming error
extern "C" void assert_failed(uint8_t* file, uint32_t line) {
    emblib_assert_fail(true, reinterpret_cast<const char*>(file), line, nullptr, nullptr);
}
#endif

const bsp_pin_config_t* bsp_get_pin_config(const char* pin_name) {
    for (uint32_t i = 0; i < stm32l432kc_config.pin_count; i++) {
        if (strcmp(stm32l432kc_config.pins[i].name, pin_name) == 0) {
            return &stm32l432kc_config.pins[i];
        }
    }
    return NULL;
}

const bsp_pin_config_t* bsp_get_pin_config_by_function(bsp_pin_function_t function, uint8_t instance) {
    uint8_t found_count = 0;
    
    for (uint32_t i = 0; i < stm32l432kc_config.pin_count; i++) {
        if (stm32l432kc_config.pins[i].function == function) {
            if (found_count == instance) {
                return &stm32l432kc_config.pins[i];
            }
            found_count++;
        }
    }
    return NULL;
}

int bsp_configure_pin(const bsp_pin_config_t* pin_config) {
    if (!pin_config) {
        return -1;
    }
    
    // Get GPIO port handle
    hal_gpio_port_t* port = hal_gpio_get_port(pin_config->port_name);
    if (!port) {
        return -2;
    }
    
    // Enable GPIO port clock
    hal_gpio_enable_port_clock(port);
    
    // Configure pin based on function
    hal_gpio_init_t gpio_config = {0};
    gpio_config.pin = pin_config->pin;
    gpio_config.pull = HAL_GPIO_NOPULL;
    gpio_config.speed = HAL_GPIO_SPEED_LOW;
    gpio_config.alternate = pin_config->alternate_function;
    
    switch (pin_config->function) {
        case BSP_PIN_FUNCTION_LED:
            gpio_config.mode = HAL_GPIO_MODE_OUTPUT_PP;
            break;
            
        case BSP_PIN_FUNCTION_DEBUG_TX:
        case BSP_PIN_FUNCTION_UART_TX:
        case BSP_PIN_FUNCTION_SPI_SCK:
        case BSP_PIN_FUNCTION_SPI_MOSI:
        case BSP_PIN_FUNCTION_I2C_SCL:
        case BSP_PIN_FUNCTION_I2C_SDA:
            gpio_config.mode = HAL_GPIO_MODE_AF_PP;
            gpio_config.speed = HAL_GPIO_SPEED_HIGH;
            break;
            
        case BSP_PIN_FUNCTION_DEBUG_RX:
        case BSP_PIN_FUNCTION_UART_RX:
        case BSP_PIN_FUNCTION_SPI_MISO:
            gpio_config.mode = HAL_GPIO_MODE_AF_PP;
            gpio_config.pull = HAL_GPIO_PULLUP;
            break;
            
        case BSP_PIN_FUNCTION_BUTTON:
            gpio_config.mode = HAL_GPIO_MODE_INPUT;
            gpio_config.pull = pin_config->active_high ? HAL_GPIO_PULLDOWN : HAL_GPIO_PULLUP;
            break;
            
        default:
            gpio_config.mode = HAL_GPIO_MODE_INPUT;
            break;
    }
    
    return hal_gpio_init(port, &gpio_config);
}

bsp_mcu_family_t bsp_get_mcu_family(void) {
    return stm32l432kc_config.mcu_family;
}

const char* bsp_get_mcu_name(void) {
    return stm32l432kc_config.mcu_name;
}

const char* bsp_get_board_name(void) {
    return stm32l432kc_config.board_name;
}

void bsp_delay_ms(uint32_t ms) {
    HAL_Delay(ms);
}

uint32_t bsp_get_tick(void) {
    return HAL_GetTick();
}
