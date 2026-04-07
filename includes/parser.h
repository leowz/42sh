/**
 * @file parser.h
 * @brief Parser definitions for the 42sh shell
 * @author jguillem pulgamecanica
 */

#ifndef PARSER_H
# define PARSER_H

# include "42sh.h"
# include "lexer.h"
# include "ast.h"

/**
 * @brief Parser internal state.
 *
 * @param tokens:  the t_list* of t_token* produced by the lexer.
 * @param current: the list node the parser is currently examining.
 * @param error:   human-readable error string (set on syntax error, free'd by parser).
 */
typedef struct s_parser
{
	t_list	*tokens;
	t_list	*current;
	char	*error;
}	t_parser;

/**
 * @brief consume a t_list* of tokens and return the AST root.
 *
 * @details Returns NULL on syntax error (error printed to stderr).
 * @details The token list is NOT freed here, caller frees it with lexer_free_tokens().
 */
t_ast	*parser_parse(t_list *tokens, t_shell *shell);

/**
 * @brief walk the AST after parsing and read heredoc content
 *
 * @details from stdin for each << redirection.
 * @details shell is needed to read from the correct fd and to check SIGINT.
 * @details Returns 0 on success, -1 if SIGINT aborted heredoc input.
 */
int		parser_collect_heredocs(t_ast *ast, t_shell *shell);

void	heredoc_expand_config(t_redir *redir);
int		parser_accept(t_parser *p, t_token_type type);
int		is_redir(t_token_type type);
t_token	*parser_peek(t_parser *p);
t_token	*parser_next(t_parser *p);
t_ast	*parse_subshell(t_parser *p);
t_ast	*parse_block(t_parser *p);
t_ast	*parse_command(t_parser *p);
t_ast	*parse_simple_command(t_parser *p);
t_ast	*parse_pipeline(t_parser *p);
t_ast	*parse_and_or(t_parser *p);
t_ast	*parse_list(t_parser *p);

#endif
