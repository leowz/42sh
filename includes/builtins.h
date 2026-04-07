/**
 * @file builtins.h
 * @brief Header for builtin function registry and declarations.
 * @details Defines the t_builtin_fn type and declares builtin functions.
 *          Also includes the builtin lookup function.
 * @author zweng, pulgamecanica
 */

#ifndef BUILTINS_H
# define BUILTINS_H

# include "42sh.h"

 /* Forward declaration to avoid circular dependency with 42sh.h */
typedef struct s_shell	t_shell;

/**
 * @brief Builtin function type
 * @param shell Pointer to the central shell state, for accessing variables, jobs, etc.
 * @param argc Argument count (number of elements in argv)
 * @param argv Argument vector (array of strings, with argv[0] being the command name)
 * @return Exit status code (0 for success, non-zero for failure)
*/
typedef int	(*t_builtin_fn)(struct s_shell *shell, int argc, char **argv);
							
/**
 * @brief Builtin registry
 * @param name Name of the builtin function to retrieve
 * @return Pointer to the builtin function, or NULL if not found
 */
t_builtin_fn	builtin_get(const char *name);
int				builtin_is_builtin(const char*name);
int				builtin_echo(struct s_shell *shell, int argc, char **argv);
int				builtin_cd(struct s_shell *shell, int argc, char **argv);
int				builtin_exit(struct s_shell *shell, int argc, char **argv);
int				builtin_type(struct s_shell *shell, int argc, char **argv);
int				builtin_export(struct s_shell *shell, int argc, char **argv);
int				builtin_unset(struct s_shell *shell, int argc, char **argv);
int				builtin_set(struct s_shell *shell, int argc, char **argv);
int				builtin_jobs(struct s_shell *shell, int argc, char **argv);
int				builtin_fg(struct s_shell *shell, int argc, char **argv);
int				builtin_bg(struct s_shell *shell, int argc, char **argv);
int				builtin_history(struct s_shell *shell, int argc, char **argv);
#endif
