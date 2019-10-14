#ifndef SIMPLESH_EXECUTE_H
#define SIMPLESH_EXECUTE_H

#include "simplesh_structs.h"

/******************************************************************************
 * Funciones para la ejecución de comandos internos
 ******************************************************************************/

/// Variable global que indica si se ha ejecutado el comando exit
extern int EXIT;

void run_cmd(struct cmd* cmd);

void print_cmd(struct cmd* cmd);

#endif
