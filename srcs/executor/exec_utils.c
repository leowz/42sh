/**
 * @file exec_utils.c
 * @brief Utility functions for command execution in 42sh.
 * @author wengzhang, pulgamecanica
 */

#include "42sh.h"
#include "executor.h"

/**
 * @brief Convert raw waitpid status to shell exit code.
 * @details - Normal exit: WEXITSTATUS (0-255)
 * @details - Killed by signal: 128 + signal number
 * @param wstatus The waitpid status.
 * @return The shell exit code.
 */
int	get_exit_status(int wstatus)
{
	if (WIFEXITED(wstatus))
		return (WEXITSTATUS(wstatus));
	if (WIFSIGNALED(wstatus))
		return (128 + WTERMSIG(wstatus));
	return (1);
}

/**
 * @brief Split "NAME=value" at first '=' into separate name and value strings.
 * @details Caller must free both *name and *value.
 * @details If no '=' found, *name = dup of assign, *value = dup of "".
 * @param assign The assignment string.
 * @param name The pointer to store the name.
 * @param value The pointer to store the value.
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
