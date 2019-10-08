
//TODO add include guards in all files
#include "simplesh_syntactic.c"
#include <unistd.h>

/******************************************************************************
 * Funciones para la ejecución de comandos internos
 ******************************************************************************/

int run_cwd(struct execcmd* ecmd) {
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

static int EXIT = 0;
int run_exit(struct execcmd* ecmd) {
    //TODO
    EXIT = 1;
    return 0;
}

int run_cd(struct execcmd* ecmd) {
    static const int BUFFER_SIZE = 300;
    static char OLDPWD[300] = "", buff[300]; //TODO no deja poner BUFFER_SIZE ahí
    if (ecmd->argc > 2) {
        error("cwd: too many arguments");
        return 1; //TODO copied from bash
    }   
    char *dir;
    if (ecmd->argc == 1) {
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
        if (strcmp(ecmd->argv[1], "-") == 0) {
            if (strcmp(OLDPWD, "") == 0) {
                error("cd: no older directory\n");
                return 1;
            }
            strcpy(buff, OLDPWD);
            dir = buff;
        } else {
            dir = ecmd->argv[1];
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

static char inter_comm[][15] = {"cwd", "exit", "cd"};
static int (*inter_func[])(struct execcmd*) = {run_cwd, run_exit, run_cd};

/******************************************************************************
 * Funciones para la ejecución de la línea de órdenes
 ******************************************************************************/

int (*isInter(char* comm))(struct execcmd*) {
    for (int i = 0; i < sizeof(inter_comm) / sizeof(inter_comm[0]); i++)
        if (strcmp(inter_comm[i], comm) == 0) return inter_func[i];
    return NULL;
}

void exec_cmd(struct execcmd* ecmd) {
    assert(ecmd->type == EXEC);

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
    int p[2];
    int fd;

    DPRINTF(DBG_TRACE, "STR\n");

    if (cmd == 0) return;

    switch (cmd->type) {
        case EXEC:
            ecmd = (struct execcmd*)cmd;
            if (ecmd->argc == 0) return; //TODO check necessity or deeper error
            int (*func)(struct execcmd*) = isInter(ecmd->argv[0]);
            if (func != NULL) {
                (*func)(ecmd);
            } else {
                if (fork_or_panic("fork EXEC") == 0) {
                    exec_cmd(ecmd);
                }
                TRY(wait(NULL));
            }
            break;

        case REDR: //TODO do not use with internal commands
            rcmd = (struct redrcmd*)cmd;
            if (fork_or_panic("fork REDR") == 0) {
                TRY(close(rcmd->fd));
                if ((fd = open(rcmd->file, rcmd->flags, rcmd->mode)) < 0) {
                    perror("open");
                    exit(EXIT_FAILURE);
                }

                if (rcmd->cmd->type == EXEC) {
                    ecmd = (struct execcmd*)rcmd->cmd;
                    int (*func)(struct execcmd*) = isInter(ecmd->argv[0]);
                    if (func != NULL) {
                        (*func)(ecmd);
                    } else {
                        exec_cmd(ecmd);
                    }
                } else {
                    run_cmd(rcmd->cmd);
                }
                exit(EXIT_SUCCESS);
            }
            TRY(wait(NULL));
            break;

        case LIST:
            lcmd = (struct listcmd*)cmd;
            run_cmd(lcmd->left);
            run_cmd(lcmd->right);
            break;

        case PIPE: //TODO ambos procesos en subshells? Podemos evitar hacer hijos de más?
        //TODO do not use with internal commands
            pcmd = (struct pipecmd*)cmd;
            if (pipe(p) < 0) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }

            // Ejecución del hijo de la izquierda
            if (fork_or_panic("fork PIPE left") == 0) {
                TRY(close(STDOUT_FILENO));
                TRY(dup(p[1]));
                TRY(close(p[0]));
                TRY(close(p[1]));
                if (pcmd->left->type == EXEC) {
                    ecmd = (struct execcmd*)pcmd->left;
                    int (*func)(struct execcmd*) = isInter(ecmd->argv[0]);
                    if (func != NULL) {
                        (*func)(ecmd);
                    } else {
                        exec_cmd(ecmd);
                    }
                    exec_cmd(ecmd);
                } else {
                    run_cmd(pcmd->left);
                }
                exit(EXIT_SUCCESS);
            }

            // Ejecución del hijo de la derecha
            if (fork_or_panic("fork PIPE right") == 0) {
                TRY(close(STDIN_FILENO));
                TRY(dup(p[0]));
                TRY(close(p[0]));
                TRY(close(p[1]));
                if (pcmd->right->type == EXEC) {
                    ecmd = (struct execcmd*)pcmd->right;
                    int (*func)(struct execcmd*) = isInter(ecmd->argv[0]);
                    if (func != NULL) {
                        (*func)(ecmd);
                    } else {
                        exec_cmd(ecmd);
                    }
                    exec_cmd(ecmd);
                } else {
                    run_cmd(pcmd->right);
                }
                exit(EXIT_SUCCESS);
            }
            TRY(close(p[0]));
            TRY(close(p[1]));

            // Esperar a ambos hijos
            TRY(wait(NULL));
            TRY(wait(NULL));
            break;

        case BACK:  // TODO como crear un subshell y no hacer un wait? podemos
                    // evitar hacer hijos de más?
                    // Aunque checkeemos el caso EXEC, se nos pasa el caso
                    // redirección
                    // TODO do not use with internal commands
            bcmd = (struct backcmd*)cmd;
            if (fork_or_panic("fork BACK") == 0) {
                if (bcmd->cmd->type == EXEC) {
                    ecmd = (struct execcmd*)bcmd->cmd;
                    int (*func)(struct execcmd*) = isInter(ecmd->argv[0]);
                    if (func != NULL) {
                        (*func)(ecmd);
                    } else {
                        exec_cmd(ecmd);
                    }
                    exec_cmd(ecmd);
                } else {
                    run_cmd(bcmd->cmd);
                }
                exit(EXIT_SUCCESS);

            }
            break;

        case SUBS:
            scmd = (struct subscmd*)cmd;
            if (fork_or_panic("fork SUBS") == 0) {
                run_cmd(scmd->cmd);
                exit(EXIT_SUCCESS);
            }
            TRY(wait(NULL));
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

void free_cmd(struct cmd* cmd) { //TODO move to simples_structs.c
    struct execcmd* ecmd;
    struct redrcmd* rcmd;
    struct listcmd* lcmd;
    struct pipecmd* pcmd;
    struct backcmd* bcmd;
    struct subscmd* scmd;

    if (cmd == 0) return;

    switch (cmd->type) {
        case EXEC:
            break;

        case REDR:
            rcmd = (struct redrcmd*)cmd;
            free_cmd(rcmd->cmd);

            //free(rcmd->cmd);
            break;

        case LIST:
            lcmd = (struct listcmd*)cmd;

            free_cmd(lcmd->left);
            free_cmd(lcmd->right);

            //free(lcmd->right);
            //free(lcmd->left); //TODO ask if correct
            break;

        case PIPE:
            pcmd = (struct pipecmd*)cmd;

            free_cmd(pcmd->left);
            free_cmd(pcmd->right);

            // free(pcmd->right);
            // free(pcmd->left);
            break;

        case BACK:
            bcmd = (struct backcmd*)cmd;

            free_cmd(bcmd->cmd);

            // free(bcmd->cmd);
            break;

        case SUBS:
            scmd = (struct subscmd*)cmd;

            free_cmd(scmd->cmd);

            // free(scmd->cmd);
            break;

        case INV:
        default:
            panic("%s: estructura `cmd` desconocida\n", __func__);
    }
    free(cmd);
}
