/**
 * @file parser_list.c
 * @brief file for parse list
 * @author jguillem
 */
#include "parser.h"

/**
 * @param p struc s_parser pointer
 * @param operator enum e_node_type pointer
 * @brief helper function for parse_list
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
 * @brief handle list (& or ; separator)
 * @return struct s_ast pointer
 */
t_ast	*parse_list(t_parser *p)
{
	t_ast		*left;
	t_ast		*right;
	t_token		*current;
	t_node_type	operator;
	int			op;

	operator = -1;
	left = parse_and_or(p);
	if (!left)
		return (NULL);
	while ((op = detect_separator(p, &operator)))
	{
		current = parser_peek(p);
		if (!current || current->type == TOK_EOF
			|| (current->type == TOK_WORD && strcmp(current->value, "}") == 0))
			return (left);
		if (operator == NODE_BACKGROUND)
		{
		   left = ast_new_binary(operator, left, NULL);
		   if (current->type == TOK_EOF)
			   return (left);
		}
		right = parse_list(p);
		left = ast_new_binary(NODE_SEQUENCE, left, right);
	}
	return (left);
}
