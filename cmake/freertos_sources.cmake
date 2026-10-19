# FreeRTOS kernel sources (FREERTOS_DIR from the main CMakeLists.txt,
# FREERTOS_PORT from the target file, e.g. GCC/ARM_CM4F)

set(FREERTOS_SOURCES
    ${FREERTOS_DIR}/tasks.c
    ${FREERTOS_DIR}/queue.c
    ${FREERTOS_DIR}/list.c
    ${FREERTOS_DIR}/timers.c
    ${FREERTOS_DIR}/event_groups.c
    ${FREERTOS_DIR}/stream_buffer.c
    ${FREERTOS_DIR}/croutine.c
    ${FREERTOS_DIR}/portable/${FREERTOS_PORT}/port.c
    ${FREERTOS_DIR}/portable/MemMang/heap_4.c
)

set(FREERTOS_INCLUDES
    ${FREERTOS_DIR}/include
    ${FREERTOS_DIR}/portable/${FREERTOS_PORT}
)
