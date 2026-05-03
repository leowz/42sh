/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   variables.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: wengzhang <marvin@42.fr>                   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/06 00:00:00 by wengzhang         #+#    #+#             */
/*   Updated: 2026/04/06 00:00:00 by wengzhang        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "42sh.h"
#include "variables.h"
#include <stdlib.h>

/* TODO P3: implement variable storage (linked list of t_var) */

t_var	*var_get(t_shell *shell, const char *name)
{
	t_list	*node;
	t_var	*var;

	if (!shell || !name)
		return (NULL);
	node = shell->variables;
	while (node)
	{
		var = LST_VAR(node);
		if (var && var->name && ft_strequ(var->name, name))
			return (var);
		node = node->next;
	}
	return (NULL);
}

char	*var_get_value(t_shell *shell, const char *name)
{
	t_var	*var;

	var = var_get(shell, name);
	if (!var)
		return (NULL);
	return (var->value);
}

int	var_set(t_shell *shell, const char *name, const char *value)
{
	(void)shell;
	(void)name;
	(void)value;
	return (0);
}

int	var_unset(t_shell *shell, const char *name)
{
	(void)shell;
	(void)name;
	return (0);
}

int	var_export(t_shell *shell, const char *name)
{
	(void)shell;
	(void)name;
	return (0);
}

char	**var_get_environ(t_shell *shell)
{
	(void)shell;
	return (NULL);
}

void	var_init_from_environ(t_shell *shell, char **envp)
{
	(void)shell;
	(void)envp;
}
