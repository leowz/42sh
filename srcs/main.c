#include "lexer.h"
#include "parser.h"
#include "executor.h"
#include "42sh.h"

static int	shell_init(t_shell *shell, char **envp)
{
	ft_bzero(shell, sizeof(t_shell));
	shell->interactive = isatty(STDIN_FILENO);
	shell->running = 1;
	shell->terminal_fd = STDIN_FILENO;
	shell->shell_pgid = getpid();
	shell->env_dirty = 1;
	if (shell->interactive)
	{
		history_init(shell);
		signals_setup_interactive();
	}
	(void)envp;
	// init a 42shrc file here if it exists
	// init variables from envp here
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

static void parse_options(int argc, char *argv[], t_shell *shell) {
	int opt;
	char *cmd_entrypoint;

	cmd_entrypoint = NULL;
	while ((opt = getopt(argc, argv, "hic:")) != -1) {
		switch (opt) {
		case 'c':
			if (cmd_entrypoint) {
				fprintf(stderr, "Error: Multiple -c options provided.\n");
				exit(EXIT_FAILURE);
			}
			shell->cmd_entrypoint = optarg;
			break;
		case 'i':
			shell->interactive = 1;
			break;
		default: /* '?' */
			fprintf(stderr, """Usage: %s [-i] [-c command]\n",
							argv[0]);
			exit(EXIT_FAILURE);
		}
	}
}

#ifdef FT_EXTRA_VERBOSE
static void _display(t_list *tokens, t_ast *ast, char *line)
{
	char	*tok_json;
	lexer_display(tokens, line);
	tok_json = lexer_to_json(tokens, line);
	if (tok_json)
		printf("  \033[2mJSON → %s\033[0m\n\n", tok_json);

	char	*ast_json;

	ast_display(ast, line);
	ast_json = ast_to_json(ast, line, tok_json);
	if (ast_json)
	{
		printf("  \033[2mAST  → %s\033[0m\n\n",
			ast_json);
		free(ast_json);
	}
	free(tok_json);

}
#endif

static void process_line(t_shell *shell, char *line)
{
	/* Tokenize, display, and convert to JSON */
	t_list *tokens;
	t_ast  *ast;

	tokens = lexer_tokenize(line);
	if (!tokens)
	{
		fprintf(stderr, "Error: Failed to tokenize input.\n");
		return;
	}

	ast = parser_parse(tokens, shell);

#ifdef FT_EXTRA_VERBOSE
	_display(tokens, ast, line);
#endif

	lexer_free_tokens(tokens);
	if (!ast)
	{
		fprintf(stderr, "Error: Failed to parse tokens into AST.\n");
		return;
	}

	executor_execute(shell, ast);

	ast_free(ast); //this will leak!!!
}


int	main(int argc, char *argv[], char *envp[])
{
	t_shell	shell;
	char	*raw_line, *line;

	shell_init(&shell, envp);
	parse_options(argc, argv, &shell);
	while (shell.running)
	{
		signals_check(&shell);
		raw_line = read_line(&shell);
		if (!raw_line)
		{
			if (shell.interactive)
				write(STDOUT_FILENO, "exit\n", 5);
			break ;
		}
		line = ft_strtrim(raw_line);
		free(raw_line);
		if (*line == '\0')
		{
			free(line);
			continue ;
		}
		if (shell.interactive)
		{
			add_history(line);
			history_save(shell.history_file);
		}
		process_line(&shell, line);
		free(line);
	}
	shell_cleanup(&shell);
	return (shell.last_exit_status);
}
