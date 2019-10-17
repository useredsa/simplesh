#ifndef SIMPLESH_SYNTACTIC_H
#define SIMPLESH_SYNTACTIC_H

#include "cmd.h"

/******************************************************************************
 * Funciones para realizar el análisis sintáctico de la línea de órdenes
 ******************************************************************************/

/**
 * @brief Get the token object
 *
 * `get_token` recibe un puntero al principio de una cadena (`start_of_str`),
 * otro puntero al final de esa cadena (`end_of_str`) y, opcionalmente, dos
 * punteros para guardar el principio y el final del token, respectivamente.
 * `get_token` devuelve un *token* de la cadena de entrada.
 *
 * @param start_of_str
 * @param end_of_str
 * @param start_of_token
 * @param end_of_token
 * @return int
 */
int get_token(char** start_of_str, char const* end_of_str,
              char** start_of_token, char** end_of_token);

/**
 * @brief
 *
 *  * `peek` recibe un puntero al principio de una cadena (`start_of_str`), otro
 * puntero al final de esa cadena (`end_of_str`) y un conjunto de caracteres
 * (`delimiter`).
 *
 * El primer puntero pasado como parámero (`start_of_str`) avanza hasta el
 * primer carácter que no está en el conjunto de caracteres `WHITESPACE`.
 *
 * `peek` devuelve un valor distinto de `NULL` si encuentra alguno de los
 * caracteres en `delimiter` justo después de los caracteres en `WHITESPACE`.
 * @param start_of_str
 * @param end_of_str
 * @param delimiter
 * @return int
 */
int peek(char** start_of_str, char const* end_of_str, char* delimiter);

/**
 * @brief
 *
 * `parse_cmd` realiza el *análisis sintáctico* de la línea de órdenes
 * introducida por el usuario.
 *
 * `parse_cmd` utiliza `parse_line` para obtener una estructura `cmd`.
 * @param start_of_str
 * @return struct cmd*
 */
struct cmd* parse_cmd(char* start_of_str);

/**
 * @brief
 * `parse_line` realiza el análisis sintáctico de la línea de órdenes
 * introducida por el usuario.
 *
 * `parse_line` comprueba en primer lugar si la línea contiene alguna tubería.
 * Para ello `parse_line` llama a `parse_pipe` que a su vez verifica si hay
 * bloques de órdenes y/o redirecciones.  A continuación, `parse_line`
 * comprueba si la ejecución de la línea se realiza en segundo plano (con `&`)
 * o si la línea de órdenes contiene una lista de órdenes (con `;`).
 * @param start_of_str
 * @param end_of_str
 * @return struct cmd*
 */
struct cmd* parse_line(char** start_of_str, char* end_of_str);

/**
 * @brief
 * `parse_pipe` realiza el análisis sintáctico de una tubería de manera
 * recursiva si encuentra el delimitador de tuberías '|'.
 *
 * `parse_pipe` llama a `parse_exec` y `parse_pipe` de manera recursiva para
 * realizar el análisis sintáctico de todos los componentes de la tubería.
 * @param start_of_str
 * @param end_of_str
 * @return struct cmd*
 */
struct cmd* parse_pipe(char** start_of_str, char* end_of_str);

/**
 * @brief
 * `parse_exec` realiza el análisis sintáctico de un comando a no ser que la
 * expresión comience por un paréntesis, en cuyo caso se llama a `parse_subs`.
 *
 * `parse_exec` reconoce las redirecciones antes y después del comando.
 * @param start_of_str
 * @param end_of_str
 * @return struct cmd*
 */
struct cmd* parse_exec(char** start_of_str, char* end_of_str);

/**
 * @brief
 * `parse_subs` realiza el análisis sintáctico de un bloque de órdenes
 * delimitadas por paréntesis o `subshell` llamando a `parse_line`.
 *
 * `parse_subs` reconoce las redirecciones después del bloque de órdenes.
 * @param start_of_str
 * @param end_of_str
 * @return struct cmd*
 */
struct cmd* parse_subs(char** start_of_str, char* end_of_str);

/**
 * @brief
 * `parse_redr` realiza el análisis sintáctico de órdenes con
 * redirecciones si encuentra alguno de los delimitadores de
 * redirección ('<' o '>').
 * @param cmd
 * @param start_of_str
 * @param end_of_str
 * @return struct cmd*
 */
struct cmd* parse_redr(struct cmd* cmd, char** start_of_str, char* end_of_str);

/// Termina en NULL todas las cadenas de las estructuras `cmd`
struct cmd* null_terminate(struct cmd* cmd);

/******************************************************************************
 * Lectura de la línea de órdenes con la biblioteca libreadline
 ******************************************************************************/

/**
 * @brief Get the cmd object
 * `get_cmd` muestra un *prompt* y lee lo que el usuario escribe usando la
 * biblioteca readline. Ésta permite mantener el historial, utilizar las flechas
 * para acceder a las órdenes previas del historial, búsquedas de órdenes, etc.
 * @return char*
 */
char* get_cmd();

#endif
