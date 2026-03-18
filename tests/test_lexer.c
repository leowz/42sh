/**
 * @file test_lexer.c
 * @brief Unit tests for the 42sh lexer module.
 *
 * Covers lexer_tokenize(), is_operator(), is_operator_start(), read_word(),
 * read_operator(), token_new(), and lexer_free_tokens().
 *
 * ## How to enable
 *
 * Add `-DTEST_LEXER_ENABLED` to `TEST_FLAGS` in the root Makefile and declare
 * `test_lexer_suite` in test_runner.c.  Once all assertions are permanently
 * green, remove the `#ifdef` guards and the `-D` flag per the workflow
 * described in test_runner.c.
 *
 * ## Sections
 *
 * - **token_new** — allocation and field initialisation
 * - **is_operator / is_operator_start** — character classification
 * - **read_word** — word boundary detection, quote handling, backslash escapes
 * - **read_operator** — every operator token type
 * - **lexer_tokenize** — end-to-end tokenisation of realistic shell inputs
 */

#ifdef TEST_LEXER_ENABLED
#endif /* TEST_LEXER_ENABLED */

# include "minunit.h"
# include "lexer.h"
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
# include <sys/wait.h>

/* ========================================================================= */
/*  Helpers                                                                   */
/* ========================================================================= */

/**
 * @brief Return the nth token (0-based) from a token list.
 *
 * @param head  Head of the list returned by lexer_tokenize().
 * @param n     Zero-based index of the desired node.
 * @return      Pointer to the t_token, or NULL if the list is shorter than n.
 */
static t_token	*nth_token(t_list *head, int n)
{
	while (head && n-- > 0)
		head = head->next;
	return (head ? TOK(head) : NULL);
}

static void	assert_token_type(t_list *tokens, int idx, t_token_type expected)
{
	t_token	*tok;

	tok = nth_token(tokens, idx);
	MU_ASSERT("token exists at expected index", tok != NULL);
	if (tok)
		MU_ASSERT_INT(expected, tok->type);
}

/* ========================================================================= */
/*  token_new                                                                 */
/* ========================================================================= */

/**
 * @brief token_new allocates a node and sets all three fields correctly.
 *
 * Verifies that the returned list node is non-NULL, that its content pointer
 * is non-NULL, and that type, value, and io_number match the arguments.
 */
static void	test_token_new_basic(void)
{
	char	*val;
	t_list	*node;
	t_token	*tok;

	val = strdup("hello");
	node = token_new(TOK_WORD, val, -1);
	MU_ASSERT("token_new returns non-NULL node", node != NULL);
	MU_ASSERT("token_new content is non-NULL", node->content != NULL);
	tok = TOK(node);
	MU_ASSERT_INT(TOK_WORD, tok->type);
	MU_ASSERT_STR("token value", "hello", tok->value);
	MU_ASSERT_INT(-1, tok->io_number);
	token_free(tok);
	free(node);
}

/**
 * @brief token_new stores a positive io_number without modification.
 */
static void	test_token_new_io_number(void)
{
	t_list	*node;
	t_token	*tok;

	node = token_new(TOK_REDIR_OUT, strdup(">"), 2);
	tok = TOK(node);
	MU_ASSERT_INT(2, tok->io_number);
	MU_ASSERT_INT(TOK_REDIR_OUT, tok->type);
	token_free(tok);
	free(node);
}

/* ========================================================================= */
/*  is_operator / is_operator_start                                           */
/* ========================================================================= */

/**
 * @brief is_operator returns non-zero for every recognised operator character.
 */
static void	test_is_operator_true(void)
{
	MU_ASSERT("'|' is operator", is_operator('|'));
	MU_ASSERT("'&' is operator", is_operator('&'));
	MU_ASSERT("'>' is operator", is_operator('>'));
	MU_ASSERT("'<' is operator", is_operator('<'));
	MU_ASSERT("';' is operator", is_operator(';'));
	MU_ASSERT("'(' is operator", is_operator('('));
	MU_ASSERT("')' is operator", is_operator(')'));
	MU_ASSERT("'\\n' is operator", is_operator('\n'));
}

/**
 * @brief is_operator returns zero for ordinary characters.
 */
static void	test_is_operator_false(void)
{
	MU_ASSERT("'a' is not operator", !is_operator('a'));
	MU_ASSERT("'0' is not operator", !is_operator('0'));
	MU_ASSERT("'\\0' is not operator", !is_operator('\0'));
	MU_ASSERT("' ' is not operator", !is_operator(' '));
}

/**
 * @brief is_operator_start detects a bare operator at the start of a string.
 */
static void	test_is_operator_start_simple(void)
{
	MU_ASSERT("|cmd is operator start", is_operator_start("|cmd"));
	MU_ASSERT("&& is operator start", is_operator_start("&&cmd"));
	MU_ASSERT(">> is operator start", is_operator_start(">>file"));
}

/**
 * @brief is_operator_start detects a digit-prefixed redirect (e.g. "2>").
 */
static void	test_is_operator_start_io_number(void)
{
	MU_ASSERT("2> is operator start", is_operator_start("2>file"));
	MU_ASSERT("1< is operator start", is_operator_start("1<file"));
	MU_ASSERT("12>> is operator start", is_operator_start("12>>file"));
}

/**
 * @brief is_operator_start returns zero when digits are not followed by a
 *        redirect character.
 */
static void	test_is_operator_start_digit_not_redir(void)
{
	MU_ASSERT("42 alone is not operator start", !is_operator_start("42sh"));
}

/**
 * @brief is_operator_start returns zero for plain words.
 */
static void	test_is_operator_start_word(void)
{
	MU_ASSERT("'echo' is not operator start", !is_operator_start("echo"));
}

/* ========================================================================= */
/*  read_word                                                                 */
/* ========================================================================= */

/**
 * @brief read_word consumes a simple unquoted word and stops at whitespace.
 */
static void	test_read_word_simple(void)
{
	const char	*p = "echo hello";
	t_list		*node;
	t_token		*tok;

	node = read_word(&p);
	tok = TOK(node);
	MU_ASSERT_INT(TOK_WORD, tok->type);
	MU_ASSERT_STR("simple word", "echo", tok->value);
	MU_ASSERT("cursor advanced past word", *p == ' ');
	lexer_free_tokens(node);
}

/**
 * @brief read_word includes the entire single-quoted span as one token.
 *
 * Spaces inside single quotes must not terminate the word.
 */
static void	test_read_word_single_quote(void)
{
	const char	*p = "'hello world' next";
	t_list		*node;
	t_token		*tok;

	node = read_word(&p);
	tok = TOK(node);
	MU_ASSERT_STR("single-quoted word", "'hello world'", tok->value);
	MU_ASSERT("cursor after closing quote", *p == ' ');
	lexer_free_tokens(node);
}

/**
 * @brief read_word includes the entire double-quoted span as one token.
 */
static void	test_read_word_double_quote(void)
{
	const char	*p = "\"hello world\" next";
	t_list		*node;
	t_token		*tok;

	node = read_word(&p);
	tok = TOK(node);
	MU_ASSERT_STR("double-quoted word", "\"hello world\"", tok->value);
	lexer_free_tokens(node);
}

/**
 * @brief read_word respects a backslash-escaped space outside quotes.
 *
 * "hello\ world" should be consumed as a single token.
 */
static void	test_read_word_backslash_escape(void)
{
	const char	*p = "hello\\ world end";
	t_list		*node;
	t_token		*tok;

	node = read_word(&p);
	tok = TOK(node);
	MU_ASSERT_STR("backslash-escaped space", "hello\\ world", tok->value);
	lexer_free_tokens(node);
}

/**
 * @brief read_word stops at an operator character.
 */
static void	test_read_word_stops_at_operator(void)
{
	const char	*p = "cmd|next";
	t_list		*node;
	t_token		*tok;

	node = read_word(&p);
	tok = TOK(node);
	MU_ASSERT_STR("word before pipe", "cmd", tok->value);
	MU_ASSERT("cursor is at pipe", *p == '|');
	lexer_free_tokens(node);
}

/**
 * @brief A pipe inside single quotes is NOT an operator boundary.
 */
static void	test_read_word_pipe_in_single_quote(void)
{
	const char	*p = "'cmd|next'";
	t_list		*node;
	t_token		*tok;

	node = read_word(&p);
	tok = TOK(node);
	MU_ASSERT_STR("pipe inside single quotes", "'cmd|next'", tok->value);
	lexer_free_tokens(node);
}

/* ========================================================================= */
/*  read_operator                                                             */
/* ========================================================================= */

/**
 * @brief Helper: tokenise a single operator string and return its type.
 */
static t_token_type	operator_type(const char *s)
{
	t_list		*node;
	t_token		*tok;
	t_token_type	type;

	node = read_operator(&s);
	tok = TOK(node);
	type = tok->type;
	lexer_free_tokens(node);
	return (type);
}

/*
 * Runs read_operator() in a child process to detect hard crashes on malformed
 * operator starts without taking down the whole test process.
*/
static int	read_operator_crashes(const char *s)
{
	pid_t	pid;
	int		status;
	t_list	*node;

	pid = fork();
	if (pid == 0)
	{
		node = read_operator(&s);
		if (node)
			lexer_free_tokens(node);
		exit(0);
	}
	if (pid < 0)
		return (1);
	if (waitpid(pid, &status, 0) < 0)
		return (1);
	if (WIFSIGNALED(status))
		return (1);
	if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
		return (0);
	return (1);
}

/**
 * @brief read_operator correctly classifies every single-character operator.
 */
static void	test_read_operator_single_char(void)
{
	MU_ASSERT_INT(TOK_PIPE, operator_type("|"));
	MU_ASSERT_INT(TOK_AMPERSAND, operator_type("&"));
	MU_ASSERT_INT(TOK_REDIR_IN, operator_type("<"));
	MU_ASSERT_INT(TOK_REDIR_OUT, operator_type(">"));
	MU_ASSERT_INT(TOK_SEMICOLON, operator_type(";"));
	MU_ASSERT_INT(TOK_LPAREN, operator_type("("));
	MU_ASSERT_INT(TOK_RPAREN, operator_type(")"));
	MU_ASSERT_INT(TOK_NEWLINE, operator_type("\n"));
}

/**
 * @brief read_operator correctly classifies every two-character operator.
 */
static void	test_read_operator_double_char(void)
{
	MU_ASSERT_INT(TOK_OR, operator_type("||"));
	MU_ASSERT_INT(TOK_AND, operator_type("&&"));
	MU_ASSERT_INT(TOK_REDIR_APPEND, operator_type(">>"));
	MU_ASSERT_INT(TOK_HEREDOC, operator_type("<<"));
	MU_ASSERT_INT(TOK_REDIR_DUP_IN, operator_type("<&"));
	MU_ASSERT_INT(TOK_REDIR_DUP_OUT, operator_type(">&"));
}

/**
 * @brief read_operator extracts an io_number from a digit-prefixed redirect.
 *
 * "2>" must produce TOK_REDIR_OUT with io_number == 2.
 */
static void	test_read_operator_io_number(void)
{
	const char	*p = "2>file";
	t_list		*node;
	t_token		*tok;

	node = read_operator(&p);
	tok = TOK(node);
	MU_ASSERT_INT(TOK_REDIR_OUT, tok->type);
	MU_ASSERT_INT(2, tok->io_number);
	MU_ASSERT("cursor advanced past operator", *p == 'f');
	lexer_free_tokens(node);
}

/**
 * @brief read_operator must not crash on malformed digit-prefixed non-operator
 *        input (e.g. "42x").
 */
static void	test_read_operator_malformed_no_crash(void)
{
	MU_ASSERT("read_operator does not crash on malformed input",
		!read_operator_crashes("42x"));
}

/* ========================================================================= */
/*  lexer_tokenize — end-to-end                                              */
/* ========================================================================= */

/**
 * @brief "echo hello" produces WORD WORD EOF (3 tokens).
 */
static void	test_tokenize_simple_command(void)
{
	t_list	*tokens;
	t_token	*tok;

	tokens = lexer_tokenize("echo hello");
	MU_ASSERT("tokens not NULL", tokens != NULL);
	tok = nth_token(tokens, 0);
	MU_ASSERT_INT(TOK_WORD, tok->type);
	MU_ASSERT_STR("first word", "echo", tok->value);
	tok = nth_token(tokens, 1);
	MU_ASSERT_INT(TOK_WORD, tok->type);
	MU_ASSERT_STR("second word", "hello", tok->value);
	tok = nth_token(tokens, 2);
	MU_ASSERT_INT(TOK_EOF, tok->type);
	lexer_free_tokens(tokens);
}

/**
 * @brief "echo | cat" produces WORD PIPE WORD EOF (4 tokens).
 */
static void	test_tokenize_pipe(void)
{
	t_list	*tokens;

	tokens = lexer_tokenize("echo | cat");
	MU_ASSERT_INT(TOK_WORD, nth_token(tokens, 0)->type);
	MU_ASSERT_INT(TOK_PIPE, nth_token(tokens, 1)->type);
	MU_ASSERT_INT(TOK_WORD, nth_token(tokens, 2)->type);
	MU_ASSERT_INT(TOK_EOF, nth_token(tokens, 3)->type);
	lexer_free_tokens(tokens);
}

/**
 * @brief "echo > file" produces WORD REDIR_OUT WORD EOF (4 tokens).
 */
static void	test_tokenize_redirect_out(void)
{
	t_list	*tokens;

	tokens = lexer_tokenize("echo > file");
	MU_ASSERT_INT(TOK_WORD, nth_token(tokens, 0)->type);
	MU_ASSERT_INT(TOK_REDIR_OUT, nth_token(tokens, 1)->type);
	MU_ASSERT_INT(TOK_WORD, nth_token(tokens, 2)->type);
	MU_ASSERT_INT(TOK_EOF, nth_token(tokens, 3)->type);
	lexer_free_tokens(tokens);
}

/**
 * @brief "cat << EOF" produces WORD HEREDOC WORD EOF (4 tokens).
 */
static void	test_tokenize_heredoc(void)
{
	t_list	*tokens;

	tokens = lexer_tokenize("cat << EOF");
	MU_ASSERT_INT(TOK_WORD, nth_token(tokens, 0)->type);
	MU_ASSERT_INT(TOK_HEREDOC, nth_token(tokens, 1)->type);
	MU_ASSERT_INT(TOK_WORD, nth_token(tokens, 2)->type);
	lexer_free_tokens(tokens);
}

/**
 * @brief An empty string produces only a single TOK_EOF token.
 */
static void	test_tokenize_empty(void)
{
	t_list	*tokens;

	tokens = lexer_tokenize("");
	MU_ASSERT("empty input returns a list", tokens != NULL);
	MU_ASSERT_INT(TOK_EOF, nth_token(tokens, 0)->type);
	lexer_free_tokens(tokens);
}

/**
 * @brief Leading and trailing spaces are ignored.
 *
 * "  echo  " should yield the same token stream as "echo".
 */
static void	test_tokenize_whitespace_trimmed(void)
{
	t_list	*tokens;

	tokens = lexer_tokenize("   echo   ");
	MU_ASSERT_INT(TOK_WORD, nth_token(tokens, 0)->type);
	MU_ASSERT_STR("trimmed word", "echo", nth_token(tokens, 0)->value);
	MU_ASSERT_INT(TOK_EOF, nth_token(tokens, 1)->type);
	lexer_free_tokens(tokens);
}

/**
 * @brief "cmd && other || fallback" produces the correct AND/OR tokens.
 */
static void	test_tokenize_and_or(void)
{
	t_list	*tokens;

	tokens = lexer_tokenize("cmd && other || fallback");
	MU_ASSERT_INT(TOK_WORD, nth_token(tokens, 0)->type);
	MU_ASSERT_INT(TOK_AND, nth_token(tokens, 1)->type);
	MU_ASSERT_INT(TOK_WORD, nth_token(tokens, 2)->type);
	MU_ASSERT_INT(TOK_OR, nth_token(tokens, 3)->type);
	MU_ASSERT_INT(TOK_WORD, nth_token(tokens, 4)->type);
	MU_ASSERT_INT(TOK_EOF, nth_token(tokens, 5)->type);
	lexer_free_tokens(tokens);
}

/**
 * @brief Quoted strings containing operators are treated as a single WORD.
 *
 * "echo '|; <>'" must not produce any operator tokens.
 */
static void	test_tokenize_quoted_operators(void)
{
	t_list	*tokens;
	t_token	*tok;

	tokens = lexer_tokenize("echo '|; <>'");
	tok = nth_token(tokens, 1);
	MU_ASSERT_INT(TOK_WORD, tok->type);
	MU_ASSERT_STR("quoted operators as word", "'|; <>'", tok->value);
	lexer_free_tokens(tokens);
}

/**
 * @brief A digit-prefixed redirect stores the correct io_number on the token.
 *
 * "cmd 2>>err" must produce io_number == 2 on the TOK_REDIR_APPEND token.
 */
static void	test_tokenize_io_number(void)
{
	t_list	*tokens;
	t_token	*tok;

	tokens = lexer_tokenize("cmd 2>>err");
	tok = nth_token(tokens, 1);
	MU_ASSERT_INT(TOK_REDIR_APPEND, tok->type);
	MU_ASSERT_INT(2, tok->io_number);
	lexer_free_tokens(tokens);
}

/**
 * @brief Subshell syntax "(echo hi)" produces LPAREN WORD WORD RPAREN EOF.
 */
static void	test_tokenize_subshell(void)
{
	t_list	*tokens;

	tokens = lexer_tokenize("(echo hi)");
	MU_ASSERT_INT(TOK_LPAREN, nth_token(tokens, 0)->type);
	MU_ASSERT_INT(TOK_WORD, nth_token(tokens, 1)->type);
	MU_ASSERT_INT(TOK_WORD, nth_token(tokens, 2)->type);
	MU_ASSERT_INT(TOK_RPAREN, nth_token(tokens, 3)->type);
	MU_ASSERT_INT(TOK_EOF, nth_token(tokens, 4)->type);
	lexer_free_tokens(tokens);
}

/**
 * @brief Newline is a shell command separator and must be tokenized.
 *
 * "echo\ncat" should produce WORD NEWLINE WORD EOF.
 */
static void	test_tokenize_newline_separator(void)
{
	t_list	*tokens;

	tokens = lexer_tokenize("echo\ncat");
	assert_token_type(tokens, 0, TOK_WORD);
	assert_token_type(tokens, 1, TOK_NEWLINE);
	assert_token_type(tokens, 2, TOK_WORD);
	assert_token_type(tokens, 3, TOK_EOF);
	lexer_free_tokens(tokens);
}

/* ========================================================================= */
/*  Suite entry point                                                         */
/* ========================================================================= */

/**
 * @brief Run all lexer unit tests.
 *
 * Called by test_runner.c via MU_RUN(test_lexer_suite).
 */
void	test_lexer_suite(void)
{
	/* token_new */
	test_token_new_basic();
	test_token_new_io_number();

	/* is_operator / is_operator_start */
	test_is_operator_true();
	test_is_operator_false();
	test_is_operator_start_simple();
	test_is_operator_start_io_number();
	test_is_operator_start_digit_not_redir();
	test_is_operator_start_word();

	/* read_word */
	test_read_word_simple();
	test_read_word_single_quote();
	test_read_word_double_quote();
	test_read_word_backslash_escape();
	test_read_word_stops_at_operator();
	test_read_word_pipe_in_single_quote();

	/* read_operator */
	test_read_operator_single_char();
	test_read_operator_double_char();
	test_read_operator_io_number();
	test_read_operator_malformed_no_crash();

	/* lexer_tokenize end-to-end */
	test_tokenize_simple_command();
	test_tokenize_pipe();
	test_tokenize_redirect_out();
	test_tokenize_heredoc();
	test_tokenize_empty();
	test_tokenize_whitespace_trimmed();
	test_tokenize_and_or();
	test_tokenize_quoted_operators();
	test_tokenize_io_number();
	test_tokenize_subshell();
	test_tokenize_newline_separator();
}

