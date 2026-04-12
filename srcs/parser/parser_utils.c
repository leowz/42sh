/**
 * @file parser.c
 * @brief file for parser utils functions 
 * @author jguillem
 */
#include "parser.h"

/**
 * @param type enum e_node_type
 * @param left struct s_ast pointer
 * @param right struct s_ast pointer
 * @brief create and allocate new binary ast node
 * @return struct s_ast pointer
 */
t_ast	*ast_new_binary(t_node_type type, t_ast *left, t_ast *right)
{
	t_ast		*ast;
	t_binary	*binary;

	ast = malloc(sizeof(t_ast));
	if (!ast)
		return (NULL);
	binary = malloc(sizeof(t_binary));
	if (!binary)
	{
		free(ast);
		return (NULL);
	}
	binary->left = left;
	binary->right = right;
	ast->type = type;
	ast->data.binary = binary;
	return (ast);
}

/**
 * @param type enum e_token_type
 * @brief check is the the token is a redirect one
 * @return 1 | 0
 */
int	is_redir(t_token_type type)
{
	return (type == TOK_REDIR_IN
		|| type == TOK_REDIR_OUT
		|| type == TOK_REDIR_APPEND
		|| type == TOK_REDIR_DUP_IN
		|| type == TOK_REDIR_DUP_OUT
		|| type == TOK_HEREDOC_STRIP
		|| type == TOK_HEREDOC);
}

/**
 * @param p struct s_parser pointer
 * @brief recuperate current token without consume it
 * @return struct s_token (current token)
 */
t_token	*parser_peek(t_parser *p)
{
	if (!p || !p->current)
		return (NULL);
	return ((t_token *)p->current->content);
}

/**
 * @param p struct s_parser pointer
 * @brief recuperate current token and consume it
 * @return struct s_token (current token)
 */
t_token	*parser_next(t_parser *p)
{
	t_token	*token;

	if (!p || !p->current)
		return (NULL);
	token = (t_token *)p->current->content;
	p->current = p->current->next;
	return (token);
}

/**
 * @param p struct s_parser pointer
 * @param type enum e_token_type
 * @brief consume token when type correspondance
 * @return 1 | 0
 */
int	parser_accept(t_parser *p, t_token_type type)
{
	t_token	*token;

	token = parser_peek(p);
	if (!token)
		return (0);
	if (token->type == type)
	{
		parser_next(p);
		return (1);
	}
	return (0);
}
