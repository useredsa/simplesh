#ifndef SIMPLESH_STRUCTS_H
#define SIMPLESH_STRUCTS_H

#include <sys/types.h>

/// Número máximo de argumentos de un comando
#define MAX_ARGS 16

/// Delimitadores
static const char WHITESPACE[] = " \t\r\n\v";
/// Caracteres especiales
static const char SYMBOLS[] = "<|>&;()";

/******************************************************************************
 * Estructuras de datos `cmd`
 ******************************************************************************/

// Las estructuras `cmd` se utilizan para almacenar información que servirá a
// simplesh para ejecutar líneas de órdenes con redirecciones, tuberías, listas
// de comandos y tareas en segundo plano. El formato es el siguiente:

//     |----------+--------------+--------------|
//     | (1 byte) | ...          | ...          |
//     |----------+--------------+--------------|
//     | type     | otros campos | otros campos |
//     |----------+--------------+--------------|

// Nótese cómo las estructuras `cmd` comparten el primer campo `type` para
// identificar su tipo. A partir de él se obtiene un tipo derivado a través de
// *casting* forzado de tipo. Se consigue así polimorfismo básico en C.

/// Valores del campo `type` de las estructuras de datos `cmd`
enum cmd_type {
    EXEC = 1,
    REDR = 2,
    PIPE = 3,
    LIST = 4,
    BACK = 5,
    SUBS = 6,
    INV = 7
};

struct cmd {
    enum cmd_type type;
};

/// Comando con sus parámetros
struct execcmd {
    enum cmd_type type;
    char* argv[MAX_ARGS];
    char* eargv[MAX_ARGS];
    int argc;
};

/// Comando con redirección
struct redrcmd {
    enum cmd_type type;
    struct cmd* cmd;
    char* file;
    char* efile;
    int flags;
    mode_t mode;
    int fd;
};

/// Comandos con tubería
struct pipecmd {
    enum cmd_type type;
    struct cmd* left;
    struct cmd* right;
};

/// Lista de órdenes
struct listcmd {
    enum cmd_type type;
    struct cmd* left;
    struct cmd* right;
};

/// Tarea en segundo plano (background) con `&`
struct backcmd {
    enum cmd_type type;
    struct cmd* cmd;
};

/// Subshell
struct subscmd {
    enum cmd_type type;
    struct cmd* cmd;
};

/******************************************************************************
 * Funciones para construir las estructuras de datos `cmd`
 ******************************************************************************/

/// Construye una estructura `cmd` de tipo `EXEC`
struct cmd* execcmd(void);

/// Construye una estructura `cmd` de tipo `REDR`
struct cmd* redrcmd(struct cmd* subcmd, char* file, char* efile, int flags,
                    mode_t mode, int fd);

/// Construye una estructura `cmd` de tipo `PIPE`
struct cmd* pipecmd(struct cmd* left, struct cmd* right);

/// Construye una estructura `cmd` de tipo `LIST`
struct cmd* listcmd(struct cmd* left, struct cmd* right);

/// Construye una estructura `cmd` de tipo `BACK`
struct cmd* backcmd(struct cmd* subcmd);

/// Construye una estructura `cmd` de tipo `SUB`
struct cmd* subscmd(struct cmd* subcmd);

/// Destruye una estructura de datos cmd
void free_cmd(struct cmd* cmd);

#endif
