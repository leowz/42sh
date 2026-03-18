/**
 * @file .parser_subshell.c
 * @brief file for parse subshell
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

	ast = malloc(sizeof(t_ast));
	if (!ast)
		return (NULL);
	ast->type = type;
	ast->data.group.child = child;
	ast->data.group.redirs = redirs;
	return (ast);	
}

/**
 * @param p struct s_parser pointer
 * @brief create a new  NODE_SUBSHELL with its child and redirs
 * @return pointer on struct s_ast
 */
t_ast	*parse_subshell(t_parser *p)
{
	t_ast	*child;
	t_list	*redirs;
	t_token	*token;
	t_token	*target;
	t_redir	*redir;

	if (!parser_accept(p, TOK_LPAREN))
		return (NULL);
	child = parse_sequence(p);
	if (!child)
		return (NULL);
	if (!parser_accept(p, TOK_RPAREN))
	{
		p->error = strdup("Syntax error : expected ')'");
		return (NULL);
	}
	redirs = NULL;
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
		redir->type = token->type;
		redir->fd = -1;
		redir->target = strdup(target->value);
		ft_lstappend(&redirs, ft_lstnew(redir));
	}
	return (ast_new_group(NODE_SUBSHELL, child, redirs));
}
