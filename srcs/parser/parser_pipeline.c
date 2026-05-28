/**
 * @file parser_pipeline.c
 * @brief file for pipeline parser
 * @author jguillem
 */

#include "parser.h"

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
			ast_free(left);
			if (!p->error)
				p->error = strdup("syntax error near '|'");
			return (NULL);
		}
		left = ast_new_binary(NODE_PIPE, left, right);
	}
	return (left);
}
