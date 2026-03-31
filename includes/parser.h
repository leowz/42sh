/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: wengzhang <marvin@42.fr>                   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/22 21:00:00 by wengzhang         #+#    #+#             */
/*   Updated: 2026/03/30 21:12:27 by jguillem         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PARSER_H
# define PARSER_H

# include "42sh.h"
# include "lexer.h"
# include "ast.h"

/*
 * Parser internal state.
 *
 * tokens:  the t_list* of t_token* produced by the lexer.
 * current: the list node the parser is currently examining.
 * error:   human-readable error string (set on syntax error, free'd by parser).
 */
typedef struct s_parser
{
	t_list	*tokens;
	t_list	*current;
	char	*error;
}	t_parser;

/**
 * Parser interface
 *
 * parser_parse: consume a t_list* of tokens and return the AST root.
 *   Returns NULL on syntax error (error printed to stderr).
 *   The token list is NOT freed here — caller frees it with lexer_free_tokens().
 *
 * parser_collect_heredocs: walk the AST after parsing and read heredoc content
 *   from stdin for each << redirection.
 *   shell is needed to read from the correct fd and to check SIGINT.
 *   Returns 0 on success, -1 if SIGINT aborted heredoc input.
 */
t_ast	*parser_parse(t_list *tokens, t_shell *shell);
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
