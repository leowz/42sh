/**
 * @file parser_pipeline.c
 * @brief file for pipeline parser
 * @author jguillem
 */

#include "parser.h"

/**
 * @param p struct s_parser
 * @brief detect the pipe and create a t_ast node with two commands
 * @return a pointer on a struct s_ast
 */
t_ast	*parse_pipeline(t_parser *p)
{
	t_ast	*left;
	t_ast	*right;

	left = parse_command(p);
	if (!left)
		return (NULL);
	while (parser_accept(p, TOK_PIPE))
	{
		right = parse_command(p);
		if (!right)
		{
			p->error = strdup("syntax error near '|'");
			return (NULL);
		}
		left = ast_new_binary(NODE_PIPE, left, right);
	}
	return (left);
}
