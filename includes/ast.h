/**
 * @file ast.h
 * @brief Abstract Syntax Tree definitions for the 42sh shell
 * @author jguillem pulgamecanica
 */

#ifndef AST_H
# define AST_H

# include "lexer.h"

/**
 * @brief AST node types
 *
 * @note Important to don't change the order
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

/**
 * @brief Redirection data node.
 * @details Stored in t_cmd.redirs and t_group.redirs as t_list*
 *          (each node->content is a t_redir*).  No *next field.
 *
 * @param fd				source fd (-1 = default for the operator type).
 * @param target			raw filename or fd number string (unexpanded).
 * @param heredoc_delim		the delimiter word for << (raw, may be quoted).
 * @param heredoc_stripped	1 if leadling tab are stripped
 * @param heredoc_content	collected heredoc content (filled after parse, before exec).
 * @param heredoc_quoted	1 if delimiter was quoted (no expansion inside heredoc).
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

/**
 * @brief Simple command data.
 *
 * @param argv        NULL-terminated array of raw/unexpanded argument strings.
 * @param argc        number of elements in argv (not counting the NULL terminator).
 * @param assignments t_list* of char* "NAME=value" strings (detected by parser,
 *                    unexpanded). Each node->content is a char*.
 * @param redirs      t_list* of t_redir* (in source order, applied left-to-right).
 *                    Each node->content is a t_redir*.
 */
typedef struct s_cmd
{
	char	**argv;
	int		argc;
	t_list	*assignments;
	t_list	*redirs;
}	t_cmd;

/**
 * @brief Binary operation data (pipe, &&, ||, ;)
 */
typedef struct s_binary
{
	struct s_ast	*left;
	struct s_ast	*right;
}	t_binary;

/**
 * @brief Group data (subshell, block, background).

 * @param redirs t_list* of t_redir* for redirections applied to the whole group,
 *               e.g. (cmd) > file or { cmd; } 2>&1.
 */
typedef struct s_group
{
	struct s_ast	*child;
	t_list			*redirs;
}	t_group;

/**
 * @brief AST node - union-based for zero-overhead dispatch
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

/**
 * @brief AST construction helpers
 */
t_ast		*ast_new_command(t_cmd *cmd);
t_ast		*ast_new_binary(t_node_type type, t_ast *left, t_ast *right);
t_ast		*ast_new_group(t_node_type type, t_ast *child, t_list *redirs);
void		ast_free(t_ast *node);

/**
 * @brief Redirection helpers
 */
t_redir		*redir_new(t_token_type type, int fd, char *target);
void		redir_free(t_redir *redir);

/**
 *  @brief Convenience accessor: get t_redir* from a t_list node.
 */
# define REDIR(node)	((t_redir *)(node)->content)

#ifdef FT_EXTRA_VERBOSE
void		ast_display(t_ast *ast, const char *input);
char		*ast_to_json(t_ast *ast, const char *input, const char *tok_file);
#endif

#endif
