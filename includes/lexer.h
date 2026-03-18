/**
 * @file lexer.h
 * @brief Header of the lexer module
 * @author jguillem
 */

#ifndef LEXER_H
# define LEXER_H

# include "libft.h"

/**
 * @brief Token types for the shell's lexer.
 * @details These types represent the different kinds of tokens that can be
 *          identified in the shell's input. They include words, operators,
 *          and special tokens like EOF and errors.
 */
typedef enum e_token_type
{
	TOK_WORD, /**< A word token ex: "ls" or "echo" */
	TOK_PIPE, /**< A pipe token ex: "|" */
	TOK_AND, /**< A logical AND token ex: "&&" */
	TOK_OR, /**< A logical OR token ex: "||" */
	TOK_SEMICOLON, /**< A semicolon token ex: ";" */
	TOK_AMPERSAND, /**< An ampersand token ex: "&" */
	TOK_NEWLINE, /**< A newline token (end of command) */
	TOK_REDIR_IN, /**< A redirect input token ex: "<" */
	TOK_REDIR_OUT, /**< A redirect output token ex: ">" */
	TOK_REDIR_APPEND, /**< A redirect append token ex: ">>" */
	TOK_HEREDOC, /**< A heredoc token ex: "<<" */
	TOK_REDIR_DUP_IN, /**< A duplicate redirect input token ex: "<&" */
	TOK_REDIR_DUP_OUT, /**< A duplicate redirect output token ex: ">&" */
	TOK_LPAREN, /**< A left parenthesis token ex: "(" */
	TOK_RPAREN, /**< A right parenthesis token ex: ")" */
	TOK_EOF, /**< An end-of-file token */
	TOK_ERROR /**< An error token (invalid syntax) */
}	t_token_type;

/**
 * @brief Struct representing a shell token.
 * @details This struct holds the type of the token, its raw value
 *  (with quotes preserved for later expansion), and an optional io_number for redirection tokens.
 */
typedef struct s_operator
{
	const char		*literal; /**< The literal string of the operator (e.g., "|", "&&") */
	t_token_type	type; /**< The token type corresponding to this operator */
}	t_operator;

/**
 * @brief Token data node.
 * @details Stored in a t_list* returned by lexer_tokenize (each node->content is a
 *  t_token*). No *next field — traversal is via the t_list wrapper.
 */
typedef struct s_token
{
	t_token_type	type; /**< Type of the token (word, operator, etc.) */
	char			*value; /**< Raw token string (quotes preserved for expander) */
	int				io_number; /**< File descriptor number for redirection tokens (-1 if none) */
}	t_token;

/*
 * Lexer interface
 *
 * lexer_tokenize: tokenize input string.
 *   Returns a t_list* (nodes contain t_token* via node->content).
 *   Returns NULL on error (error already printed to stderr).
 *   Caller must free with lexer_free_tokens().
 *
 * lexer_check_quotes: check for unclosed quotes.
 *   Sets *unclosed_quote to the quote char ('\'', '"') or 0 if balanced.
 *   Returns 1 if open quote found, 0 if balanced.
 *
 * lexer_free_tokens: free entire token list (tokens + strings + nodes).
 * Convenience accessor: get t_token* from a t_list node.
 */
t_list				*lexer_tokenize(const char *input);
int					lexer_check_quotes(const char *input, char *unclosed_quote);
void				lexer_free_tokens(t_list *tokens);

#ifdef FT_EXTRA_VERBOSE
void				lexer_display(t_list *tokens, const char *input);
char				*lexer_to_json(t_list *tokens, const char *input);
#endif

/**
 * @brief Token helpers (used by lexer internally and by tests)
 * @details These functions are used to create and free tokens, check for operators,
 *          and read operators and words from the input string. They are also used by the unit tests to verify the correctness of the lexer implementation.
 * 
 * @param type The type of the token to create.
 * @param value The raw string value of the token (with quotes preserved).
 * @param io_number The file descriptor number for redirection tokens (set to -1 if not applicable).
 * @return A new t_list node containing the created token, or NULL on allocation failure.
 */
t_list				*token_new(t_token_type type, char *value, int io_number);
void				token_free(t_token *token);

int					is_operator(char c);
int					is_operator_start(const char *line);
t_list				*read_operator(const char **line);
t_list				*read_word(const char **line);

/**
 * @brief Convenience accessor: get t_token* from a t_list node.
 */
# define TOK(node)	((t_token *)(node)->content)

#endif
