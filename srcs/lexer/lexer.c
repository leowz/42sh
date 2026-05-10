/**
 * @file lexer.c
 * @brief main file of lexer module
 * @author jguillem
 */

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "lexer.h"

/**
 * @details skip spaces and recognize the type of token before adding it in the list. The memory is allocated and the list must be freed.
 */
t_list	*lexer_tokenize(const char *input)
{
	t_list	*head;
	t_list	*token;
	char	*eof;

	head = NULL;
	token = NULL;
	while (*input)
	{
		while (*input != '\n' && isspace(*input))
			input++;
		if (!*input)
			break ;
		if (is_operator_start(input))
			token = read_operator(&input);
		else
			token = read_word(&input);
		ft_lstappend(&head, token);
	}
	eof = strdup("");
	ft_lstappend(&head, token_new(TOK_EOF, eof, -1));
	return (head);
}
