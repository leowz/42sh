/**
 * @file parser_command.c
 * @brief file for parse command
 * @author jguillem
 */
#include "parser.h"

/**
 * @param cmd struct s_cmd
 * @brief allocate memory and define type for a NODE_COMMAND
 * @return pointer on struct s_ast
 */
t_ast	*ast_new_command(t_cmd *cmd)
{
	t_ast	*ast;

	ast = malloc(sizeof(t_ast));
	if (!ast)
		return (NULL);
	ast->type = NODE_COMMAND;
	ast->data.cmd = cmd;
	return (ast);
}

/**
 * @param p t_parser struct
 * @brief handle (subshell) and (block) then parser command
 * @return t_ast struct
 */
t_ast	*parse_command(t_parser *p)
{
	t_token	*token;

	token = parser_peek(p);
	if (!token)
		return (NULL);
	if (token->type == TOK_LPAREN)
		return (parse_subshell(p));
	else if (token->type == TOK_WORD && strcmp(token->value, "{") == 0)
		return (parse_block(p));
	return (parse_simple_command(p));
}

/**
 * @brief initialize t_cmd struct
 */
static t_cmd	*command_init(void)
{
	t_cmd	*command;

	command = malloc(sizeof(t_cmd));
	if (!command)
		return (NULL);
	command->argv = NULL;
	command->argc = 0;
	command->assignments = NULL;
	command->redirs = NULL;
	return (command);
}

/**
 * @param p t_parser struct
 * @param token t_token struct
 * @brief handle t_parser error field
 */
static void	parser_error_unexpected(t_parser *p, t_token *token)
{
	char	buf[256];

	if (!token || token->type == TOK_EOF)
		p->error = strdup("syntax error near unexpected token `newline'");
	else
	{
		snprintf(buf, sizeof(buf),
			"syntax error near unexpected token `%s'",
			token->value);
		p->error = strdup(buf);
	}
}

/**
 * @param p t_parser struct
 * @param command t_cmd struct
 * @brief allocate memory of t_cmd struct argv field
 * @return char **argv
 */
static char	**command_size(t_parser *p, t_cmd *command)
{
	t_token	*token;
	t_list	*start;
	char	**argv;

	while (parser_accept(p, TOK_NEWLINE));
	start = p->current;
	while ((token = parser_peek(p))
		&& (token->type == TOK_WORD || is_redir(token->type)))
	{
		token = parser_next(p);
		if (token->type == TOK_WORD)
			command->argc++;
		else
			parser_next(p);
	}
	if (command->argc == 0 && !command->assignments)
	{
		p->current = start;
		if (parser_peek(p)->type != TOK_HEREDOC
			&& parser_peek(p)->type != TOK_HEREDOC_STRIP)
			parser_error_unexpected(p, token);
		return (NULL);
	}
	argv = malloc(sizeof(char *) * (command->argc + 1));
	if (!argv)
		return (NULL);
	argv[command->argc] = NULL;
	p->current = start;
	return (argv);
}

/**
 * @param p t_parser struct
 * @param command t_cmd struct
 * @brief fill the t_cmd struct
 * @return t_ast struct
 */
static t_ast	*command_build(t_parser *p, t_cmd *command)
{
	int		i;
	t_token	*token;
	t_redir	*redir;

	i = 0;
	while ((token = parser_peek(p))
		&& (token->type == TOK_WORD || is_redir(token->type)))
	{
		token = parser_next(p);
		if (token->type == TOK_WORD)
			command->argv[i++] = strdup(token->value);
		else
		{
			redir = malloc(sizeof(t_redir));
			if (!redir)
				return (NULL);
			redir->heredoc_delim = NULL;
			redir->heredoc_fd = -1;
			redir->heredoc_quoted = 0;
			redir->type = token->type;
			redir->fd = token->io_number; 
			token = parser_next(p);
			if (!token || token->type != TOK_WORD)
			{
				parser_error_unexpected(p, token);
				return (NULL);
			}
			redir->target = strdup(token->value);
			heredoc_expand_config(redir);
			ft_lstappend(&(command->redirs), ft_lstnew(redir));
		}
	}
	return (ast_new_command(command));
}

/**
 * @param str char *
 * @brief check for valid assignment
 * @return 0 | 1
 */
static int	is_assignment(char *str)
{
	int	equal;

	equal = 0;
	if (!str)
		return (0);
	if (*str == '=' || (!isalpha(*str) && *str != '_'))
		return (0);
	while (*str)
	{
		if (*str == '=')
			equal = 1;
		else if (!isalnum(*str) && *str != '_')
			return (0);
		str++;
	}
	return (equal);
}

/**
 * @param p t_parser struct
 * @param command t_cmd struct
 * @brief append assignments to t_cmd struct
 */
static void	parse_assignment(t_parser *p, t_cmd *command)
{
	t_token	*token;
	while ((token = parser_peek(p))
		&& token->type == TOK_WORD
		&& is_assignment(token->value))
	{
		token = parser_next(p);
		ft_lstappend(&command->assignments, ft_lstnew(strdup(token->value)));
	}
}

/**
 * @param p struct s_parser
 * @brief handle commands and detect subshells
 * @return a pointer on a struct s_ast
 */
t_ast	*parse_simple_command(t_parser *p)
{
	t_cmd	*command;

	command = command_init();
	if (!command)
		return (NULL);
	parse_assignment(p, command);
	if (parser_peek(p)->type == TOK_LPAREN)
	{
		free(command);
		return (parse_subshell(p));
	}
	if (!(command->argv = command_size(p, command))
		&& parser_peek(p)->type != TOK_HEREDOC
		&& parser_peek(p)->type != TOK_HEREDOC_STRIP
		&& !command->assignments)
	{
		free(command);
		return (NULL);
	}
	return (command_build(p, command));
}
