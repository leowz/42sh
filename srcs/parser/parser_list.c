/**
 * @file parser_list.c
 * @brief file for parse list
 * @author jguillem
 */
#include "parser.h"

/**
 * @param p struc s_parser pointer
 * @param operator enum e_node_type pointer
 * @brief helper function for parse_list. Newlines act as command terminators
 *        (POSIX 2.10) so `;` and a literal `\n` between commands are
 *        equivalent. This matters for multi-line `()` / `{}` groups and
 *        for any input that came through `shell_read_logical_line`, which
 *        joins physical lines with `\n` to preserve quote interiors and
 *        paren grouping.
 * @return 0 (no separator), 1 (; or \n separator), 2 (& separator)
 */
static int	detect_separator(t_parser *p, t_node_type *operator)
{
	int	ate;

	ate = 0;
	while (parser_accept(p, TOK_NEWLINE))
		ate = 1;
	if (parser_accept(p, TOK_SEMICOLON) || ate)
	{
		while (parser_accept(p, TOK_NEWLINE))
			;
		*operator = NODE_SEQUENCE;
		return (1);
	}
	else if (parser_accept(p, TOK_AMPERSAND))
	{
		while (parser_accept(p, TOK_NEWLINE))
			;
		*operator = NODE_BACKGROUND;
		return (2);
	}
	return (0);
}

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
		if (operator == NODE_BACKGROUND)
			left = ast_new_group(NODE_BACKGROUND, left, NULL);
		current = parser_peek(p);
		if (!current || current->type == TOK_EOF
			|| current->type == TOK_RPAREN
			|| (current->type == TOK_WORD && strcmp(current->value, "}") == 0))
			return (left);
		right = parse_list(p);
		left = ast_new_binary(NODE_SEQUENCE, left, right);
	}
	return (left);
}
