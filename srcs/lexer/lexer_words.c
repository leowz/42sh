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
 * @param line The string of the word
 * @brief Tokenize a worf
 * @details Delimit the word and tokenize it, manage the \ escape character
 * @return A t_list *node with a TOK_WORD token as content
 */
t_list	*read_word(const char **line)
{
	t_list		*tok;
	const char	*scout;
	int			in_squote;
	int			in_dquote;

	in_squote = 0;
	in_dquote = 0;
	scout = *line;
	while (*scout
		&& (in_squote || in_dquote
			|| (!isspace(*scout) && !is_operator_start(scout))))
	{
		if (*scout == '\'' && !in_dquote)
			toggle(&in_squote);
		else if (*scout == '"' && !in_squote)
			toggle(&in_dquote);
		else if (*scout == '\\' && !in_squote)
			if (*(scout + 1))
				scout++;
		scout++;
	}
	tok = token_new(TOK_WORD, strndup(*line, scout - *line), -1);
	(*line) = scout;
	return (tok);
}
