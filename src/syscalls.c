/**
 * @file syscalls.c
 * @brief System call stubs for embedded systems
 */

#include <sys/stat.h>
#include <errno.h>
#include "bsp.h"

#undef errno
extern int errno;

/**
 * @brief Exit a program without cleaning up files
 */
void _exit(int status) {
    (void)status;
    while (1) {
        // Infinite loop - program never exits in embedded system
    }
}

/**
 * @brief Kill process
 */
int _kill(int pid, int sig) {
    (void)pid;
    (void)sig;
    errno = EINVAL;
    return -1;
}

/**
 * @brief Get process ID
 */
int _getpid(void) {
    return 1; // Always return 1 in embedded system
}

/**
 * @brief Send a signal to a process
 */
int _write(int file, char *ptr, int len) {
    (void)file;
    bsp_debug_write(ptr, (size_t)len);
    return len;
}

/**
 * @brief Read from a file
 */
int _read(int file, char *ptr, int len) {
    (void)file;
    (void)ptr;
    (void)len;
    return 0;
}

/**
 * @brief Set position in a file
 */
int _lseek(int file, int ptr, int dir) {
    (void)file;
    (void)ptr;
    (void)dir;
    return 0;
}

/**
 * @brief Status of an open file
 */
int _fstat(int file, struct stat *st) {
    (void)file;
    st->st_mode = S_IFCHR;
    return 0;
}

/**
 * @brief Query whether output stream is a terminal
 */
int _isatty(int file) {
    (void)file;
    return 1;
}

/**
 * @brief Close a file
 */
int _close(int file) {
    (void)file;
    return -1;
}

/**
 * @brief Increase program data space
 */
extern char _end; // Defined by the linker script
static char *heap_end = 0;

void *_sbrk(int incr) {
    char *prev_heap_end;
    
    if (heap_end == 0) {
        heap_end = &_end;
    }
    
    prev_heap_end = heap_end;
    heap_end += incr;
    
    return (void *)prev_heap_end;
}
