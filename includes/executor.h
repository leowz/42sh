/**
 * @file executor.h
 * @brief defines the executor functions for handling AST nodes.
 * @author zweng, pulgamecanica
 */

#ifndef EXECUTOR_H
# define EXECUTOR_H

# include "ast.h"
# include "builtins.h"
# include "expander.h"

# define MAX_PIPELINE 256
# define MAX_SAVED_FDS 3

/**
 * @brief Main dispatch function for executing AST nodes
 * @param shell Pointer to the central shell state
 * @param ast Pointer to the AST node to execute
 * @return Exit status code
 */
int		executor_execute(struct s_shell *shell, t_ast *ast);

/**
 * @brief Node-type executors
 * @param shell Pointer to the central shell state
 * @param ast Pointer to the AST node to execute
 * @return Exit status code
 */
int		execute_simple_command(struct s_shell *shell, t_cmd *cmd);
int		execute_pipeline(struct s_shell *shell, t_ast *ast);
int		execute_and(struct s_shell *shell, t_ast *ast);
int		execute_or(struct s_shell *shell, t_ast *ast);
int		execute_sequence(struct s_shell *shell, t_ast *ast);
int		execute_subshell(struct s_shell *shell, t_ast *ast);
int		execute_block(struct s_shell *shell, t_ast *ast);
int		execute_background(struct s_shell *shell, t_ast *ast);

/**
 * @brief redirection setup
 * @param redirs List of redirection nodes
 * @param saved_fds Array to save original file descriptors
 * @return Exit status code
 */
int		setup_redirections(t_list *redirs, int saved_fds[3]);

/**
 * @brief Restore redirections
 * @param saved_fds Array of saved file descriptors
 */
void	restore_redirections(int saved_fds[3]);

/**
 * @brief Heredoc setup
 * @param redir Pointer to the redirection node
 * @return Exit status code
 */
int		setup_heredoc(t_redir *redir);

/**
 * @brief Command search (PATH)
 * @param shell Pointer to the central shell state
 * @param name Name of the command to search for
 * @return Pointer to the found command, or NULL if not found
 */
char	*find_command(struct s_shell *shell, const char *name);

/**
 * @brief Pipeline helper (called from pipe_child, does not return)
 * @param shell Pointer to the central shell state
 * @param cmd Pointer to the command node
 */
void	exec_pipeline_external(struct s_shell *shell, t_cmd *cmd);

/**
 * @brief get exit status from wait status
 * @param wstatus Wait status code
 * @return Exit status code
 */
int		get_exit_status(int wstatus);

void	split_assignment(const char *assign, char **name, char **value);

#endif
