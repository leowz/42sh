/**
 * @file builtin_type.c
 * @brief Implementation of the type builtin command for the 42sh shell.
 * @author zweng
 */

#include "42sh.h"
#include "builtins.h"
#include "executor.h"

/**
 * @brief Display the type of each argument.
 * @details For each name:
 *          - If it's a builtin: "name is a shell builtin"
 *          - If found in PATH: "name is /path/to/name"
 *          - Otherwise: "42sh: type: name: not found" (exit 1)
 * @param shell The shell instance.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return 0 if all found, 1 if any not found.
 */
int	builtin_type(struct s_shell *shell, int argc, char **argv)
{
	int		i;
	int		status;
	char	*path;

	status = 0;
	i = 1;
	while (i < argc)
	{
		if (builtin_is_builtin(argv[i]))
			printf("%s is a shell builtin\n", argv[i]);
		else
		{
			path = find_command(shell, argv[i]);
			if (path)
			{
				printf("%s is %s\n", argv[i], path);
				free(path);
			}
			else
			{
				ft_putstr_fd("42sh: type: ", 2);
				ft_putstr_fd(argv[i], 2);
				ft_putendl_fd(": not found", 2);
				status = 1;
			}
		}
		i++;
	}
	shell->last_exit_status = status;
	return (status);
}
