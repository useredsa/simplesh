#include "cmd_exec.h"
#include "macros.h"

#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <fcntl.h>
#include <dirent.h>

#include "psplit.h"
#include "sign_mgmt.h"

/******************************************************************************
 * Funciones para la ejecución de comandos internos
 ******************************************************************************/

int EXIT = 0;

int run_exit(char** argv, int argc) {
    //TODO check arguments?
    EXIT = 1;
    return 0;
}

int run_cwd(char** argv, int argc) {
    //TODO check arguments?
    static const int BUFFER_SIZE = 300;
    char buf[BUFFER_SIZE];
    char* token = getcwd(buf, BUFFER_SIZE);
    if (token == NULL) {
        error("cwd: couldn't get current working directory: errno %d (%s)", errno, strerror(errno));
        return errno; //TODO is correct?
    }
    printf("cwd: %s\n", token);
    return 0;
}

int run_cd(char** argv, int argc) { //TODO setenv
    static const int BUFFER_SIZE = 300;
    static char OLDPWD[300] = "", buff[300]; //TODO no deja poner BUFFER_SIZE ahí
    if (argc > 2) {
        error("cwd: too many arguments");
        return 1; //TODO copied from bash
    }   
    char *dir;
    if (argc == 1) {
        char* buf;
        const int PROMPT_STRING_SIZE = 300;
        char prompt[PROMPT_STRING_SIZE];

        uid_t uid = getuid();  // Obtenemos el uid del usuario
        // Obtenemos de la entrada del fichero /etc/passwd el directorio home
        // ¡No liberar la estructura devuelta por getpwuid con free!
        struct passwd* entry = getpwuid(uid);
        if (entry == NULL) {
            error("cd: couldn't locate the home directory");
            return errno; //TODO checkear si es correcto (tbn abajo 2 veces)
        }
        dir = entry->pw_dir;
        if (dir == NULL) {
            error("cd: couldn't locate the home directory");
            return errno;
        }
    } else {
        if (strcmp(argv[1], "-") == 0) {
            if (strcmp(OLDPWD, "") == 0) {
                error("cd: no older directory\n"); //TODO buen manejo de los mensajes de error
                return 1;
            }
            strcpy(buff, OLDPWD);
            dir = buff;
        } else {
            dir = argv[1];
        }
    }
    getcwd(OLDPWD, BUFFER_SIZE);
    if (chdir(dir) == -1) {
        error("cd: couldn't change current directory to %s: errno %d (%s)",
              dir, errno, strerror(errno));
        return errno;
    }
    return 0;
}

static char inter_comms[][15] = {"cwd", "exit", "cd", "psplit", "bjobs"};
static int (*inter_funcs[])(char** argv, int argc) = {run_cwd, run_exit, run_cd,
                                                      run_psplit, run_bjobs};
int (*isInter(char *comm))(char **argv, int argc) {
    if (comm == NULL) return NULL;
    for (int i = 0; i < sizeof(inter_comms) / sizeof(inter_comms[0]); i++)
        if (strcmp(inter_comms[i], comm) == 0) return inter_funcs[i];
    return NULL;
}

/******************************************************************************
 * Funciones para la ejecución de la línea de órdenes
 ******************************************************************************/

void exec_cmd(struct execcmd* ecmd) {
    assert(ecmd->type == EXEC);
    // Si no hay comando:
    if (ecmd->argv[0] == NULL) exit(EXIT_SUCCESS);

    execvp(ecmd->argv[0], ecmd->argv);

    panic("no se encontró el comando '%s'\n", ecmd->argv[0]);
}

void run_cmd(struct cmd* cmd) {
    if (EXIT) return;
    struct execcmd* ecmd;
    struct redrcmd* rcmd;
    struct listcmd* lcmd;
    struct pipecmd* pcmd;
    struct backcmd* bcmd;
    struct subscmd* scmd;
    int p[2], pid[2];
    int fd;

    DPRINTF(DBG_TRACE, "STR\n");

    if (cmd == 0) return;

    switch (cmd->type) {
        case EXEC:
            ecmd = (struct execcmd*)cmd;
            int (*func)(char** argv, int argc) = isInter(ecmd->argv[0]);
            if (func != NULL) {
                (*func)(ecmd->argv, ecmd->argc);
            } else {
                if ((pid[0] = fork_or_panic("fork EXEC")) == 0) {
                    exec_cmd(ecmd);
                }
                TRY_AND_ACCEPT_ECHILD(waitpid(pid[0], NULL, 0));
            }
            break;

        case REDR:
            rcmd = (struct redrcmd*)cmd;
            TRY(fd = dup(rcmd->fd));
            TRY(close(rcmd->fd));
            if (open(rcmd->file, rcmd->flags, rcmd->mode) < 0) {
                fprintf(stderr, "simplesh: Couldn't open file '%s' (%s)\n", rcmd->file, strerror(errno));
            } else {
                run_cmd(rcmd->cmd);
                TRY(close(rcmd->fd));
            }
            TRY(dup(fd));
            TRY(close(fd));
            break;

        case LIST:
            lcmd = (struct listcmd*)cmd;
            run_cmd(lcmd->left);
            run_cmd(lcmd->right);
            break;

        case PIPE:
            pcmd = (struct pipecmd*)cmd;
            if (pipe(p) < 0) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }

            // Ejecución del hijo de la izquierda
            if ((pid[0] = fork_or_panic("fork PIPE left")) == 0) {
                TRY(close(STDOUT_FILENO));
                TRY(dup(p[1]));
                TRY(close(p[0]));
                TRY(close(p[1]));
                run_cmd(pcmd->left);
                exit(EXIT_SUCCESS);
            }

            // Ejecución del hijo de la derecha
            if ((pid[1] = fork_or_panic("fork PIPE right")) == 0) {
                TRY(close(STDIN_FILENO));
                TRY(dup(p[0]));
                TRY(close(p[0]));
                TRY(close(p[1]));
                run_cmd(pcmd->right);
                exit(EXIT_SUCCESS);
            }
            TRY(close(p[0]));
            TRY(close(p[1]));

            // Esperar a ambos hijos
            TRY_AND_ACCEPT_ECHILD(waitpid(pid[0], NULL, 0));
            TRY_AND_ACCEPT_ECHILD(waitpid(pid[1], NULL, 0));
            break;

        case BACK:
            bcmd = (struct backcmd*)cmd;
            if ((pid[0] = fork_back_child()) == 0) {
                run_cmd(bcmd->cmd);
                exit(EXIT_SUCCESS);
            }
            break;

        case SUBS:
            scmd = (struct subscmd*)cmd;
            if ((pid[0] = fork_subshell("fork SUBS")) == 0) {
                run_cmd(scmd->cmd);
                exit(EXIT_SUCCESS);
            }
            TRY_AND_ACCEPT_ECHILD(waitpid(pid[0], NULL, 0));
            break;

        case INV:
        default:
            panic("%s: estructura `cmd` desconocida\n", __func__);
    }

    DPRINTF(DBG_TRACE, "END\n");
}

void print_cmd(struct cmd* cmd) {
    struct execcmd* ecmd;
    struct redrcmd* rcmd;
    struct listcmd* lcmd;
    struct pipecmd* pcmd;
    struct backcmd* bcmd;
    struct subscmd* scmd;

    if (cmd == 0) return;

    switch (cmd->type) {
        case EXEC:
            ecmd = (struct execcmd*)cmd;
            if (ecmd->argv[0] != 0) printf("fork( exec( %s ) )", ecmd->argv[0]);
            break;

        case REDR:
            rcmd = (struct redrcmd*)cmd;
            printf("fork( ");
            if (rcmd->cmd->type == EXEC)
                printf("exec ( %s )", ((struct execcmd*)rcmd->cmd)->argv[0]);
            else
                print_cmd(rcmd->cmd);
            printf(" )");
            break;

        case LIST:
            lcmd = (struct listcmd*)cmd;
            print_cmd(lcmd->left);
            printf(" ; ");
            print_cmd(lcmd->right);
            break;

        case PIPE:
            pcmd = (struct pipecmd*)cmd;
            printf("fork( ");
            if (pcmd->left->type == EXEC)
                printf("exec ( %s )", ((struct execcmd*)pcmd->left)->argv[0]);
            else
                print_cmd(pcmd->left);
            printf(" ) => fork( ");
            if (pcmd->right->type == EXEC)
                printf("exec ( %s )", ((struct execcmd*)pcmd->right)->argv[0]);
            else
                print_cmd(pcmd->right);
            printf(" )");
            break;

        case BACK:
            bcmd = (struct backcmd*)cmd;
            printf("fork( ");
            if (bcmd->cmd->type == EXEC)
                printf("exec ( %s )", ((struct execcmd*)bcmd->cmd)->argv[0]);
            else
                print_cmd(bcmd->cmd);
            printf(" )");
            break;

        case SUBS:
            scmd = (struct subscmd*)cmd;
            printf("fork( ");
            print_cmd(scmd->cmd);
            printf(" )");
            break;

        case INV:
        default:
            panic("%s: estructura `cmd` desconocida\n", __func__);
    }
}
