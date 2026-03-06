/**
 * @file lexer_operator.c
 * @brief file to manage operators of the prompt command
 * @author jguillem
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

/**
 * @brief array of struct s_operator struct
 * @details match literal representation and token type
 * @details important to keep long operators before short ones (e.g. && and &)
 * @details this allow strncmp to works properly
 */
static const	t_operator operators[] = {
	{";",	TOK_SEMICOLON},
	{"\n",	TOK_NEWLINE},
	{"(",	TOK_LPAREN},
	{")",	TOK_RPAREN},
	{"||",	TOK_OR},
	{"|",	TOK_PIPE},
	{"&&",	TOK_AND},
	{"&",	TOK_AMPERSAND},
	{"<<",	TOK_HEREDOC},
	{"<&",	TOK_REDIR_DUP_IN},
	{"<",	TOK_REDIR_IN},
	{">>",	TOK_REDIR_APPEND},
	{">&",	TOK_REDIR_DUP_OUT},
	{">",	TOK_REDIR_OUT},
	{NULL,	0}
};

/**
 * @param c character to check
 * @brief check if a char is the beginning of an operator
 * @details check if the character is in "&|><;{}\n"
 * @return 0 | 1
 */
int	is_operator(char c)
{
	int	i;

	i = 0;
	while (operators[i].literal)
	{
		if (operators[i].literal[0] == c)
			return (1);
		i++;
	}
	return (0);
}

/**
 * @param line string of operator to test
 * @brief check if a string is an operator
 * @details manage the digits in case of redirection then call is_operator
 * @return 1 | 0
 */
int	is_operator_start(const char *line)
{
	if (isdigit(*line))
	{
		while (*line && isdigit(*line))
			line++;
		if (*line == '<' || *line == '>')
			return (1);
		return (0);
	}
	return (is_operator(*line));
}

/*
 * @param the string of the operator
 * @brief extract the file descriptor before the operator
 * @return an int
 */
static int	extract_io_number(const char **line)
{
	int	io_number;

	io_number = -1;
	if (isdigit(**line))
	{
		io_number = atoi(*line);
		while (isdigit(**line))
			(*line)++;
	}
	return (io_number);
}

/*
 * @param line Address of the string to tokenize
 * @param io_number file descriptor for redirection
 * @param literal representation of the operator
 * @param type A t_token_type
 * @brief Create a new t_list token node
 * @return A t_list token node
 */
static t_list	*create_operator_token(
		const char		**line,
		int				io_number,
		const char		*literal,
		t_token_type	type)
{
	t_list	*tok;

	tok = token_new(type, strdup(literal), io_number);
	(*line) += strlen(literal);
	return (tok);
}

/*
 * @param the string of the operator
 * @brief read the operator and tokenize it
 * @details choose the good function to tokenize
 * @return a t_list* node
 */
t_list	*read_operator(const char **line)
{
	int		i;
	int		io_number;
	size_t	len;

	io_number = extract_io_number(line);
	i = 0;
	while (operators[i].literal)
	{
		len = strlen(operators[i].literal);
		if (strncmp(*line, operators[i].literal, len) == 0)
			return (create_operator_token(
						line,
						io_number,
						operators[i].literal,
						operators[i].type));
		i++;
	}
	return (NULL);
}
