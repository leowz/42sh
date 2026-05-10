/**
 * @file parser_and_or.c
 * @brief file for && and || parser
 * @author jguillem
 */
#include "parser.h"

/**
 * @param p struct s_parser
 * @param type enum e_node_type
 * @brief helper function to detect TOK_AND and TOK_OR
 * @return	0 (no one), 1 (TOK_AND), 2 (TOK_OR)
 */
static int	detect_and_or(t_parser *p, t_node_type *type)
{
	if (parser_accept(p, TOK_AND))
	{
		*type = NODE_AND;
		return (1);
	}
	if (parser_accept(p, TOK_OR))
	{
		*type = NODE_OR;
		return (2);
	}
	return (0);
}

t_ast	*parse_and_or(t_parser *p)
{
	t_ast		*left;
	t_ast		*right;
	t_node_type	operator;
	int			op;
	char		*errors[2] = {"syntax error near '&&'", "syntax error near '||'"};

	left = parse_pipeline(p);
	if (!left)
		return (NULL);
	while ((op = detect_and_or(p, &operator)))
	{
		right = parse_pipeline(p);
		if (!right)
		{
			p->error = strdup(errors[op - 1]);
			return (NULL);
		}
		left = ast_new_binary(operator, left, right);
	}
	return (left);
}
