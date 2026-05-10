/**
 * @file builtin_exit.c
 * @brief Implementation of the exit builtin command for the 42sh shell.
 * @author zweng
 */

#include "42sh.h"
#include "builtins.h"
#include <limits.h>

/**
 * @brief Check if a string is a valid numeric argument.
 * @details Allows optional leading '+' or '-', followed by digits only.
 *          Also rejects values that overflow long.
 * @param str The string to check.
 * @return 1 if numeric, 0 otherwise.
 */
static int	is_numeric(const char *str)
{
	int		i;
	long	val;
	int		sign;

	if (!str || !*str)
		return (0);
	i = 0;
	sign = 1;
	if (str[i] == '+' || str[i] == '-')
	{
		if (str[i] == '-')
			sign = -1;
		i++;
	}
	if (!str[i])
		return (0);
	val = 0;
	while (str[i])
	{
		if (str[i] < '0' || str[i] > '9')
			return (0);
		if (sign == 1 && (val > (LONG_MAX - (str[i] - '0')) / 10))
			return (0);
		if (sign == -1 && (val > (-(LONG_MIN + (str[i] - '0'))) / 10))
			return (0);
		val = val * 10 + (str[i] - '0');
		i++;
	}
	return (1);
}

int	builtin_exit(struct s_shell *shell, int argc, char **argv)
{
	if (shell->interactive)
		ft_putendl_fd("exit", 2);
	if (argc == 1)
	{
		shell->running = 0;
		return (shell->last_exit_status);
	}
	if (!is_numeric(argv[1]))
	{
		ft_putstr_fd("42sh: exit: ", 2);
		ft_putstr_fd(argv[1], 2);
		ft_putendl_fd(": numeric argument required", 2);
		shell->running = 0;
		shell->last_exit_status = 2;
		return (2);
	}
	if (argc > 2)
	{
		ft_putendl_fd("42sh: exit: too many arguments", 2);
		shell->last_exit_status = 1;
		return (1);
	}
	shell->running = 0;
	shell->last_exit_status = (unsigned char)ft_atoi(argv[1]);
	return (shell->last_exit_status);
}
