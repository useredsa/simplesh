#include "sign_mgmt.h"
#include "macros.h"

#include <signal.h>
#include <wait.h>
#include <getopt.h>

int back_procs[MAX_BACK_CHLD];

void handle_sigchld(int sig) {
    int old_errno = errno;
    int pid;
    while ((pid = waitpid((pid_t) -1, 0, WNOHANG)) > 0) {
        for (int i = 0; i < MAX_BACK_CHLD; i++) {
            if (back_procs[i] == pid) {
                printf("[%d]\n", pid);
                back_procs[i] = 0;
                break;
            }
        }
    }
    errno = old_errno;
}

void clear_back_list() {
    for (int i = 0; i < MAX_BACK_CHLD; i++) {
        back_procs[i] = 0;
    }
}

int fork_subshell() {
    int pid;
    pid = fork_or_panic("fork SUBS");
    if (pid == 0) {
        clear_back_list();
    }
    return pid;
}

int fork_back_child() {
    for (int i = 0; i < MAX_BACK_CHLD; i++) {
        if (back_procs[i] == 0) {
            TRY(back_procs[i] = fork_or_panic("fork BACK"));
            if (back_procs[i] != 0)
                printf("[%d]\n", back_procs[i]);
            return back_procs[i];
        }
    }
    perror("Maximum number of background process reached\n");
    return -1;
}

void set_sign_mgmt() {
    // Block SIGINT
    sigset_t set;
    TRY(sigemptyset(&set));
    TRY(sigaddset(&set, SIGINT));
    TRY(sigprocmask(SIG_BLOCK, &set, NULL));

    // Ignore SIGQUIT
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    TRY(sigemptyset(&sa.sa_mask));
    TRY(sigaction(SIGQUIT, &sa, NULL));

    // Manage SIGCHLD
    /* Alternative solution using previous handling options
        TRY(sigaction(SIGCHLD, NULL, &sa));
        sa.sa_handler = &handle_sigchld;
    */
    sa.sa_handler = &handle_sigchld;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    TRY(sigemptyset(&sa.sa_mask));
    TRY(sigaction(SIGCHLD, &sa, 0));
}


int run_bjobs(char **argv, int argc) {
    int optionk = 0;
    int opt;
    optind = 1;
    while ((opt = getopt(argc, argv, "hk")) != -1) {
        switch (opt) {
            case 'k':
                optionk = 1;
                break;

            default:
            case '?':
            case ':':
            case 'h':
                printf(
                    "Uso: bjobs [-k] [-h]\n"
                    "     Opciones:\n"
                    "     -k Mata a todos los procesos en segundo plano.\n"
                    "     -h Ayuda\n\n");
                return 0;
        }
    }

    if (optionk) {
        for (int i = 0; i < MAX_BACK_CHLD; i++) {
            if (back_procs[i] != 0) {
                kill(back_procs[i], SIGTERM);
            }
        }
        return 0;
    }

    for (int i = 0; i < MAX_BACK_CHLD; i++) {
        if (back_procs[i] != 0) {
            printf("[%d]\n", back_procs[i]);
        }
    }
    return 0;
}
