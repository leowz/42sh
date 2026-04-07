/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtins.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: wengzhang <marvin@42.fr>                   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/22 21:00:00 by wengzhang         #+#    #+#             */
/*   Updated: 2026/04/01 20:10:06 by jguillem         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef BUILTINS_H
# define BUILTINS_H

# include "42sh.h"
/*
** Builtin function type
*/
typedef int				(*t_builtin_fn)(struct s_shell *shell, int argc,
							char **argv);

/*
** Builtin registry
*/
t_builtin_fn			builtin_get(const char *name);
int						builtin_is_builtin(const char *name);

/*
** Builtin implementations
*/
int						builtin_echo(struct s_shell *shell, int argc,
							char **argv);
int						builtin_cd(struct s_shell *shell, int argc,
							char **argv);
int						builtin_exit(struct s_shell *shell, int argc,
							char **argv);
int						builtin_type(struct s_shell *shell, int argc,
							char **argv);
int						builtin_export(struct s_shell *shell, int argc,
							char **argv);
int						builtin_unset(struct s_shell *shell, int argc,
							char **argv);
int						builtin_set(struct s_shell *shell, int argc,
							char **argv);
int						builtin_jobs(struct s_shell *shell, int argc,
							char **argv);
int						builtin_fg(struct s_shell *shell, int argc,
							char **argv);
int						builtin_bg(struct s_shell *shell, int argc,
							char **argv);
int						builtin_history(struct s_shell *shell, int argc,
							char **argv);

#endif
