/**
 * @file parser.c
 * @brief main file of parser module
 * @author jguillem, pulgamecanica
 */

#include <fcntl.h>
#include "parser.h"

/**
 * @details this is the main function of the parser module
 * @details call parse list which wrap all the layers and check for EOF
 * @details parse_list (";" and "&" separators) -> parse_and_or
 * @details parse_and_or ("&&" and "||" separators) -> parse_pipeline
 * @details parse_pipeline ("|" separator) -> parse_command
 * @details parse_command (read simple command) -> parse_subshell
 * @details parse_subshell recurse on parse_list
 * @details parse_heredoc walk ast and collect heredocs
 *
 */
t_ast	*parser_parse(t_list *tokens, t_shell *shell)
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
		fprintf(stderr, "%s\n", p.error);
		shell->last_exit_status = 2;
		free(p.error);
		ast_free(ast);
		return (NULL);
	}
	token = parser_peek(&p);
	if (!token || token->type != TOK_EOF)
	{
		fprintf(stderr, "syntax error: EOF not found\n");
		shell->last_exit_status = 2;
		free(p.error);
		ast_free(ast);
		return (NULL);
	}
	free(p.error);
	if (parser_collect_heredocs(ast, shell) == -1)
	{
		ast_free(ast);
		return (NULL);
	}
	return (ast);
}
