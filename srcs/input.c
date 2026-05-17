/**
 * @file input.c
 * @brief The shell's single stdin reader.
 * @author wengzhang
 *
 * `shell_read_line` is the one place the shell consumes a line of input.
 * The REPL loop and the heredoc collector both call it, so command lines
 * and heredoc bodies travel through a single stdin cursor and can never
 * desynchronise — the root-cause fix for the non-interactive heredoc bug.
 */
#include "42sh.h"

char	*shell_read_line(t_shell *shell, const char *prompt)
{
	char	*line;
	size_t	len;
	ssize_t	n;

	if (shell->interactive)
		return (readline(prompt));
	line = NULL;
	len = 0;
	n = getline(&line, &len, stdin);
	if (n <= 0)
	{
		free(line);
		return (NULL);
	}
	if (line[n - 1] == '\n')
		line[n - 1] = '\0';
	return (line);
}
