#include "simplesh_structs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "simplesh_macros.h"

struct cmd* execcmd(void) {
    struct execcmd* cmd;

    if ((cmd = malloc(sizeof(*cmd))) == NULL) {
        perror("execcmd: malloc");
        exit(EXIT_FAILURE);
    }
    memset(cmd, 0, sizeof(*cmd));
    cmd->type = EXEC;

    return (struct cmd*)cmd;
}

struct cmd* redrcmd(struct cmd* subcmd, char* file, char* efile, int flags,
                    mode_t mode, int fd) {
    struct redrcmd* cmd;

    if ((cmd = malloc(sizeof(*cmd))) == NULL) {
        perror("redrcmd: malloc");
        exit(EXIT_FAILURE);
    }
    memset(cmd, 0, sizeof(*cmd));
    cmd->type = REDR;
    cmd->cmd = subcmd;
    cmd->file = file;
    cmd->efile = efile;
    cmd->flags = flags;
    cmd->mode = mode;
    cmd->fd = fd;

    return (struct cmd*)cmd;
}

struct cmd* pipecmd(struct cmd* left, struct cmd* right) {
    struct pipecmd* cmd;

    if ((cmd = malloc(sizeof(*cmd))) == NULL) {
        perror("pipecmd: malloc");
        exit(EXIT_FAILURE);
    }
    memset(cmd, 0, sizeof(*cmd));
    cmd->type = PIPE;
    cmd->left = left;
    cmd->right = right;

    return (struct cmd*)cmd;
}

struct cmd* listcmd(struct cmd* left, struct cmd* right) {
    struct listcmd* cmd;

    if ((cmd = malloc(sizeof(*cmd))) == NULL) {
        perror("listcmd: malloc");
        exit(EXIT_FAILURE);
    }
    memset(cmd, 0, sizeof(*cmd));
    cmd->type = LIST;
    cmd->left = left;
    cmd->right = right;

    return (struct cmd*)cmd;
}

struct cmd* backcmd(struct cmd* subcmd) {
    struct backcmd* cmd;

    if ((cmd = malloc(sizeof(*cmd))) == NULL) {
        perror("backcmd: malloc");
        exit(EXIT_FAILURE);
    }
    memset(cmd, 0, sizeof(*cmd));
    cmd->type = BACK;
    cmd->cmd = subcmd;

    return (struct cmd*)cmd;
}

struct cmd* subscmd(struct cmd* subcmd) {
    struct subscmd* cmd;

    if ((cmd = malloc(sizeof(*cmd))) == NULL) {
        perror("subscmd: malloc");
        exit(EXIT_FAILURE);
    }
    memset(cmd, 0, sizeof(*cmd));
    cmd->type = SUBS;
    cmd->cmd = subcmd;

    return (struct cmd*)cmd;
}

void free_cmd(struct cmd* cmd) {
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
            break;

        case LIST:
            lcmd = (struct listcmd*)cmd;
            free_cmd(lcmd->left);
            free_cmd(lcmd->right);
            break;

        case PIPE:
            pcmd = (struct pipecmd*)cmd;
            free_cmd(pcmd->left);
            free_cmd(pcmd->right);
            break;

        case BACK:
            bcmd = (struct backcmd*)cmd;
            free_cmd(bcmd->cmd);
            break;

        case SUBS:
            scmd = (struct subscmd*)cmd;
            free_cmd(scmd->cmd);
            break;

        case INV:
        default:
            panic("%s: estructura `cmd` desconocida\n", __func__);
    }
    free(cmd);
}
