#ifndef SIMPLESH_CMD_H
#define SIMPLESH_CMD_H

#include "macros.h"
#include "cmd.h"

/******************************************************************************
 * Funciones para la ejecución de comandos internos
 ******************************************************************************/

/// Variable global que indica si se ha ejecutado el comando exit
extern int EXIT;

void run_cmd(struct cmd* cmd);

void print_cmd(struct cmd* cmd);

#endif
