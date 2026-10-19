/**
 * @file crash_diag_freertos.c
 * @brief FreeRTOS hooks reported through crash_diag
 */

#include "crash_diag.h"
#include "string_builder.h"
#include "FreeRTOS.h"
#include "task.h"

#if (configCHECK_FOR_STACK_OVERFLOW > 0)
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void) xTask;

    char msg[EMBLIB_CRASH_RECORD_MSG_LEN];
    string_builder_t sb;
    sb_init(&sb, msg, sizeof(msg));
    sb_append_str(&sb, "task ");
    sb_append_str(&sb, pcTaskName);

    crash_diag_panic(EMBLIB_CRASH_STACK_OVERFLOW, msg);
}
#endif

#if (configUSE_MALLOC_FAILED_HOOK > 0)
void vApplicationMallocFailedHook(void) {
    crash_diag_panic(EMBLIB_CRASH_USER, "malloc failed");
}
#endif
