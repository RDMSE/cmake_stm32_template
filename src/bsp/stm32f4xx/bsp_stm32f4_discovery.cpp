/**
 * @file bsp_stm32f4_discovery.cpp
 * @brief Board Support Package for the STM32F4 Discovery boards
 *
 * This file implements the board specific part of the BSP interface for the
 * two Discovery boards that share the same PCB layout (LEDs on PD12-PD15,
 * user button on PA0, 8 MHz crystal):
 *   - STM32F4-Discovery (STM32F407G-DISC1), STM32F407VG, 168 MHz
 *   - STM32F401C-DISCO, STM32F401VC, 84 MHz
 * The chip is selected by the device define of the target (STM32F407xx or
 * STM32F401xC). The functions shared by all boards are in common/bsp_common.cpp.
 */

#include "../common/bsp.h"

extern "C" {
#include "stm32f4xx_hal.h"
#include "emblib_assert.h"
}

#if defined(STM32F407xx)
#define BOARD_NAME          "STM32F4-Discovery"
#define MCU_NAME            "STM32F407VG"
#define FLASH_SIZE_KB       1024U
#define RAM_SIZE_KB         128U        // plus 64 KB CCM RAM, not used
#define SYSCLK_HZ           168000000U
#define PLL_P               RCC_PLLP_DIV2
#define APB1_DIVIDER        RCC_HCLK_DIV4   // 42 MHz, the APB1 maximum
#define APB2_DIVIDER        RCC_HCLK_DIV2   // 84 MHz, the APB2 maximum
#define VOLTAGE_SCALE       PWR_REGULATOR_VOLTAGE_SCALE1
#define FLASH_WAIT_STATES   FLASH_LATENCY_5 // 168 MHz at 2.7-3.6 V
#elif defined(STM32F401xC)
#define BOARD_NAME          "STM32F401C-DISCO"
#define MCU_NAME            "STM32F401VC"
#define FLASH_SIZE_KB       256U
#define RAM_SIZE_KB         64U
#define SYSCLK_HZ           84000000U
#define PLL_P               RCC_PLLP_DIV4
#define APB1_DIVIDER        RCC_HCLK_DIV2   // 42 MHz, the APB1 maximum
#define APB2_DIVIDER        RCC_HCLK_DIV1   // 84 MHz
#define VOLTAGE_SCALE       PWR_REGULATOR_VOLTAGE_SCALE2  // enough up to 84 MHz
#define FLASH_WAIT_STATES   FLASH_LATENCY_2 // 84 MHz at 2.7-3.6 V
#else
#error "Unsupported STM32F4 Discovery chip: define STM32F407xx or STM32F401xC"
#endif

// The on-board ST-LINK has no virtual COM port wired to the STM32F4: the
// debug output goes out on USART2 PA2 (TX) / PA3 (RX), to be connected to a
// 3.3 V USB-serial adapter
#define DEBUG_UART              USART2
#define DEBUG_UART_BAUDRATE     115200U
// bounded busy-wait: a byte at 115200 baud takes ~87 us, this is well above it
#define DEBUG_UART_TIMEOUT      100000U

// =============================================================================
// Pin Configuration for STM32F4-Discovery
// =============================================================================

static const bsp_pin_config_t discovery_pins[] = {
    // User LEDs: the first one is the default LED of the portable examples
    {
        .name = "LED_USER",         // LD4, green
        .port_name = "GPIOD",
        .pin = 12,
        .function = BSP_PIN_FUNCTION_LED,
        .alternate_function = 0,
        .active_high = true
    },
    {
        .name = "LED_ORANGE",       // LD3
        .port_name = "GPIOD",
        .pin = 13,
        .function = BSP_PIN_FUNCTION_LED,
        .alternate_function = 0,
        .active_high = true
    },
    {
        .name = "LED_RED",          // LD5
        .port_name = "GPIOD",
        .pin = 14,
        .function = BSP_PIN_FUNCTION_LED,
        .alternate_function = 0,
        .active_high = true
    },
    {
        .name = "LED_BLUE",         // LD6
        .port_name = "GPIOD",
        .pin = 15,
        .function = BSP_PIN_FUNCTION_LED,
        .alternate_function = 0,
        .active_high = true
    },

    // User button B1 (external pull-down on the board)
    {
        .name = "BUTTON_USER",
        .port_name = "GPIOA",
        .pin = 0,
        .function = BSP_PIN_FUNCTION_BUTTON,
        .alternate_function = 0,
        .active_high = true
    },

    // Debug UART (USART2, see DEBUG_UART above)
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
        .pin = 3,
        .function = BSP_PIN_FUNCTION_DEBUG_RX,
        .alternate_function = 7,  // AF7: USART2_RX
        .active_high = true
    },

    // SPI1 pins (also wired to the on-board MEMS sensor)
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

    // I2C1 pins (also wired to the CS43L22 audio DAC and, on the F401, the LSM303DLHC)
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
        .pin = 9,
        .function = BSP_PIN_FUNCTION_I2C_SDA,
        .alternate_function = 4,  // AF4: I2C1_SDA
        .active_high = true
    }
};

// =============================================================================
// Clock Configuration
// =============================================================================

static const bsp_clock_config_t discovery_clock_config = {
    .hsi_frequency_hz = 16000000,   // 16 MHz HSI
    .hse_frequency_hz = 8000000,    // 8 MHz crystal (X2)
    .lsi_frequency_hz = 32000,      // ~32 kHz LSI
    .lse_frequency_hz = 0,          // No 32.768 kHz crystal fitted
    .target_sysclk_hz = SYSCLK_HZ,
    .target_hclk_hz = SYSCLK_HZ,
    .target_pclk1_hz = 42000000,    // 42 MHz APB1 clock
    .target_pclk2_hz = 84000000,    // 84 MHz APB2 clock
    .hse_bypass = false,
    .lse_bypass = false
};

// =============================================================================
// BSP Configuration Structure
// =============================================================================

static const bsp_config_t discovery_config = {
    .board_name = BOARD_NAME,
    .mcu_name = MCU_NAME,
    .mcu_family = BSP_MCU_FAMILY_STM32F4XX,
    .flash_size_kb = FLASH_SIZE_KB,
    .ram_size_kb = RAM_SIZE_KB,
    .pin_count = sizeof(discovery_pins) / sizeof(discovery_pins[0]),
    .pins = discovery_pins,
    .clock_config = &discovery_clock_config,
    .mcu_specific_data = NULL
};

// =============================================================================
// BSP Implementation Functions
// =============================================================================

const bsp_config_t* bsp_get_config(void) {
    return &discovery_config;
}

int bsp_init_system_clock(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    // Configure the main internal regulator output voltage
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(VOLTAGE_SCALE);

    // Initialize the RCC Oscillators according to the specified parameters
    // Using HSE 8MHz external crystal
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;   // VCO input = HSE/PLLM = 8MHz/8 = 1MHz
    RCC_OscInitStruct.PLL.PLLN = 336; // VCO output = VCO input * PLLN = 1MHz * 336 = 336MHz
    RCC_OscInitStruct.PLL.PLLP = PLL_P; // PLLCLK = VCO output / PLLP = 336MHz/2 = 168MHz (F407), /4 = 84MHz (F401)
    RCC_OscInitStruct.PLL.PLLQ = 7;   // USB OTG FS clock = VCO output / PLLQ = 336MHz/7 = 48MHz

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        return -1;
    }

    // Initialize the CPU, AHB and APB buses clocks
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;   // AHB = SYSCLK
    RCC_ClkInitStruct.APB1CLKDivider = APB1_DIVIDER;     // APB1 = 42MHz (max 42MHz)
    RCC_ClkInitStruct.APB2CLKDivider = APB2_DIVIDER;     // APB2 = 84MHz (max 84MHz)

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_WAIT_STATES) != HAL_OK) {
        return -2;
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

    // 8N1, oversampling by 16, USART2 is clocked from PCLK1
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
        while (!(DEBUG_UART->SR & USART_SR_TXE) && --timeout) {
        }
        DEBUG_UART->DR = (uint8_t) data[i];
    }
}

void bsp_debug_flush(void) {
    if (!(DEBUG_UART->CR1 & USART_CR1_UE)) {
        return;
    }

    uint32_t timeout = DEBUG_UART_TIMEOUT;
    while (!(DEBUG_UART->SR & USART_SR_TC) && --timeout) {
    }
}

const char* bsp_get_reset_cause(void) {
    static const char* cause = nullptr;

    if (!cause) {
        const uint32_t csr = RCC->CSR;

        // a watchdog, software or power-on reset also sets PINRSTF, so check those first
        if (csr & RCC_CSR_LPWRRSTF) {
            cause = "low-power";
        } else if (csr & RCC_CSR_IWDGRSTF) {
            cause = "independent watchdog";
        } else if (csr & RCC_CSR_WWDGRSTF) {
            cause = "window watchdog";
        } else if (csr & RCC_CSR_SFTRSTF) {
            cause = "software";
        } else if (csr & RCC_CSR_PORRSTF) {
            cause = "power-on";
        } else if (csr & RCC_CSR_BORRSTF) {
            cause = "brown-out";
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
