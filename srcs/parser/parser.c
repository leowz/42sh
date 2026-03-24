/**
 * @file parser.c
 * @brief main file of parser module
 * @author jguillem
 */

#include <fcntl.h>
#include "parser.h"

/**
 * TODO :
 *	- implement free ast memory
 *	- check what is missing or doesn't fit with the plan
 *	- implement tests
 */

/**
 * @param tokens pointer on a struct s_list of tokens
 * @brief receive a tokens from lexer and build an ast binary tree
 * @details this is the main function of the parser module
 * @details call parse list which wrap all the layers and check for EOF
 * @details parse_list (";" and "&" separators) -> parse_and_or
 * @details parse_and_or ("&&" and "||" separators) -> parse_pipeline
 * @details parse_pipeline ("|" separator) -> parse_command
 * @details parse_command (read simple command) -> parse_subshell
 * @details parse_subshell recurse on parse_list
 * @details parse_heredoc walk ast and collect heredocs
 */
t_ast	*parser_parse(t_list *tokens)
{
	t_parser	p;
	t_ast		*ast;
	t_token		*token;

	p.tokens = tokens;
	p.current = tokens;
	p.error = NULL;
	ast = parse_list(&p);
	if (p.error)
	{
		ast_free(ast);
		return (NULL);
	}
	token = parser_peek(&p);
	if (!token || token->type != TOK_EOF)
	{
		ast_free(ast);
		p.error = strdup("Syntax error: EOF not found");
		return (NULL);
	}
	return (ast);
}
