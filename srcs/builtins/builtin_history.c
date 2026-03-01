/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   builtin_history.c                                 :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pulgamecanica <pulgamecanica@student.42.fr> +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 00:00:00 by pulgamecanica    #+#    #+#             */
/*   Updated: 2026/02/26 00:00:00 by pulgamecanica    ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "42sh.h"
#include "builtins.h"

/*
** builtin_history — display the command history list.
**
** Usage: history [n]
**
** With no arguments: prints all entries with their history numbers,
** matching the format of bash / ksh:
**   <5-wide number>  <command>
**
** With a numeric argument n: prints only the last n entries.
**
** history_list() returns a NULL-terminated array of HIST_ENTRY* (readline).
** history_base is the number assigned to the first entry (typically 1).
** history_length is the total number of entries currently in the list.
*/
static void	print_entries(HIST_ENTRY **list, int start, int end)
{
	int	i;

	i = start;
	while (i < end)
	{
		printf("%5d  %s\n", history_base + i, list[i]->line);
		i++;
	}
}

int	builtin_history(struct s_shell *shell, int argc, char **argv)
{
	HIST_ENTRY	**list;
	int			n;
	int			start;

	(void)shell;
	list = history_list();
	if (!list)
		return (0);
	if (argc >= 2)
	{
		n = ft_atoi(argv[1]);
		if (n <= 0)
			return (0);
		start = history_length - n;
		if (start < 0)
			start = 0;
	}
	else
		start = 0;
	print_entries(list, start, history_length);
	return (0);
}
