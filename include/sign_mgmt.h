#ifndef SIMPLESH_SIGN_MGMT_H
#define SIMPLESH_SIGN_MGMT_H


#define MAX_BACK_CHLD 100

/**
 * @brief Añade un proceso a la lista de procesos en segundo plano.
 */
int fork_back_child();

/**
 * @brief Determina el comportamiento del proceso ante las señales.
 */
void set_sign_mgmt();

#endif 