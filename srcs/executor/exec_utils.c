/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exec_utils.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: wengzhang <marvin@42.fr>                   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/27 00:00:00 by wengzhang         #+#    #+#             */
/*   Updated: 2026/03/27 00:00:00 by wengzhang        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "42sh.h"
#include "executor.h"

/*
** Convert raw waitpid status to shell exit code.
** - Normal exit: WEXITSTATUS (0-255)
** - Killed by signal: 128 + signal number
*/
int	get_exit_status(int wstatus)
{
	if (WIFEXITED(wstatus))
		return (WEXITSTATUS(wstatus));
	if (WIFSIGNALED(wstatus))
		return (128 + WTERMSIG(wstatus));
	return (1);
}

/*
** Split "NAME=value" at first '=' into separate name and value strings.
** Caller must free both *name and *value.
** If no '=' found, *name = dup of assign, *value = dup of "".
*/
void	split_assignment(const char *assign, char **name, char **value)
{
	const char	*eq;

	eq = ft_strchr(assign, '=');
	if (eq)
	{
		*name = ft_strsub(assign, 0, eq - assign);
		*value = ft_strdup(eq + 1);
	}
	else
	{
		*name = ft_strdup(assign);
		*value = ft_strdup("");
	}
}
