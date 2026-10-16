#pragma once

/**
 * @file crash_demo.h
 * @brief Triggers one kind of crash per boot to exercise crash_diag on the board
 *
 * Each boot runs the next step: division by zero, bus fault, undefined
 * instruction, fatal assert, stack overflow, then a recoverable assert. The
 * step counter lives in .noinit RAM, so it advances across the resets caused
 * by the crashes. After the last step it starts over on the next reset.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create the demo task. Call before vTaskStartScheduler().
 * @return 0 on success
 */
int crash_demo_start(void);

#ifdef __cplusplus
}
#endif
