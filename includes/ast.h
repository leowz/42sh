/**
 * @file ast.h
 * @brief Abstract Syntax Tree
 * @author jguillem pulgamecanica
 */

#ifndef AST_H
#define AST_H

#include "lexer.h"

/**
 *  @brief Convenience accessor: get `t_redir *` from a `t_list` node.
 */
#define REDIR(node) ((t_redir *)(node)->content)

/**
 * ```sh
 * # Example
 * $> (which ls) | echo "dude" && which top
 * ```
 * ![AST visualizer](/assets/ast_1.png)
 * 
 * @warning Important to don't change the order
 */
typedef enum e_node_type {
  NODE_COMMAND,
  NODE_PIPE,
  NODE_AND,
  NODE_OR,
  NODE_SEQUENCE,
  NODE_SUBSHELL,
  NODE_BLOCK,
  NODE_BACKGROUND
} t_node_type;

/**
 * @brief Redirection data node.
 * @details Stored in t_cmd.redirs and t_group.redirs as t_list*
 *          (each node->content is a t_redir *).
 */
typedef struct s_redir {
  t_token_type type;          /**< Operator kind (TOK_REDIR_IN/OUT/APPEND/HEREDOC/...). */
  int          fd;            /**< Source fd (-1 = default for the operator type). */
  char         *target;       /**< Raw filename or fd-number string (unexpanded). */
  char         *heredoc_delim;/**< Delimiter word for << (raw, may be quoted). */
  int          heredoc_fd;    /**< Read end of pipe holding collected heredoc body. */
  int          heredoc_quoted;/**< 1 if delimiter was quoted (suppresses expansion). */
} t_redir;

/**
 * @brief Simple command data.
 * @details Stored in t_ast.data as a union with further access `t_ast->data.cmd`
 */
typedef struct s_cmd {
  char   **argv;       /**< NULL-terminated array of raw/unexpanded argument strings. */
  int    argc;         /**< Number of elements in argv (excluding NULL terminator). */
  t_list *assignments; /**< `t_list*` of char* "NAME=value" (unexpanded). */
  t_list *redirs;      /**< `t_list*` of `t_redir*` in source order (left-to-right). */
} t_cmd;

/**
 * @brief Binary operation data (pipe, &&, ||, ;).
 */
typedef struct s_binary {
  struct s_ast *left;  /**< Left-hand operand subtree. */
  struct s_ast *right; /**< Right-hand operand subtree. */
} t_binary;

/**
 * @brief Group data (subshell, block, background).
 */
typedef struct s_group {
  struct s_ast *child; /**< Subtree wrapped by the group. */
  t_list       *redirs;/**< `t_list*` of `t_redir*` applied to the whole group,
                            e.g. `(cmd) > file` or `{ cmd; } 2>&1`. */
} t_group;

/**
 * @brief AST node - union-based for zero-overhead dispatch.
 */
typedef struct s_ast {
  t_node_type type;       /**< Discriminator selecting which arm of `data` is live. */
  union {
    t_cmd    *cmd;        /**< `data.cmd` Live when `type == NODE_COMMAND`. */
    t_binary *binary;     /**< `data.binary` Live when `type == NODE_PIPE/AND/OR/SEQUENCE`. */
    t_group  *group;      /**< `data.group` Live when `type == NODE_SUBSHELL/BLOCK/BACKGROUND`. */
  } data;                 /**< Per-node payload selected by `type`. */
} t_ast;


char *ast_to_string(t_ast *ast);
char *cmd_to_string(t_cmd *cmd);
t_ast *ast_new_command(t_cmd *cmd);
t_ast *ast_new_binary(t_node_type type, t_ast *left, t_ast *right);
t_ast *ast_new_group(t_node_type type, t_ast *child, t_list *redirs);
void ast_free(t_ast *node);
t_redir *redir_new(t_token_type type, int fd, char *target);
void redir_free(t_redir *redir);
#ifdef FT_EXTRA_VERBOSE
void ast_display(t_ast *ast, const char *input);
char *ast_to_json(t_ast *ast, const char *input, const char *tok_file);
#endif

#endif
