#ifndef SIMPLESH_MACROS_H
#define SIMPLESH_MACROS_H

#define _POSIX_C_SOURCE \
    200809L /* IEEE 1003.1-2008 (véase /usr/include/features.h) */
//#define NDEBUG                /* Traduce asertos y DMACROS a 'no ops' */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


/******************************************************************************
 * Constantes, macros y variables globales
 ******************************************************************************/

static const char* VERSION = "0.19";

// Niveles de depuración
#define DBG_CMD (1 << 0)
#define DBG_TRACE (1 << 1)
// . . .
static int g_dbg_level = 0;

#ifndef NDEBUG
#define DPRINTF(dbg_level, fmt, ...)                                          \
    do {                                                                      \
        if (dbg_level & g_dbg_level)                                          \
            fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, \
                    ##__VA_ARGS__);                                           \
    } while (0)

#define DBLOCK(dbg_level, block)            \
    do {                                    \
        if (dbg_level & g_dbg_level) block; \
    } while (0);
#else
#define DPRINTF(dbg_level, fmt, ...)
#define DBLOCK(dbg_level, block)
#endif

#define TRY(x)                                                                \
    do {                                                                      \
        int __rc = (x);                                                       \
        if (__rc < 0) {                                                       \
            fprintf(stderr, "%s:%d:%s: TRY(%s) failed\n", __FILE__, __LINE__, \
                    __func__, #x);                                            \
            fprintf(stderr, "ERROR: rc=%d errno=%d (%s)\n", __rc, errno,      \
                    strerror(errno));                                         \
            exit(EXIT_FAILURE);                                               \
        }                                                                     \
    } while (0)

/**
 * @brief Check whether the function failed because the children
 * wait was performed by the signal manager.
 */
#define TRY_AND_ACCEPT_ECHILD(x)        \
    do {                                \
        int __rt = (x);                 \
        if (__rt < 0 && errno != ECHILD) TRY(__rt); \
    } while (0)

/* Macros de Impresión */

#define INFO(fmt, ...)                                        \
    do {                                                      \
        fprintf(stdout, "%s: " fmt, __func__, ##__VA_ARGS__); \
    } while (0)

#define ERROR(fmt, ...)                                       \
    do {                                                      \
        fprintf(stderr, "%s: " fmt, __func__, ##__VA_ARGS__); \
    } while (0)

/**
 * @brief Imprime el mensaje
 *
 * @param fmt
 * @param ...
 */
void info(const char* fmt, ...);

/**
 * @brief Imprime el mensaje de error
 *
 * @param fmt
 * @param ...
 */
void error(const char* fmt, ...);

/**
 * @brief Imprime el mensaje de error y aborta la ejecución
 *
 * @param fmt
 * @param ...
 */
void panic(const char* fmt, ...);

/**
 * @brief `fork()` que muestra un mensaje de error si no se puede crear el hijo
 */
int fork_or_panic(const char* s);

#endif
