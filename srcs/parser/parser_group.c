/**
 * @file .parser_group.c
 * @brief file for parse group
 * @author jguillem
 */
#include "parser.h"

t_ast	*ast_new_group(t_node_type type, t_ast *child, t_list *redirs)
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
 * @brief Free the @c child AST and any partially-built @c redirs list.
 * @details Called from every error path in @c parse_group, @c parse_subshell
 *          and @c parse_block so a half-built subshell/block cannot leak
 *          its inner AST plus accumulated redirections on a syntax error
 *          (e.g. random-binary input that parses partway then dies).
 */
static void	parse_group_cleanup(t_ast *child, t_list *redirs)
{
	ast_free(child);
	ft_lstdel(&redirs, (void (*)(void *))&redir_free);
}

/**
 * @param p struct s_parser pointer
 * @param child struct s_ast pointer
 * @param node enum e_node_type
 * @brief factorization of the end of parse_subshell and parse_block
 * @return pointer on struct s_ast
 */
static t_ast	*parse_group(t_parser *p, t_ast *child, t_node_type node)
{
	t_list	*redirs = NULL;
	t_redir	*redir;
	t_token	*token;
	t_token	*target;
	t_ast	*ast;

	token = parser_peek(p);
	while (token && is_redir(token->type))
	{
		token = parser_next(p);
		target = parser_next(p);
		if (!target || target->type != TOK_WORD)
		{
			if (!p->error)
				p->error = strdup("Syntax error : expected filename");
			parse_group_cleanup(child, redirs);
			return (NULL);
		}
		redir = malloc(sizeof(t_redir));
		if (!redir)
		{
			parse_group_cleanup(child, redirs);
			return (NULL);
		}
		redir->target = NULL;
		redir->heredoc_delim = NULL;
		redir->heredoc_fd = -1;
		redir->heredoc_quoted = 0;
		redir->type = token->type;
		redir->fd = token->io_number;
		redir->target = strdup(target->value);
		heredoc_expand_config(redir);
		ft_lstappend(&redirs, ft_lstnew(redir));
		token = parser_peek(p);
	}
	ast = ast_new_group(node, child, redirs);
	if (!ast)
		parse_group_cleanup(child, redirs);
	return (ast);
}

t_ast	*parse_subshell(t_parser *p)
{
	t_ast	*child;

	if (!parser_accept(p, TOK_LPAREN))
		return (NULL);
	while (parser_accept(p, TOK_NEWLINE))
		;
	child = parse_list(p);
	if (!child)
		return (NULL);
	while (parser_accept(p, TOK_NEWLINE))
		;
	if (!parser_accept(p, TOK_RPAREN))
	{
		if (!p->error)
			p->error = strdup("Syntax error : expected ')'");
		ast_free(child);
		return (NULL);
	}
	return (parse_group(p, child, NODE_SUBSHELL));
}

t_ast	*parse_block(t_parser *p)
{
	t_ast	*child;
	t_token	*token;

	token = parser_next(p);
	if (!token || token->type != TOK_WORD || strcmp(token->value, "{"))
		return (NULL);
	while (parser_accept(p, TOK_NEWLINE))
		;
	child = parse_list(p);
	if (!child)
		return (NULL);
	while (parser_accept(p, TOK_NEWLINE))
		;
	token = parser_next(p);
	if (!token || token->type != TOK_WORD || strcmp(token->value, "}"))
	{
		if (!p->error)
			p->error = strdup("Syntax error : expected '}'");
		ast_free(child);
		return (NULL);
	}
	return (parse_group(p, child, NODE_BLOCK));
}
