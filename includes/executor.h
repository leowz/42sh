/**
 * @file executor.h
 * @brief Defines the executor functions for handling AST nodes.
 * @author zweng, pulgamecanica
 */

#ifndef EXECUTOR_H
#define EXECUTOR_H

#include "ast.h"
#include "builtins.h"
#include "expander.h"

#define MAX_PIPELINE 256
#define MAX_SAVED_FDS 3

/** Default bucket count for the command-path hash table (prime, small). */
#define CMD_HASH_BUCKETS 61

/**
 * @brief Cached PATH lookup, indexed by command name in `shell->cmd_hash`.
 * @details `path` is heap-allocated and owned by the value. `hits` tracks
 *          how many times the cache satisfied a lookup (read by `hash`).
 */
typedef struct s_cmd_hash_value
{
	char	*path;
	size_t	hits;
}	t_cmd_hash_value;

/** Allocate `shell->cmd_hash` if not already present. */
void				cmd_hash_init(struct s_shell *shell);

/** Drop every entry and free the table itself. Safe to call repeatedly. */
void				cmd_hash_destroy(struct s_shell *shell);

/** Drop every entry but keep the table allocated (for `hash -r`). */
void				cmd_hash_clear(struct s_shell *shell);

/** Look up `name` without bumping the hit counter. */
t_cmd_hash_value	*cmd_hash_get(struct s_shell *shell, const char *name);

/**
 * @brief Insert or replace `name -> path` (path is duplicated internally).
 * @details Preserves the hit counter when `name` already exists, so
 *          `hash -p` doesn't reset history. Returns 1 on success, 0 on
 *          allocation failure (table left unchanged).
 */
int					cmd_hash_set(struct s_shell *shell,
						const char *name, const char *path);

/** Remove `name`. Returns 1 if an entry was removed, 0 otherwise. */
int					cmd_hash_delete(struct s_shell *shell, const char *name);

/** Iterate every (name, value) pair in insertion-agnostic order. */
void				cmd_hash_iter(struct s_shell *shell,
						void (*f)(const char *, t_cmd_hash_value *, void *),
						void *userdata);

/**
 * @brief Main dispatch function for executing AST nodes
 * @param shell Pointer to the central shell state
 * @param ast Pointer to the AST node to execute
 * @return Exit status code
 */
int executor_execute(struct s_shell *shell, t_ast *ast);

/**
 * @brief Node-type executors
 * @param shell Pointer to the central shell state
 * @param ast Pointer to the AST node to execute
 * @return Exit status code
 */
int execute_simple_command(struct s_shell *shell, t_cmd *cmd);
int execute_pipeline(struct s_shell *shell, t_ast *ast);
int execute_and(struct s_shell *shell, t_ast *ast);
int execute_or(struct s_shell *shell, t_ast *ast);
int execute_sequence(struct s_shell *shell, t_ast *ast);
int execute_subshell(struct s_shell *shell, t_ast *ast);
int execute_block(struct s_shell *shell, t_ast *ast);
int execute_background(struct s_shell *shell, t_ast *ast);

/**
 * @brief redirection setup
 * @param redirs List of redirection nodes
 * @param saved_fds Array to save original file descriptors
 * @return Exit status code
 */
int setup_redirections(t_list *redirs, int saved_fds[3]);

/**
 * @brief Restore redirections
 * @param saved_fds Array of saved file descriptors
 */
void restore_redirections(int saved_fds[3]);

/**
 * @brief Command search (PATH)
 * @param shell Pointer to the central shell state
 * @param name Name of the command to search for
 * @return Pointer to the found command, or NULL if not found
 */
char *find_command(struct s_shell *shell, const char *name);

/**
 * @brief Pipeline helper (called from pipe_child, does not return)
 * @param shell Pointer to the central shell state
 * @param cmd Pointer to the command node
 */
void exec_pipeline_external(struct s_shell *shell, t_cmd *cmd);

/**
 * @brief get exit status from wait status
 * @param wstatus Wait status code
 * @return Exit status code
 */
int get_exit_status(int wstatus);

void split_assignment(const char *assign, char **name, char **value);

/**
 * @brief Apply variable assignments to the shell (shared by the simple
 *        command and pipeline child paths).
 * @param shell The shell instance.
 * @param assigns List of "NAME=value" assignment strings.
 * @param do_export Whether to also export each variable.
 */
void apply_assignments(struct s_shell *shell, t_list *assigns, int do_export);

/**
 * @brief Print the diagnostic for an unrunnable command and return its
 *        exit status: 126 if the path exists but is not executable,
 *        127 if it could not be found.
 * @param name The command name as typed.
 * @return 126 or 127.
 */
int report_command_error(const char *name);

#endif
