/**
 * @file crash_diag.c
 * @brief Post-mortem crash diagnostics for Cortex-M3/M4/M7
 */

#include "crash_diag.h"
#include "emblib_assert.h"
#include "string_builder.h"
#include "bsp.h"

#include <stdlib.h>
#include <string.h>

#ifndef CRASH_DIAG_DEVICE_HEADER
#define CRASH_DIAG_DEVICE_HEADER "stm32l4xx.h"
#endif
#include CRASH_DIAG_DEVICE_HEADER

#ifndef CRASH_DIAG_BREAK_ON_FAULT
#if defined(DEBUG)
#define CRASH_DIAG_BREAK_ON_FAULT 1
#else
#define CRASH_DIAG_BREAK_ON_FAULT 0
#endif
#endif

// fits the one-line report of emblib_crash_record_format()
#define CRASH_DIAG_REPORT_LEN 192

// EXC_RETURN bit 4 is 0 when the core stacked the extended (FPU) frame
#define EXC_RETURN_STD_FRAME (1u << 4)
// stacked xPSR bit 9 is 1 when the core added 4 bytes to align the stack
#define STACKED_PSR_ALIGNED (1u << 9)

#define BASIC_FRAME_SIZE (8u * 4u)
#define FPU_FRAME_SIZE (26u * 4u)

// registers stacked by the core on exception entry
typedef struct {
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t psr;
} exception_frame_t;

typedef struct {
    uint32_t mask;
    const char *name;
} fault_bit_t;

static const fault_bit_t cfsr_bits[] = {
    {SCB_CFSR_IACCVIOL_Msk, "IACCVIOL"},
    {SCB_CFSR_DACCVIOL_Msk, "DACCVIOL"},
    {SCB_CFSR_MUNSTKERR_Msk, "MUNSTKERR"},
    {SCB_CFSR_MSTKERR_Msk, "MSTKERR"},
    {SCB_CFSR_MLSPERR_Msk, "MLSPERR"},
    {SCB_CFSR_MMARVALID_Msk, "MMARVALID"},
    {SCB_CFSR_IBUSERR_Msk, "IBUSERR"},
    {SCB_CFSR_PRECISERR_Msk, "PRECISERR"},
    {SCB_CFSR_IMPRECISERR_Msk, "IMPRECISERR"},
    {SCB_CFSR_UNSTKERR_Msk, "UNSTKERR"},
    {SCB_CFSR_STKERR_Msk, "STKERR"},
    {SCB_CFSR_LSPERR_Msk, "LSPERR"},
    {SCB_CFSR_BFARVALID_Msk, "BFARVALID"},
    {SCB_CFSR_UNDEFINSTR_Msk, "UNDEFINSTR"},
    {SCB_CFSR_INVSTATE_Msk, "INVSTATE"},
    {SCB_CFSR_INVPC_Msk, "INVPC"},
    {SCB_CFSR_NOCP_Msk, "NOCP"},
    {SCB_CFSR_UNALIGNED_Msk, "UNALIGNED"},
    {SCB_CFSR_DIVBYZERO_Msk, "DIVBYZERO"},
};

static const fault_bit_t hfsr_bits[] = {
    {SCB_HFSR_VECTTBL_Msk, "VECTTBL"},
    {SCB_HFSR_FORCED_Msk, "FORCED"},
    {SCB_HFSR_DEBUGEVT_Msk, "DEBUGEVT"},
};

// not initialized by the startup code, so it survives the reset
__attribute__((section(".noinit"))) static emblib_crash_record_t crash_record;

// linker script symbols delimiting RAM
extern uint32_t _sdata;
extern uint32_t _estack;

static void diag_print(const char *s) {
    bsp_debug_write(s, strlen(s));
}

__attribute__((noreturn)) static void crash_diag_stop(void) {
    bsp_debug_flush();

#if CRASH_DIAG_BREAK_ON_FAULT
    if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) {
        __BKPT(0);
    }
#endif

    NVIC_SystemReset();
}

static void crash_diag_save_and_print(const emblib_crash_record_t *data) {
    char report[CRASH_DIAG_REPORT_LEN];
    string_builder_t sb;

    const bool saved = emblib_crash_record_save(&crash_record, data);

    // local sealed copy, so this crash is printed even when an older record was kept
    emblib_crash_record_t shown;
    memset(&shown, 0, sizeof(shown));
    emblib_crash_record_save(&shown, data);

    sb_init(&sb, report, sizeof(report));
    emblib_crash_record_format(&shown, &sb);

    diag_print("\r\n[CRASH] ");
    diag_print(report);
    diag_print(saved ? "\r\n" : "\r\n(not saved: a previous crash record is pending)\r\n");
}

static void append_fault_bits(string_builder_t *sb, uint32_t reg, const fault_bit_t *bits, size_t count) {
    for (size_t i = 0; i < count; i++) {
        if (reg & bits[i].mask) {
            if (sb_get_len(sb)) {
                sb_append_char(sb, ' ');
            }
            sb_append_str(sb, bits[i].name);
        }
    }
}

static bool frame_in_ram(const exception_frame_t *frame) {
    const uint32_t addr = (uint32_t) frame;
    return addr >= (uint32_t) &_sdata && addr + sizeof(*frame) <= (uint32_t) &_estack;
}

/**
 * Called by HardFault_Handler with the stack the core used when the fault
 * happened (MSP or PSP) and the EXC_RETURN value.
 */
__attribute__((used, noreturn)) void crash_diag_fault_handler_c(const exception_frame_t *frame, uint32_t exc_return) {
    const uint32_t cfsr = SCB->CFSR;
    const uint32_t hfsr = SCB->HFSR;

    emblib_crash_record_t data;
    memset(&data, 0, sizeof(data));
    data.reason = EMBLIB_CRASH_FAULT;
    data.fault_status = cfsr;
    data.fault_status2 = hfsr;

    if (cfsr & SCB_CFSR_MMARVALID_Msk) {
        data.fault_address = SCB->MMFAR;
    } else if (cfsr & SCB_CFSR_BFARVALID_Msk) {
        data.fault_address = SCB->BFAR;
    }

    // a stacking error means the frame was not written: do not trust it
    const bool stacking_failed = cfsr & (SCB_CFSR_MSTKERR_Msk | SCB_CFSR_STKERR_Msk);
    if (!stacking_failed && frame_in_ram(frame)) {
        data.pc = frame->pc;
        data.lr = frame->lr;
        data.psr = frame->psr;

        // stack pointer of the faulting code, before the core stacked the frame
        uint32_t frame_size = (exc_return & EXC_RETURN_STD_FRAME) ? BASIC_FRAME_SIZE : FPU_FRAME_SIZE;
        if (frame->psr & STACKED_PSR_ALIGNED) {
            frame_size += 4u;
        }
        data.sp = (uint32_t) frame + frame_size;
    } else {
        data.sp = (uint32_t) frame;
    }

    string_builder_t sb;
    sb_init(&sb, data.msg, sizeof(data.msg));
    append_fault_bits(&sb, cfsr, cfsr_bits, sizeof(cfsr_bits) / sizeof(cfsr_bits[0]));
    append_fault_bits(&sb, hfsr, hfsr_bits, sizeof(hfsr_bits) / sizeof(hfsr_bits[0]));

    crash_diag_save_and_print(&data);
    crash_diag_stop();
}

/**
 * Entry point of the fault. Naked, so no prologue touches the stack before we
 * read it: EXC_RETURN bit 2 tells whether the core stacked on MSP or PSP.
 */
__attribute__((naked)) void HardFault_Handler(void) {
    __asm volatile(
        "tst lr, #4                     \n"
        "ite eq                         \n"
        "mrseq r0, msp                  \n"
        "mrsne r0, psp                  \n"
        "mov r1, lr                     \n"
        "b crash_diag_fault_handler_c   \n");
}

// configurable faults escalate to HardFault while disabled; route them here if enabled
void MemManage_Handler(void) __attribute__((alias("HardFault_Handler")));
void BusFault_Handler(void) __attribute__((alias("HardFault_Handler")));
void UsageFault_Handler(void) __attribute__((alias("HardFault_Handler")));

void crash_diag_panic(emblib_crash_reason_t reason, const char *msg) {
    __disable_irq();

    emblib_crash_record_t data;
    memset(&data, 0, sizeof(data));
    data.reason = reason;
    data.lr = (uint32_t) __builtin_return_address(0);
    data.psr = __get_xPSR();

    // the stack in use (PSP inside a task, MSP in an ISR or before the scheduler)
    uint32_t sp;
    __asm volatile("mov %0, sp" : "=r"(sp));
    data.sp = sp;

    if (msg) {
        string_builder_t sb;
        sb_init(&sb, data.msg, sizeof(data.msg));
        sb_append_str(&sb, msg);
    }

    crash_diag_save_and_print(&data);
    crash_diag_stop();
}

static void crash_diag_assert_handler(const emblib_assert_info_t *info) {
    char text[EMBLIB_CRASH_RECORD_MSG_LEN];
    string_builder_t sb;
    sb_init(&sb, text, sizeof(text));
    emblib_assert_format(info, &sb);

    if (info->fatal) {
        crash_diag_panic(EMBLIB_CRASH_ASSERT, text);
    }

    diag_print("[ASSERT] ");
    diag_print(text);
    diag_print("\r\n");
}

void crash_diag_init(void) {
    // without this, an integer division by zero silently returns 0
    SCB->CCR |= SCB_CCR_DIV_0_TRP_Msk;

    emblib_assert_set_handler(crash_diag_assert_handler);
}

bool crash_diag_report_previous(void) {
    diag_print("\r\n[BOOT] reset cause: ");
    diag_print(bsp_get_reset_cause());
    diag_print("\r\n");

    if (!emblib_crash_record_is_valid(&crash_record)) {
        return false;
    }

    char report[CRASH_DIAG_REPORT_LEN];
    string_builder_t sb;
    sb_init(&sb, report, sizeof(report));
    emblib_crash_record_format(&crash_record, &sb);

    diag_print("[BOOT] previous crash: ");
    diag_print(report);
    diag_print("\r\n");

    emblib_crash_record_clear(&crash_record);
    return true;
}

// newlib assert()
__attribute__((noreturn)) void __assert_func(const char *file, int line, const char *function, const char *condition) {
    emblib_assert_fail(true, file, (uint32_t) line, function, condition);
    crash_diag_panic(EMBLIB_CRASH_ASSERT, "assert");
}

// replaces newlib abort(): also reached by emblib fatal asserts with no handler
__attribute__((noreturn)) void abort(void) {
    crash_diag_panic(EMBLIB_CRASH_ASSERT, "abort");
}
