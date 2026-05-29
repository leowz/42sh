/**
 * @file test_alias.c
 * @brief Tests for the shell alias storage module.
 *
 * Covers alias_set / alias_get / alias_get_value / alias_unset /
 * alias_clear: insert, lookup, overwrite-in-place, removal, missing-name
 * handling, multi-entry tables and NULL-safety.
 * @author zweng
 */

#ifdef TEST_ALIAS_ENABLED

# include "minunit.h"
# include "../includes/42sh.h"
# include "../includes/aliases.h"
# include "../includes/lexer.h"
# include <stdlib.h>
# include <string.h>

static void	alias_test_init(t_shell *shell)
{
	memset(shell, 0, sizeof(*shell));
}

/**
 * @brief Join every non-EOF token's raw value with single spaces.
 * @return Heap string the caller must free.
 */
static char	*join_tokens(t_list *tok)
{
	char	*out;
	char	*sep;
	char	*joined;
	int		first;

	out = ft_strdup("");
	first = 1;
	while (tok && TOK(tok)->type != TOK_EOF)
	{
		sep = ft_strjoin(out, first ? "" : " ");
		free(out);
		joined = ft_strjoin(sep, TOK(tok)->value);
		free(sep);
		out = joined;
		first = 0;
		tok = tok->next;
	}
	return (out);
}

/**
 * @brief Tokenize @p line, alias-expand it, return the joined result.
 */
static char	*expand_line(t_shell *shell, const char *line)
{
	t_list	*tokens;
	char	*result;

	tokens = lexer_tokenize(line);
	alias_expand_tokens(shell, &tokens);
	result = join_tokens(tokens);
	lexer_free_tokens(tokens);
	return (result);
}

static void	test_alias_set_and_get(void)
{
	t_shell	shell = {0};

	alias_test_init(&shell);
	MU_ASSERT_INT(0, alias_set(&shell, "ll", "ls -la"));
	MU_ASSERT_STR("alias value retrieved", "ls -la",
		alias_get_value(&shell, "ll"));
	MU_ASSERT("alias node found", alias_get(&shell, "ll") != NULL);
	alias_clear(&shell);
}

static void	test_alias_get_missing(void)
{
	t_shell	shell = {0};

	alias_test_init(&shell);
	MU_ASSERT("missing alias value is NULL",
		alias_get_value(&shell, "nope") == NULL);
	MU_ASSERT("missing alias node is NULL",
		alias_get(&shell, "nope") == NULL);
	alias_clear(&shell);
}

static void	test_alias_update_overwrites(void)
{
	t_shell	shell = {0};

	alias_test_init(&shell);
	alias_set(&shell, "g", "git");
	alias_set(&shell, "g", "git status");
	MU_ASSERT_STR("alias redefined in place", "git status",
		alias_get_value(&shell, "g"));
	MU_ASSERT("redefine creates no duplicate node",
		shell.aliases && shell.aliases->next == NULL);
	alias_clear(&shell);
}

static void	test_alias_unset(void)
{
	t_shell	shell = {0};

	alias_test_init(&shell);
	alias_set(&shell, "ll", "ls -la");
	MU_ASSERT_INT(0, alias_unset(&shell, "ll"));
	MU_ASSERT("alias gone after unset",
		alias_get_value(&shell, "ll") == NULL);
	alias_clear(&shell);
}

static void	test_alias_unset_missing(void)
{
	t_shell	shell = {0};

	alias_test_init(&shell);
	MU_ASSERT_INT(1, alias_unset(&shell, "ghost"));
	alias_clear(&shell);
}

static void	test_alias_unset_middle(void)
{
	t_shell	shell = {0};

	alias_test_init(&shell);
	alias_set(&shell, "a", "1");
	alias_set(&shell, "b", "2");
	alias_set(&shell, "c", "3");
	MU_ASSERT_INT(0, alias_unset(&shell, "b"));
	MU_ASSERT("b removed", alias_get_value(&shell, "b") == NULL);
	MU_ASSERT_STR("a survives", "1", alias_get_value(&shell, "a"));
	MU_ASSERT_STR("c survives", "3", alias_get_value(&shell, "c"));
	alias_clear(&shell);
}

static void	test_alias_clear(void)
{
	t_shell	shell = {0};

	alias_test_init(&shell);
	alias_set(&shell, "a", "1");
	alias_set(&shell, "b", "2");
	alias_clear(&shell);
	MU_ASSERT("clear empties the table", shell.aliases == NULL);
	MU_ASSERT("lookup after clear is NULL",
		alias_get_value(&shell, "a") == NULL);
}

static void	test_alias_null_safety(void)
{
	MU_ASSERT("alias_get on NULL shell", alias_get(NULL, "x") == NULL);
	MU_ASSERT_INT(1, alias_set(NULL, "x", "y"));
	MU_ASSERT_INT(1, alias_unset(NULL, "x"));
	alias_clear(NULL);
}

static void	test_expand_basic(void)
{
	t_shell	shell = {0};
	char	*r;

	alias_test_init(&shell);
	alias_set(&shell, "ll", "ls -la");
	r = expand_line(&shell, "ll");
	MU_ASSERT_STR("alias expands in command position", "ls -la", r);
	free(r);
	alias_clear(&shell);
}

static void	test_expand_keeps_args(void)
{
	t_shell	shell = {0};
	char	*r;

	alias_test_init(&shell);
	alias_set(&shell, "ll", "ls -la");
	r = expand_line(&shell, "ll foo bar");
	MU_ASSERT_STR("args after the alias survive", "ls -la foo bar", r);
	free(r);
	alias_clear(&shell);
}

static void	test_expand_not_in_argument(void)
{
	t_shell	shell = {0};
	char	*r;

	alias_test_init(&shell);
	alias_set(&shell, "ll", "ls -la");
	r = expand_line(&shell, "echo ll");
	MU_ASSERT_STR("alias not expanded as an argument", "echo ll", r);
	free(r);
	alias_clear(&shell);
}

static void	test_expand_after_operators(void)
{
	t_shell	shell = {0};
	char	*r;

	alias_test_init(&shell);
	alias_set(&shell, "g", "grep");
	r = expand_line(&shell, "echo x ; g y");
	MU_ASSERT_STR("expands after ;", "echo x ; grep y", r);
	free(r);
	r = expand_line(&shell, "cat | g z");
	MU_ASSERT_STR("expands after |", "cat | grep z", r);
	free(r);
	alias_clear(&shell);
}

static void	test_expand_chain(void)
{
	t_shell	shell = {0};
	char	*r;

	alias_test_init(&shell);
	alias_set(&shell, "a", "b");
	alias_set(&shell, "b", "echo");
	r = expand_line(&shell, "a hi");
	MU_ASSERT_STR("alias chain a->b->echo resolves", "echo hi", r);
	free(r);
	alias_clear(&shell);
}

static void	test_expand_recursion_guard(void)
{
	t_shell	shell = {0};
	char	*r;

	alias_test_init(&shell);
	alias_set(&shell, "ls", "ls -la");
	r = expand_line(&shell, "ls");
	MU_ASSERT_STR("self-referential alias expands exactly once",
		"ls -la", r);
	free(r);
	alias_clear(&shell);
}

static void	test_expand_quoted_not_expanded(void)
{
	t_shell	shell = {0};
	char	*r;

	alias_test_init(&shell);
	alias_set(&shell, "ls", "BAD");
	r = expand_line(&shell, "\"ls\"");
	MU_ASSERT_STR("quoted command word is not alias-expanded",
		"\"ls\"", r);
	free(r);
	alias_clear(&shell);
}

static void	test_expand_no_alias(void)
{
	t_shell	shell = {0};
	char	*r;

	alias_test_init(&shell);
	r = expand_line(&shell, "echo hello world");
	MU_ASSERT_STR("line with no alias is unchanged",
		"echo hello world", r);
	free(r);
	alias_clear(&shell);
}

void	test_alias_suite(void)
{
	test_alias_set_and_get();
	test_alias_get_missing();
	test_alias_update_overwrites();
	test_alias_unset();
	test_alias_unset_missing();
	test_alias_unset_middle();
	test_alias_clear();
	test_alias_null_safety();
	test_expand_basic();
	test_expand_keeps_args();
	test_expand_not_in_argument();
	test_expand_after_operators();
	test_expand_chain();
	test_expand_recursion_guard();
	test_expand_quoted_not_expanded();
	test_expand_no_alias();
}

#else

void	test_alias_suite(void)
{
}

#endif
