#ifndef SIMPLESH_SIGN_MGMT_H
#define SIMPLESH_SIGN_MGMT_H


#define MAX_BACK_CHLD 8

/**
 * @brief Añade un proceso a la lista de procesos en segundo plano.
 */
int fork_back_child();

/**
 * @brief Determina el comportamiento del proceso ante las señales.
 */
void set_sign_mgmt();


/**
 * @brief Comando interno bjobs
 */
int run_bjobs(char **argv, int argc);

#endif 