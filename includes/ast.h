/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ast.h                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: wengzhang <marvin@42.fr>                   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/22 21:00:00 by wengzhang         #+#    #+#             */
/*   Updated: 2026/03/28 13:19:23 by jguillem         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef AST_H
# define AST_H

# include "lexer.h"

/*
* AST node types
* Important to don't change the order
*/
typedef enum e_node_type
{
	NODE_COMMAND,
	NODE_PIPE,
	NODE_AND,
	NODE_OR,
	NODE_SEQUENCE,
	NODE_SUBSHELL,
	NODE_BLOCK,
	NODE_BACKGROUND
}	t_node_type;

/*
** Redirection data node.
** Stored in t_cmd.redirs and t_group.redirs as t_list*
** (each node->content is a t_redir*).  No *next field.
**
** fd:               source fd (-1 = default for the operator type).
** target:           raw filename or fd number string (unexpanded).
** heredoc_delim:    the delimiter word for << (raw, may be quoted).
** heredoc_content:  collected heredoc content (filled after parse, before exec).
** heredoc_quoted:   1 if delimiter was quoted (no expansion inside heredoc).
*/
typedef struct s_redir
{
	t_token_type	type;
	int				fd;
	char			*target;
	char			*heredoc_delim;
	char			*heredoc_content;
	int				heredoc_quoted;
}	t_redir;

/*
** Simple command data.
**
** argv:        NULL-terminated array of raw/unexpanded argument strings.
** argc:        number of elements in argv (not counting the NULL terminator).
** assignments: t_list* of char* "NAME=value" strings (detected by parser,
**              unexpanded). Each node->content is a char*.
** redirs:      t_list* of t_redir* (in source order, applied left-to-right).
**              Each node->content is a t_redir*.
*/
typedef struct s_cmd
{
	char	**argv;
	int		argc;
	t_list	*assignments;
	t_list	*redirs;
}	t_cmd;

/*
** Binary operation data (pipe, &&, ||, ;)
*/
typedef struct s_binary
{
	struct s_ast	*left;
	struct s_ast	*right;
}	t_binary;

/*
** Group data (subshell, block, background).
**
** redirs: t_list* of t_redir* for redirections applied to the whole group,
**         e.g. (cmd) > file or { cmd; } 2>&1.
*/
typedef struct s_group
{
	struct s_ast	*child;
	t_list			*redirs;
}	t_group;

/*
** AST node — union-based for zero-overhead dispatch
*/
typedef struct s_ast
{
	t_node_type	type;
	union
	{
		t_cmd		*cmd;
		t_binary	*binary;
		t_group		*group;
	}	data;
}	t_ast;

/*
** AST construction helpers
*/
t_ast		*ast_new_command(t_cmd *cmd);
t_ast		*ast_new_binary(t_node_type type, t_ast *left, t_ast *right);
void		ast_free(t_ast *node);

/*
** Redirection helpers
*/
t_redir		*redir_new(t_token_type type, int fd, char *target);
void		redir_free(t_redir *redir);
void		ast_walk(t_ast *node);
void		heredoc_expand_config(t_redir *redir);

/*
** Convenience accessor: get t_redir* from a t_list node.
*/
# define REDIR(node)	((t_redir *)(node)->content)

#endif
