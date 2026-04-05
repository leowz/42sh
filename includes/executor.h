/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   executor.h                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: wengzhang <marvin@42.fr>                   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/22 21:00:00 by wengzhang         #+#    #+#             */
/*   Updated: 2026/02/22 21:00:00 by wengzhang        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef EXECUTOR_H
# define EXECUTOR_H

# include "ast.h"
# include "builtins.h"
# include "expander.h"

# define MAX_PIPELINE 256
# define MAX_SAVED_FDS 3

/*
** Main dispatch
*/
int		executor_execute(struct s_shell *shell, t_ast *ast);

/*
** Node-type executors
*/
int		execute_simple_command(struct s_shell *shell, t_cmd *cmd);
int		execute_pipeline(struct s_shell *shell, t_ast *ast);
int		execute_and(struct s_shell *shell, t_ast *ast);
int		execute_or(struct s_shell *shell, t_ast *ast);
int		execute_sequence(struct s_shell *shell, t_ast *ast);
int		execute_subshell(struct s_shell *shell, t_ast *ast);
int		execute_block(struct s_shell *shell, t_ast *ast);
int		execute_background(struct s_shell *shell, t_ast *ast);

/*
** Redirections
*/
int		setup_redirections(t_list *redirs, int saved_fds[3]);
void	restore_redirections(int saved_fds[3]);

/*
** Heredoc
*/
int		setup_heredoc(t_redir *redir);

/*
** Command search (PATH)
*/
char	*find_command(struct s_shell *shell, const char *name);

/*
** Pipeline helper (called from pipe_child, does not return)
*/
void	exec_pipeline_external(struct s_shell *shell, t_cmd *cmd);

/*
** Utilities
*/
int		get_exit_status(int wstatus);
void	split_assignment(const char *assign, char **name, char **value);

#endif
