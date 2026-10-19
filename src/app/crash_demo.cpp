/**
 * @file crash_demo.cpp
 * @brief Triggers one kind of crash per boot to exercise crash_diag on the board
 */

#include "crash_demo.h"

extern "C" {
#include "bsp.h"
#include "emblib_assert.h"
#include "FreeRTOS.h"
#include "task.h"
}

#include <cstdint>
#include <cstring>

#define CRASH_DEMO_MAGIC        0x44454D4FU  // "DEMO"
#define CRASH_DEMO_DELAY_MS     2000U
#define CRASH_DEMO_STACK_WORDS  256U
#define OVERFLOW_STACK_WORDS    128U

// first address after the end of the flash: reserved, the bus answers with an error
#if defined(STM32F407xx)
#define UNMAPPED_ADDRESS        0x08100000U  // 1 MB of flash
#else
// 256 KB of flash on the STM32L432 and STM32F401VC.
// 0x60000000 (FMC bank) does not work on the L432: it reads as 0
#define UNMAPPED_ADDRESS        0x08040000U
#endif

struct crash_demo_state {
    uint32_t magic;
    uint32_t step;
};

// survives the resets caused by the crashes
__attribute__((section(".noinit"))) static crash_demo_state demo_state;

static void demo_print(const char* s) {
    bsp_debug_write(s, strlen(s));
}

// volatile keeps the compiler from folding or removing the faulting code
static void crash_div_by_zero(void) {
    volatile int a = 1;
    volatile int b = 0;
    volatile int c = a / b;
    (void)c;
}

static void crash_bus_fault(void) {
    volatile uint32_t value = *reinterpret_cast<volatile uint32_t*>(UNMAPPED_ADDRESS);
    (void)value;
}

static void crash_undefined_instruction(void) {
    __asm volatile("udf #0");
}

static void crash_fatal_assert(void) {
    volatile bool sensor_ready = false;
    EMBLIB_ASSERT_FATAL(sensor_ready);
}

static void overflow_task(void* param) {
    (void)param;

    // larger than the whole task stack: runs past its end and over the
    // watermark that configCHECK_FOR_STACK_OVERFLOW == 2 checks on a context switch
    volatile uint8_t buffer[OVERFLOW_STACK_WORDS * sizeof(StackType_t) + 32U];
    for (size_t i = 0; i < sizeof(buffer); i++) {
        buffer[i] = static_cast<uint8_t>(i);
    }

    vTaskDelay(1);

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void crash_stack_overflow(void) {
    xTaskCreate(overflow_task, "overflow", OVERFLOW_STACK_WORDS, nullptr, tskIDLE_PRIORITY + 2, nullptr);
    vTaskDelay(pdMS_TO_TICKS(100));
}

static void recoverable_assert(void) {
    volatile int config_version = 0;
    if (EMBLIB_ASSERT(config_version > 0)) {
        demo_print("[DEMO] invalid config, using defaults and carrying on\r\n");
    }
}

struct crash_demo_step {
    const char* description;
    void (*run)(void);
};

static const crash_demo_step demo_steps[] = {
    {"integer division by zero", crash_div_by_zero},
    {"read from an unmapped address", crash_bus_fault},
    {"undefined instruction", crash_undefined_instruction},
    {"fatal assert", crash_fatal_assert},
    {"task stack overflow", crash_stack_overflow},
    {"recoverable assert", recoverable_assert},
};

static constexpr uint32_t demo_step_count = sizeof(demo_steps) / sizeof(demo_steps[0]);

static void crash_demo_task(void* param) {
    (void)param;

    if (demo_state.magic != CRASH_DEMO_MAGIC || demo_state.step >= demo_step_count) {
        demo_state.magic = CRASH_DEMO_MAGIC;
        demo_state.step = 0;
    }

    const uint32_t step = demo_state.step;
    const crash_demo_step& current = demo_steps[step];

    // advance before crashing, so the next boot runs the next step
    demo_state.step = step + 1;

    char line[64] = "[DEMO] step ";
    const size_t len = strlen(line);
    line[len] = static_cast<char>('1' + step);
    line[len + 1] = '\0';
    demo_print(line);
    demo_print(": ");
    demo_print(current.description);
    demo_print(" in 2 s\r\n");

    vTaskDelay(pdMS_TO_TICKS(CRASH_DEMO_DELAY_MS));
    current.run();

    // only the recoverable assert is expected to get here
    if (current.run != recoverable_assert) {
        demo_print("[DEMO] unexpected: this step did not crash\r\n");
    }

    if (demo_state.step >= demo_step_count) {
        demo_print("[DEMO] done, press reset to run it again\r\n");
    }

    vTaskDelete(nullptr);
}

int crash_demo_start(void) {
    const BaseType_t result = xTaskCreate(crash_demo_task, "crash_demo", CRASH_DEMO_STACK_WORDS,
                                          nullptr, tskIDLE_PRIORITY + 1, nullptr);
    return result == pdPASS ? 0 : -1;
}
