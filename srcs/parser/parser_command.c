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
	ast->type = NODE_COMMAND;
	ast->data.cmd = *cmd;
	return (ast);
}

/**
 * @param p struct s_parser
 * @brief handle commands and detect subshells
 * @return a pointer on a struct s_ast
 */
t_ast	*parse_command(t_parser *p)
{
	t_cmd	*command;
	t_token	*token;
	t_list	*mem;
	t_redir	*redir;
	int		i;

	token = parser_peek(p);
	if (token && token->type == TOK_LPAREN)
		return (parse_subshell(p));
	if (!token || (token->type != TOK_WORD && !is_redir(token->type)))
		return (NULL);
	mem = p->current;
	i = 0;
	while ((token = parser_peek(p))
		&& (token->type == TOK_WORD || is_redir(token->type)))
	{
		token = parser_next(p);
		if (token->type == TOK_WORD)
			i++;
		else
			parser_next(p);
	}
	command = malloc(sizeof(t_cmd));
	command->assignments = NULL;
	command->redirs = NULL;
	command->argc = i;
	command->argv = malloc(sizeof(char *) * (command->argc + 1));
	command->argv[command->argc] = NULL;
	p->current = mem;
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
			redir->type = token->type;
			token = parser_next(p);
			if (!token || token->type != TOK_WORD)
			{
				p->error = strdup("syntax error: bad redirection");
				return (NULL);
			}
			redir->fd = -1; 
			redir->target = strdup(token->value);
			ft_lstappend(&(command->redirs), ft_lstnew(redir));
		}
	}
	return (ast_new_command(command));
}
