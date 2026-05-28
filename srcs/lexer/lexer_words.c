/**
 * @file lexer_words.c
 * @brief file to manage words of the prompt command
 * @author jguillem
 */

#include <ctype.h>
#include <string.h>
#include "lexer.h"

/*
 * @param flag Pointer to int to toggle
 * @brief toggle the logic value of an int
 * @details zero is toggle to 1, non-zero is toggle to 0
 * @return 0 | 1
 */
static void	toggle(int *flag)
{
	if (*flag != 0)
		*flag = 0;
	else
		*flag = 1;
}

/*
 * @brief Detect the start of an unquoted command substitution.
 * @details Returns 1 only when @p scout points at `$(` AND the next byte
 *          is NOT another `(` (which would be arithmetic `$((`). Single
 *          quotes inhibit command substitution; the caller must pass
 *          @p in_squote for that check.
 */
static int	is_cmdsub_start(const char *scout, int in_squote)
{
	return (!in_squote && scout[0] == '$' && scout[1] == '('
		&& scout[2] != '(');
}

/*
 * @param line The string of the word
 * @brief Tokenize a word
 * @details Delimit the word and tokenize it, manage the \ escape
 *          character, single/double quotes, and `$(...)` command
 *          substitution. Inside a `$(...)`, whitespace, pipes, semicolons
 *          and other operators are NOT word-separators -- they belong to
 *          the inner script. Paren balance is tracked so that nested
 *          `$( ... $(...) ...)` is collected as one TOK_WORD; the
 *          expander runs the substitution at expansion time.
 * @return A t_list *node with a TOK_WORD token as content
 */
t_list	*read_word(const char **line)
{
	t_list		*tok;
	const char	*scout;
	int			in_squote;
	int			in_dquote;
	int			sub_depth;

	in_squote = 0;
	in_dquote = 0;
	sub_depth = 0;
	scout = *line;
	while (*scout
		&& (in_squote || in_dquote || sub_depth > 0
			|| (!isspace(*scout) && !is_operator_start(scout))))
	{
		if (sub_depth == 0 && is_cmdsub_start(scout, in_squote))
		{
			sub_depth = 1;
			scout += 2;
			continue ;
		}
		if (*scout == '\'' && !in_dquote)
			toggle(&in_squote);
		else if (*scout == '"' && !in_squote)
			toggle(&in_dquote);
		else if (*scout == '\\' && !in_squote)
		{
			if (*(scout + 1))
				scout++;
		}
		else if (sub_depth > 0 && !in_squote && !in_dquote)
		{
			if (*scout == '(')
				sub_depth++;
			else if (*scout == ')')
				sub_depth--;
		}
		scout++;
	}
	tok = token_new(TOK_WORD, strndup(*line, scout - *line), -1);
	(*line) = scout;
	return (tok);
}
