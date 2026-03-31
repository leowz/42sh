/**
 * @file .parser_group.c
 * @brief file for parse group
 * @author jguillem
 */
#include "parser.h"

/**
 * @param type enum e_node_type
 * @param child struct s_ast
 * @param redirs struct s_list with a t_redir * as content
 * @brief helper function for the parse_subshell function
 * @return pointer on struct s_ast
 */
static t_ast	*ast_new_group(t_node_type type, t_ast *child, t_list *redirs)
{
	t_ast	*ast;
	t_group	*group;

	ast = malloc(sizeof(t_ast));
	if (!ast)
		return (NULL);
	group = malloc(sizeof(t_group));
	if (!group)
	{
		free(ast);
		return (NULL);
	}
	group->child = child;
	group->redirs = redirs;
	ast->type = type;
	ast->data.group = group;
	return (ast);	
}

/**
 * @param p struct s_parser pointer
 * @param child struct s_ast pointer
 * @paran node enum e_node_type
 * @brief factorization of the end of parse_subshell and parse_block
 * @return pointer on struct s_ast
 */
static t_ast	*parse_group(t_parser *p, t_ast *child, t_node_type node)
{
	t_list	*redirs = NULL;
	t_redir	*redir;
	t_token	*token;
	t_token	*target;

	token = parser_peek(p);
	while (token && is_redir(token->type))
	{
		token = parser_next(p);
		target = parser_next(p);
		if (!target || target->type != TOK_WORD)
		{
			p->error = strdup("Syntax error : expected filename");
			return (NULL);
		}
		redir = malloc(sizeof(t_redir));
		if (!redir)
			return (NULL);
		redir->heredoc_delim = NULL;
		redir->heredoc_content = NULL;
		redir->heredoc_quoted = 0;
		redir->type = token->type;
		redir->fd = token->io_number;
		redir->target = strdup(target->value);
		heredoc_expand_config(redir);
		ft_lstappend(&redirs, ft_lstnew(redir));
		token = parser_peek(p);
	}
	return (ast_new_group(node, child, redirs));
}

/**
 * @param p struct s_parser pointer
 * @brief create a new  NODE_SUBSHELL with its child and redirs
 * @return pointer on struct s_ast
 */
t_ast	*parse_subshell(t_parser *p)
{
	t_ast	*child;

	if (!parser_accept(p, TOK_LPAREN))
		return (NULL);
	child = parse_list(p);
	if (!child)
		return (NULL);
	if (!parser_accept(p, TOK_RPAREN))
	{
		p->error = strdup("Syntax error : expected ')'");
		return (NULL);
	}
	return (parse_group(p, child, NODE_SUBSHELL));
}

/**
 * @param p struct s_parser pointer
 * @brief create a new  NODE_BLOCK with its child and redirs
 * @return pointer on struct s_ast
 */
t_ast	*parse_block(t_parser *p)
{
	t_ast	*child;
	t_token	*token;

	token = parser_next(p);
	if (!token || token->type != TOK_WORD || strcmp(token->value, "{"))
		return (NULL);
	child = parse_list(p);
	if (!child)
		return (NULL);
	token = parser_next(p);
	if (!token || token->type != TOK_WORD || strcmp(token->value, "}"))
	{
		p->error = strdup("Syntax error : expected '}'");
		return (NULL);
	}
	return (parse_group(p, child, NODE_BLOCK));
}
