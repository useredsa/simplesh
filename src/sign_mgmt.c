#include "sign_mgmt.h"
#include "macros.h"

#include <signal.h>
#include <wait.h>

int nback_procs;
int back_procs[MAX_BACK_CHLD];

void handle_sigchld(int sig) {
    int old_errno = errno;
    int pid;
    while ((pid = waitpid((pid_t) -1, 0, WNOHANG)) > 0) {
        for (int i = 0; i < MAX_BACK_CHLD; i++) {
            if (back_procs[i] == pid) {
                printf("[%d]\n", pid);
                back_procs[i] = 0;
                nback_procs--;
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

int fork_back_child() {
    for (int i = 0; i < MAX_BACK_CHLD; i++) {
        if (back_procs[i] == 0) {
            TRY(back_procs[i] = fork_or_panic("fork BACK"));
            nback_procs++;
            if (back_procs[i] != 0)
                printf("[%d]\n", back_procs[i]);
            return back_procs[i];
        }
    }
    //TODO ask if good solution
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
