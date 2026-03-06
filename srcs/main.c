#include "lexer.h"
#include "42sh.h"

static int	shell_init(t_shell *shell, char **envp)
{
	ft_bzero(shell, sizeof(t_shell));
	shell->interactive = isatty(STDIN_FILENO);
	shell->running = 1;
	shell->terminal_fd = STDIN_FILENO;
	shell->shell_pgid = getpid();
	shell->env_dirty = 1;
	(void)envp;
	if (shell->interactive)
	{
		history_init(shell);
	}
	return (0);
}

static void	shell_cleanup(t_shell *shell)
{
	if (shell->history_file)
	{
		history_save(shell->history_file);
		free(shell->history_file);
		shell->history_file = NULL;
	}
}

/*
** read_line: unified input function.
**
** Interactive mode  → readline("42sh$ ")
**   Returns a malloc'd string, or NULL on EOF (Ctrl-D on empty line).
**   readline handles cursor movement, history arrows, Ctrl-C, etc.
**
** Non-interactive   → getline(stdin)
**   Used when stdin is a pipe or file (scripts, -c testing).
**   Returns a malloc'd string, or NULL on EOF / error.
*/
static char	*read_line(t_shell *shell)
{
	char	*line;
	size_t	len;
	ssize_t	n;

	if (shell->interactive)
		return (readline("42sh$ "));
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

int	main(int argc, char **argv, char **envp)
{
	t_shell	shell;
	char	*line;

	(void)argc;
	(void)argv;
	shell_init(&shell, envp);
	while (shell.running)
	{
		line = read_line(&shell);
		if (!line)
		{
			break ;
		}
		if (*line == '\0')
		{
			free(line);
			continue ;
		}
		if (shell.interactive)
			add_history(line);

		/* Tokenize, display, and convert to JSON */
		{
			t_list	*tokens;

			tokens = lexer_tokenize(line);
			if (tokens)
			{
#ifdef FT_EXTRA_VERBOSE
				char	*json_path;
				lexer_display(tokens, line);
				json_path = lexer_to_json(tokens, line);
				if (json_path)
				{
					printf("  \033[2mJSON → %s\033[0m\n\n", json_path);
					free(json_path);
				}
#endif
				lexer_free_tokens(tokens);
			}
		}
		history_save(shell.history_file);
		free(line);
	}
	shell_cleanup(&shell);
	return (shell.last_exit_status);
}
