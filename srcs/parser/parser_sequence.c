/**
 * @file parser_sequence.c
 * @brief file for parse sequence
 * @author jguillem
 */
#include "parser.h"

/**
 * @param p struc s_parser pointer
 * @param operator enum e_node_type pointer
 * @brief helper function for parse_sequence
 * @return 0 (no separator), 1 (; separator), 2 (&separator)
 */
static int	detect_separator(t_parser *p, t_node_type *operator)
{
	if (parser_accept(p, TOK_SEMICOLON))
	{
		*operator = NODE_SEQUENCE;
		return (1);
	}
	else if (parser_accept(p, TOK_AMPERSAND))
	{
		*operator = NODE_BACKGROUND;
		return (2);
	}
	return (0);
}

/**
 * @param p struct s_parser pointer
 * @brief handle sequence (& or ; separator)
 * @return struct s_ast pointer
 */
t_ast	*parse_sequence(t_parser *p)
{
	t_ast		*left;
	t_ast		*right;
	t_node_type	operator;
	int			op;
	char		*errors[2] = {"syntax error near ';'", "syntax error near '&'"};

	left = parse_and_or(p);
	if (!left)
		return (NULL);
	while ((op = detect_separator(p, &operator)))
	{
		if (operator == NODE_BACKGROUND)
		   left = ast_new_binary(operator, left, NULL);
		right = parse_and_or(p);
		if (!right)
		{
			p->error = strdup(errors[op - 1]);
			return (NULL);
		}	
		left = ast_new_binary(NODE_SEQUENCE, left, right);
	}
	return (left);
}
