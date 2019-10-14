#include "simplesh_syntactic.h"

#include <libgen.h>
#include <string.h>
#include <assert.h>

#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

// Biblioteca readline
#include <readline/history.h>
#include <readline/readline.h>
#include "simplesh_macros.h"


int get_token(char** start_of_str, char const* end_of_str,
              char** start_of_token, char** end_of_token) {
    char* s;
    int ret;

    // Salta los espacios en blanco
    s = *start_of_str;
    while (s < end_of_str && strchr(WHITESPACE, *s)) s++;

    // `start_of_token` apunta al principio del argumento (si no es NULL)
    if (start_of_token) *start_of_token = s;

    ret = *s;
    switch (*s) {
        case 0:
            break;
        case '|':
        case '(':
        case ')':
        case ';':
        case '&':
        case '<':
            s++;
            break;
        case '>':
            s++;
            if (*s == '>') {
                ret = '+';
                s++;
            }
            break;

        default:

            // El caso por defecto (cuando no hay caracteres especiales) es el
            // de un argumento de un comando. `get_token` devuelve el valor
            // `'a'`, `start_of_token` apunta al argumento (si no es `NULL`),
            // `end_of_token` apunta al final del argumento (si no es `NULL`) y
            // `start_of_str` avanza hasta que salta todos los espacios
            // *después* del argumento. Por ejemplo:
            //
            //     |-----------+---+---+---+---+---+---+---+---+---+-----------|
            //     | (espacio) | a | r | g | u | m | e | n | t | o | (espacio)
            //     |
            //     |-----------+---+---+---+---+---+---+---+---+---+-----------|
            //                   ^                                   ^
            //            start_o|f_token                       end_o|f_token

            ret = 'a';
            while (s < end_of_str && !strchr(WHITESPACE, *s) &&
                   !strchr(SYMBOLS, *s))
                s++;
            break;
    }

    // `end_of_token` apunta al final del argumento (si no es `NULL`)
    if (end_of_token) *end_of_token = s;

    // Salta los espacios en blanco
    while (s < end_of_str && strchr(WHITESPACE, *s)) s++;

    // Actualiza `start_of_str`
    *start_of_str = s;

    return ret;
}

int peek(char** start_of_str, char const* end_of_str, char* delimiter) {
    char* s;

    s = *start_of_str;
    while (s < end_of_str && strchr(WHITESPACE, *s)) s++;
    *start_of_str = s;

    return *s && strchr(delimiter, *s);
}

struct cmd* parse_cmd(char* start_of_str) {
    char* end_of_str;
    struct cmd* cmd;

    DPRINTF(DBG_TRACE, "STR\n");

    end_of_str = start_of_str + strlen(start_of_str);

    cmd = parse_line(&start_of_str, end_of_str);

    // Comprueba que se ha alcanzado el final de la línea de órdenes
    peek(&start_of_str, end_of_str, "");
    if (start_of_str != end_of_str)
        error("%s: error sintáctico: %s\n", __func__);

    DPRINTF(DBG_TRACE, "END\n");

    return cmd;
}

struct cmd* parse_line(char** start_of_str, char* end_of_str) {
    struct cmd* cmd;
    int delimiter;

    cmd = parse_pipe(start_of_str, end_of_str);

    while (peek(start_of_str, end_of_str, "&")) {
        // Consume el delimitador de tarea en segundo plano
        delimiter = get_token(start_of_str, end_of_str, 0, 0);
        assert(delimiter == '&');

        // Construye el `cmd` para la tarea en segundo plano
        cmd = backcmd(cmd);
    }

    if (peek(start_of_str, end_of_str, ";")) {
        if (cmd->type == EXEC && ((struct execcmd*)cmd)->argv[0] == 0)
            error("%s: error sintáctico: no se encontró comando\n", __func__);

        // Consume el delimitador de lista de órdenes
        delimiter = get_token(start_of_str, end_of_str, 0, 0);
        assert(delimiter == ';');

        // Construye el `cmd` para la lista
        cmd = listcmd(cmd, parse_line(start_of_str, end_of_str));
    }

    return cmd;
}

struct cmd* parse_pipe(char** start_of_str, char* end_of_str) {
    struct cmd* cmd;
    int delimiter;

    cmd = parse_exec(start_of_str, end_of_str);

    if (peek(start_of_str, end_of_str, "|")) {
        if (cmd->type == EXEC && ((struct execcmd*)cmd)->argv[0] == 0)
            error("%s: error sintáctico: no se encontró comando\n", __func__);

        // Consume el delimitador de tubería
        delimiter = get_token(start_of_str, end_of_str, 0, 0);
        assert(delimiter == '|');

        // Construye el `cmd` para la tubería
        cmd = pipecmd(cmd, parse_pipe(start_of_str, end_of_str));
    }

    return cmd;
}

struct cmd* parse_exec(char** start_of_str, char* end_of_str) {
    char* start_of_token;
    char* end_of_token;
    int token, argc;
    struct execcmd* cmd;
    struct cmd* ret;

    // ¿Inicio de un bloque?
    if (peek(start_of_str, end_of_str, "("))
        return parse_subs(start_of_str, end_of_str);

    // Si no, lo primero que hay en una línea de órdenes es un comando

    // Construye el `cmd` para el comando
    ret = execcmd();
    cmd = (struct execcmd*)ret;

    // ¿Redirecciones antes del comando?
    ret = parse_redr(ret, start_of_str, end_of_str);

    // Bucle para separar los argumentos de las posibles redirecciones
    argc = 0;
    while (!peek(start_of_str, end_of_str, "|)&;")) {
        if ((token = get_token(start_of_str, end_of_str, &start_of_token,
                               &end_of_token)) == 0)
            break;

        // El siguiente token debe ser un argumento porque el bucle
        // para en los delimitadores
        if (token != 'a')
            error("%s: error sintáctico: se esperaba un argumento\n", __func__);

        // Almacena el siguiente argumento reconocido. El primero es
        // el comando
        cmd->argv[argc] = start_of_token;
        cmd->eargv[argc] = end_of_token;
        cmd->argc = ++argc;
        if (argc >= MAX_ARGS) panic("%s: demasiados argumentos\n", __func__);

        // ¿Redirecciones después del comando?
        ret = parse_redr(ret, start_of_str, end_of_str);
    }

    // El comando no tiene más parámetros
    cmd->argv[argc] = 0;
    cmd->eargv[argc] = 0;

    return ret;
}

struct cmd* parse_subs(char** start_of_str, char* end_of_str) {
    int delimiter;
    struct cmd* cmd;
    struct cmd* scmd;

    // Consume el paréntesis de apertura
    if (!peek(start_of_str, end_of_str, "("))
        error("%s: error sintáctico: se esperaba '('", __func__);
    delimiter = get_token(start_of_str, end_of_str, 0, 0);
    assert(delimiter == '(');

    // Realiza el análisis sintáctico hasta el paréntesis de cierre
    scmd = parse_line(start_of_str, end_of_str);

    // Construye el `cmd` para el bloque de órdenes
    cmd = subscmd(scmd);

    // Consume el paréntesis de cierre
    if (!peek(start_of_str, end_of_str, ")"))
        error("%s: error sintáctico: se esperaba ')'", __func__);
    delimiter = get_token(start_of_str, end_of_str, 0, 0);
    assert(delimiter == ')');

    // ¿Redirecciones después del bloque de órdenes?
    cmd = parse_redr(cmd, start_of_str, end_of_str);

    return cmd;
}

struct cmd* parse_redr(struct cmd* cmd, char** start_of_str, char* end_of_str) {
    int delimiter;
    char* start_of_token;
    char* end_of_token;

    // Si lo siguiente que hay a continuación es delimitador de
    // redirección...
    while (peek(start_of_str, end_of_str, "<>")) {
        // Consume el delimitador de redirección
        delimiter = get_token(start_of_str, end_of_str, 0, 0);
        assert(delimiter == '<' || delimiter == '>' || delimiter == '+');

        // El siguiente token tiene que ser el nombre del fichero de la
        // redirección entre `start_of_token` y `end_of_token`.
        if ('a' !=
            get_token(start_of_str, end_of_str, &start_of_token, &end_of_token))
            error("%s: error sintáctico: se esperaba un fichero", __func__);

        // Construye el `cmd` para la redirección
        switch (delimiter) {
            case '<':
                cmd = redrcmd(cmd, start_of_token, end_of_token, O_RDONLY,
                              S_IRWXU, STDIN_FILENO);
                break;
            case '>':
                cmd =
                    redrcmd(cmd, start_of_token, end_of_token,
                            O_RDWR | O_TRUNC | O_CREAT, S_IRWXU, STDOUT_FILENO);
                break;
            case '+':  // >>
                cmd = redrcmd(cmd, start_of_token, end_of_token,
                              O_RDWR | O_APPEND | O_CREAT, S_IRWXU,
                              STDOUT_FILENO);
                break;
        }
    }

    return cmd;
}

struct cmd* null_terminate(struct cmd* cmd) {
    struct execcmd* ecmd;
    struct redrcmd* rcmd;
    struct pipecmd* pcmd;
    struct listcmd* lcmd;
    struct backcmd* bcmd;
    struct subscmd* scmd;
    int i;

    if (cmd == 0) return 0;

    switch (cmd->type) {
        case EXEC:
            ecmd = (struct execcmd*)cmd;
            for (i = 0; ecmd->argv[i]; i++) *ecmd->eargv[i] = 0;
            break;

        case REDR:
            rcmd = (struct redrcmd*)cmd;
            null_terminate(rcmd->cmd);
            *rcmd->efile = 0;
            break;

        case PIPE:
            pcmd = (struct pipecmd*)cmd;
            null_terminate(pcmd->left);
            null_terminate(pcmd->right);
            break;

        case LIST:
            lcmd = (struct listcmd*)cmd;
            null_terminate(lcmd->left);
            null_terminate(lcmd->right);
            break;

        case BACK:
            bcmd = (struct backcmd*)cmd;
            null_terminate(bcmd->cmd);
            break;

        case SUBS:
            scmd = (struct subscmd*)cmd;
            null_terminate(scmd->cmd);
            break;

        case INV:
        default:
            panic("%s: estructura `cmd` desconocida\n", __func__);
    }

    return cmd;
}

char* get_cmd() {
    char* buf;
    const int PROMPT_STRING_SIZE = 300;
    char prompt[PROMPT_STRING_SIZE];

    uid_t uid = getuid();  // Obtenemos el uid del usuario
    struct passwd* entry =
        getpwuid(uid);  // Obtenemos la entrada del fichero /etc/passwd
    // TODO comprobar errores de getpwuid y si hay que liberar memoria
    int used_size = sprintf(prompt, "%s@", entry->pw_name);
    getcwd(prompt + used_size, PROMPT_STRING_SIZE - used_size);
    char* wd = basename(prompt + used_size);
    // TODO comprobar errores de getcwd y si hay que liberar memoria (no parece)
    sprintf(prompt + used_size, "%s> ", wd);

    // Lee la orden tecleada por el usuario
    buf = readline(prompt);

    // Si el usuario ha escrito una orden, almacenarla en la historia.
    if (buf) add_history(buf);

    return buf;
}
