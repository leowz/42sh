/**
 * @file builtin_history.c
 * @brief Implementation of the history builtin command for the 42sh shell.
 * @author pulgamecanica
 *
 * ## Notes
 * 
 * - history_list() returns a NULL-terminated array of HIST_ENTRY* (readline).
 * - history_base is the number assigned to the first entry (typically 1).
 * - history_length is the total number of entries currently in the list.
 */

#include "42sh.h"
#include "builtins.h"

/**
 * @brief Print the command history list
 * @details Prints all entries of the history
 * 
 * @param list The list of history entries obtained from readline's history_list()
 * @param start The starting index in the history list to print from
 * @param end The ending index in the history list to print to (exclusive)
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

/**
 * @brief display the command history list
 * 
 * @details This function displays the command history list in a format similar to bash and ksh.
 * 
 * @param shell The shell context (not used in this function).
 * @param argc The number of arguments passed to the history command.
 * @param argv The arguments passed to the history command. If a numeric argument is provided, it specifies how many of the most recent entries to display.
 * 
 * @return int Always returns 0.
 */
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
