#include "42sh.h"
#include "lexer.h"
#include "parser.h"
#include "executor.h"

/**
 * @brief Resolve the final value of @c shell->interactive.
 * @details Precedence (highest first): -c forces non-interactive (matches
 *          bash --posix -c behaviour), -i forces interactive, otherwise
 *          we autodetect based on whether stdin is a TTY.
 */
static void	resolve_interactive(t_shell *shell, int force_interactive)
{
	if (shell->cmd_entrypoint)
		shell->interactive = 0;
	else if (force_interactive)
		shell->interactive = 1;
	else
		shell->interactive = isatty(STDIN_FILENO);
}

static int	shell_init(t_shell *shell, char **envp, int force_interactive)
{
	resolve_interactive(shell, force_interactive);
	shell->running = 1;
	shell->terminal_fd = STDIN_FILENO;
	shell->shell_pgid = getpid();
	shell->env_dirty = 1;
	var_init_from_environ(shell, envp);
	if (shell->interactive)
	{
		history_init(shell);
		signals_setup_interactive();
		job_control_init(shell);
	}
	return (0);
}

static void	free_variables(t_shell *shell)
{
	t_list	*node;
	t_list	*next;
	t_var	*var;

	node = shell->variables;
	while (node)
	{
		next = node->next;
		var = LST_VAR(node);
		if (var)
		{
			free(var->name);
			free(var->value);
			free(var);
		}
		free(node);
		node = next;
	}
	shell->variables = NULL;
}

static void	free_env_cache(t_shell *shell)
{
	int	i;

	if (!shell->env)
		return ;
	i = 0;
	while (shell->env[i])
		free(shell->env[i++]);
	free(shell->env);
	shell->env = NULL;
}

static void	shell_cleanup(t_shell *shell)
{
	if (shell->history_file)
	{
		history_save(shell->history_file);
		free(shell->history_file);
		shell->history_file = NULL;
	}
	job_control_cleanup(shell);
	free_variables(shell);
	free_env_cache(shell);
	alias_clear(shell);
}

/**
 * @brief Parse argv flags into @p shell.
 * @details -c <cmd> stores the one-shot command (POSIX -c form) and
 *          implicitly forces non-interactive mode when applied later.
 *          -i forces interactive mode (returned via @p force_interactive
 *          so the caller can apply it before history/job-control setup).
 *          -h prints usage and exits 0.
 */
static void	parse_options(int argc, char *argv[], t_shell *shell,
		int *force_interactive)
{
	int	opt;

	*force_interactive = 0;
	while ((opt = getopt(argc, argv, "hic:")) != -1)
	{
		if (opt == 'c')
		{
			if (shell->cmd_entrypoint)
			{
				fprintf(stderr, "42sh: -c: only one command allowed\n");
				exit(2);
			}
			shell->cmd_entrypoint = optarg;
		}
		else if (opt == 'i')
			*force_interactive = 1;
		else if (opt == 'h')
		{
			printf("Usage: %s [-i] [-c command]\n", argv[0]);
			exit(0);
		}
		else
		{
			fprintf(stderr, "Usage: %s [-i] [-c command]\n", argv[0]);
			exit(2);
		}
	}
}

#ifdef FT_EXTRA_VERBOSE
static void _display(t_list *tokens, t_ast *ast, char *line)
{
	char	*tok_json;
	char	*ast_json;

	lexer_display(tokens, line);
	tok_json = lexer_to_json(tokens, line);
	if (tok_json)
		printf("  \033[2mJSON → %s\033[0m\n", tok_json);
	ast_display(ast, line);
	ast_json = ast_to_json(ast, line, tok_json);
	if (ast_json)
	{
		printf("  \033[2mAST  → %s\033[0m\n", ast_json);
		free(ast_json);
	}
	free(tok_json);
}
#endif

static void process_line(t_shell *shell, char *line)
{
	t_list		*tokens;
	t_ast		*ast;
	const char	*scan;

	scan = line;
	while (*scan == ' ' || *scan == '\t' || *scan == '\n')
		scan++;
	if (*scan == '\0')
		return ;
	tokens = lexer_tokenize(line);
	if (!tokens)
	{
		shell->last_exit_status = 1;
		return;
	}
	alias_expand_tokens(shell, &tokens);

	ast = parser_parse(tokens, shell);

#ifdef FT_EXTRA_VERBOSE
	_display(tokens, ast, line);
#endif

	lexer_free_tokens(tokens);
	if (!ast)
		return;

	executor_execute(shell, ast);

	ast_free(ast); //this will leak!!!
}


/**
 * @brief Run the interactive / piped REPL loop until EOF or @c shell.running=0.
 */
static void	repl_loop(t_shell *shell)
{
	char	*raw_line;
	char	*line;

	while (shell->running)
	{
		signals_check(shell);
		if (shell->interactive)
		{
			job_update_statuses(shell);
			job_notify(shell);
		}
		raw_line = shell_read_line(shell, "42sh$ ");
		if (!raw_line)
		{
			if (shell->interactive)
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
		if (shell->interactive)
		{
			add_history(line);
			history_save(shell->history_file);
		}
		process_line(shell, line);
		free(line);
	}
}

int	main(int argc, char *argv[], char *envp[])
{
	t_shell	shell;
	int		force_interactive;

	ft_bzero(&shell, sizeof(t_shell));
	parse_options(argc, argv, &shell, &force_interactive);
	shell_init(&shell, envp, force_interactive);
	if (shell.cmd_entrypoint)
		process_line(&shell, shell.cmd_entrypoint);
	else
		repl_loop(&shell);
	shell_cleanup(&shell);
	return (shell.last_exit_status);
}
