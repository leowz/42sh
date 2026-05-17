/**
 * @file builtin_unalias.c
 * @brief Implementation of the `unalias` builtin command.
 * @author zweng
 *
 * Removes named aliases, or every alias with `-a`.
 */
#include "42sh.h"
#include "builtins.h"
#include "aliases.h"

int	builtin_unalias(struct s_shell *shell, int argc, char **argv)
{
	int	i;
	int	result;

	if (!shell || !argv)
		return (1);
	if (argc < 2)
	{
		ft_putendl_fd("42sh: unalias: usage: unalias [-a] name [name ...]", 2);
		shell->last_exit_status = 2;
		return (2);
	}
	if (ft_strcmp(argv[1], "-a") == 0)
	{
		alias_clear(shell);
		shell->last_exit_status = 0;
		return (0);
	}
	result = 0;
	i = 1;
	while (i < argc)
	{
		if (alias_unset(shell, argv[i]) != 0)
		{
			ft_putstr_fd("42sh: unalias: ", 2);
			ft_putstr_fd(argv[i], 2);
			ft_putendl_fd(": not found", 2);
			result = 1;
		}
		i++;
	}
	shell->last_exit_status = result;
	return (result);
}
