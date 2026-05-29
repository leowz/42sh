/**
 * @file test_expander.c
 * @brief Comprehensive test suite for the expander module.
 * @author pulgamecanica
 *
 * Tests cover:
 *   - t_xbuf primitives (init/putc/puts/free, mask tracking).
 *   - expand_word: quote handling, backslash escapes, $VAR / ${VAR} /
 *     $? / $$ / $0 substitution, tilde expansion.
 *   - field_split: default IFS, custom IFS, mixed ws + non-ws,
 *     adjacent delimiters, mask suppression of literal IFS bytes,
 *     IFS = "" disables splitting.
 *   - expand_word_to_fields: end-to-end happy path.
 *   - expand_command: argv field-splitting, assignment value
 *     expansion, redirection target expansion, heredoc skip.
 *
 * Uses the shared stub_shell_init / stub_shell_cleanup helpers from
 * test_stubs.c; the variables list is poked directly to avoid
 * depending on var_set (which still lives behind a stub on this
 * branch).
 */

#ifdef TEST_EXPANDER_ENABLED

# include "minunit.h"
# include "../includes/42sh.h"
# include "../includes/expander.h"
# include "../includes/ast.h"
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
# include <pwd.h>

extern void	stub_shell_init(t_shell *shell);
extern void	stub_shell_cleanup(t_shell *shell);

/**
 * @brief Build a heap t_redir for tests (redir_new isn't implemented yet).
 * @details Mirrors the layout used by parser_heredoc.c when constructing
 *          a redirection node so cmd_free / redir_free can release it.
 */
static t_redir	*make_redir(t_token_type type, int fd, const char *target)
{
	t_redir	*r;

	r = malloc(sizeof(t_redir));
	r->type = type;
	r->fd = fd;
	r->target = target ? ft_strdup(target) : NULL;
	r->heredoc_delim = NULL;
	r->heredoc_fd = -1;
	r->heredoc_quoted = 0;
	return (r);
}

/* ===== Test helper: inject a variable into shell->variables ============ */

/**
 * @brief Push a (name, value) pair onto shell->variables.
 * @details Goes around var_set so this suite can run on top of the
 *          variables.c stub. stub_shell_cleanup walks the same list and
 *          frees each name/value/var, so we just allocate.
 */
static void	inject_var(t_shell *shell, const char *name, const char *value)
{
	t_var	*var;
	t_list	*node;

	var = malloc(sizeof(t_var));
	var->name = ft_strdup(name);
	var->value = ft_strdup(value);
	var->exported = 0;
	var->readonly = 0;
	node = ft_lstnew(var);
	ft_lstadd(&shell->variables, node);
}

/* ===== xbuf primitives ================================================= */

static void	test_xbuf_basic(void)
{
	t_xbuf	buf;

	MU_ASSERT_INT(0, xbuf_init(&buf));
	MU_ASSERT("empty len", buf.len == 0);
	MU_ASSERT_INT(0, xbuf_putc(&buf, 'a', 1));
	MU_ASSERT_INT(0, xbuf_putc(&buf, 'b', 0));
	MU_ASSERT_INT(0, xbuf_putc(&buf, 'c', 1));
	MU_ASSERT_STR("data", "abc", buf.data);
	MU_ASSERT("len after 3", buf.len == 3);
	MU_ASSERT("mask[0] split", buf.mask[0] == 1);
	MU_ASSERT("mask[1] literal", buf.mask[1] == 0);
	MU_ASSERT("mask[2] split", buf.mask[2] == 1);
	xbuf_free(&buf);
	MU_ASSERT("freed data", buf.data == NULL);
}

static void	test_xbuf_growth(void)
{
	t_xbuf	buf;
	int		i;

	xbuf_init(&buf);
	i = 0;
	while (i < 100)
	{
		xbuf_putc(&buf, 'x', 1);
		i++;
	}
	MU_ASSERT("len after 100 puts", buf.len == 100);
	MU_ASSERT("data still NUL-terminated", buf.data[100] == '\0');
	MU_ASSERT("char retained", buf.data[57] == 'x');
	MU_ASSERT("mask retained", buf.mask[57] == 1);
	xbuf_free(&buf);
}

static void	test_xbuf_puts(void)
{
	t_xbuf	buf;

	xbuf_init(&buf);
	xbuf_puts(&buf, "hello", 1);
	xbuf_puts(&buf, " ", 0);
	xbuf_puts(&buf, "world", 1);
	MU_ASSERT_STR("joined", "hello world", buf.data);
	MU_ASSERT("split mask 'h'", buf.mask[0] == 1);
	MU_ASSERT("space literal", buf.mask[5] == 0);
	MU_ASSERT("split mask 'w'", buf.mask[6] == 1);
	xbuf_free(&buf);
}

/* ===== expand_word: quoting and escapes ================================ */

static void	test_expand_plain(void)
{
	t_shell	shell = {0};
	char	*out;

	stub_shell_init(&shell);
	out = expand_word(&shell, "hello");
	MU_ASSERT_STR("plain word", "hello", out);
	free(out);
	stub_shell_cleanup(&shell);
}

static void	test_expand_single_quotes(void)
{
	t_shell	shell = {0};
	char	*out;

	stub_shell_init(&shell);
	out = expand_word(&shell, "'hello $world'");
	MU_ASSERT_STR("single-quote literal", "hello $world", out);
	free(out);
	stub_shell_cleanup(&shell);
}

static void	test_expand_double_quotes(void)
{
	t_shell	shell = {0};
	char	*out;

	stub_shell_init(&shell);
	inject_var(&shell, "FOO", "bar");
	out = expand_word(&shell, "\"$FOO baz\"");
	MU_ASSERT_STR("dq expansion", "bar baz", out);
	free(out);
	stub_shell_cleanup(&shell);
}

static void	test_expand_mixed_quotes(void)
{
	t_shell	shell = {0};
	char	*out;

	stub_shell_init(&shell);
	inject_var(&shell, "X", "Y");
	out = expand_word(&shell, "a'$X'b\"$X\"c");
	MU_ASSERT_STR("mixed quotes", "a$XbYc", out);
	free(out);
	stub_shell_cleanup(&shell);
}

static void	test_expand_backslash_unquoted(void)
{
	t_shell	shell = {0};
	char	*out;

	stub_shell_init(&shell);
	out = expand_word(&shell, "a\\$b");
	MU_ASSERT_STR("escaped dollar", "a$b", out);
	free(out);
	out = expand_word(&shell, "\\\\");
	MU_ASSERT_STR("escaped backslash", "\\", out);
	free(out);
	stub_shell_cleanup(&shell);
}

static void	test_expand_backslash_dq(void)
{
	t_shell	shell = {0};
	char	*out;

	stub_shell_init(&shell);
	out = expand_word(&shell, "\"a\\$b\"");
	MU_ASSERT_STR("dq escape $", "a$b", out);
	free(out);
	out = expand_word(&shell, "\"a\\nb\"");
	MU_ASSERT_STR("dq backslash literal", "a\\nb", out);
	free(out);
	stub_shell_cleanup(&shell);
}

/* ===== expand_word: parameter expansion ================================ */

static void	test_expand_simple_var(void)
{
	t_shell	shell = {0};
	char	*out;

	stub_shell_init(&shell);
	inject_var(&shell, "NAME", "claude");
	out = expand_word(&shell, "$NAME");
	MU_ASSERT_STR("simple $", "claude", out);
	free(out);
	out = expand_word(&shell, "$NAME-suffix");
	MU_ASSERT_STR("$ then non-name", "claude-suffix", out);
	free(out);
	stub_shell_cleanup(&shell);
}

static void	test_expand_braced_var(void)
{
	t_shell	shell = {0};
	char	*out;

	stub_shell_init(&shell);
	inject_var(&shell, "X", "y");
	out = expand_word(&shell, "${X}z");
	MU_ASSERT_STR("braced", "yz", out);
	free(out);
	out = expand_word(&shell, "${UNSET}");
	MU_ASSERT_STR("unset braced empty", "", out);
	free(out);
	stub_shell_cleanup(&shell);
}

static void	test_expand_unset_var(void)
{
	t_shell	shell = {0};
	char	*out;

	stub_shell_init(&shell);
	out = expand_word(&shell, "x$NOPE-y");
	MU_ASSERT_STR("unset is empty", "x-y", out);
	free(out);
	stub_shell_cleanup(&shell);
}

static void	test_expand_question(void)
{
	t_shell	shell = {0};
	char	*out;

	stub_shell_init(&shell);
	shell.last_exit_status = 42;
	out = expand_word(&shell, "$?");
	MU_ASSERT_STR("$? value", "42", out);
	free(out);
	out = expand_word(&shell, "${?}");
	MU_ASSERT_STR("${?} value", "42", out);
	free(out);
	stub_shell_cleanup(&shell);
}

static void	test_expand_dollar_dollar(void)
{
	t_shell	shell = {0};
	char	*out;
	char	*expected;

	stub_shell_init(&shell);
	expected = ft_itoa((int)getpid());
	out = expand_word(&shell, "$$");
	MU_ASSERT_STR("$$ is pid", expected, out);
	free(out);
	free(expected);
	stub_shell_cleanup(&shell);
}

static void	test_expand_dollar_zero(void)
{
	t_shell	shell = {0};
	char	*out;

	stub_shell_init(&shell);
	out = expand_word(&shell, "$0");
	MU_ASSERT_STR("$0 is shell name", "42sh", out);
	free(out);
	stub_shell_cleanup(&shell);
}

static void	test_expand_lone_dollar(void)
{
	t_shell	shell = {0};
	char	*out;

	stub_shell_init(&shell);
	out = expand_word(&shell, "abc$");
	MU_ASSERT_STR("trailing $", "abc$", out);
	free(out);
	out = expand_word(&shell, "a$+b");
	MU_ASSERT_STR("$+ literal", "a$+b", out);
	free(out);
	stub_shell_cleanup(&shell);
}

/* ===== expand_word: tilde ============================================== */

static void	test_expand_tilde(void)
{
	t_shell	shell = {0};
	char	*out;

	stub_shell_init(&shell);
	inject_var(&shell, "HOME", "/home/test");
	out = expand_word(&shell, "~");
	MU_ASSERT_STR("tilde HOME", "/home/test", out);
	free(out);
	out = expand_word(&shell, "~/foo");
	MU_ASSERT_STR("tilde + path", "/home/test/foo", out);
	free(out);
	out = expand_word(&shell, "x~");
	MU_ASSERT_STR("not at start", "x~", out);
	free(out);
	out = expand_word(&shell, "\"~\"");
	MU_ASSERT_STR("quoted tilde", "~", out);
	free(out);
	stub_shell_cleanup(&shell);
}

static void	test_expand_tilde_user(void)
{
	t_shell			shell = {0};
	char			*out;
	struct passwd	*pw;
	char			*expected;

	stub_shell_init(&shell);
	pw = getpwuid(getuid());
	if (!pw || !pw->pw_name || !pw->pw_dir)
	{
		MU_ASSERT("getpwuid skipped", 1);
		stub_shell_cleanup(&shell);
		return ;
	}
	expected = ft_strjoin("~", pw->pw_name);
	out = expand_word(&shell, expected);
	MU_ASSERT_STR("~user", pw->pw_dir, out);
	free(out);
	free(expected);
	out = expand_word(&shell, "~no_such_user_xyz_42");
	MU_ASSERT_STR("unknown user literal", "~no_such_user_xyz_42", out);
	free(out);
	stub_shell_cleanup(&shell);
}

static void	test_expand_tilde_no_home(void)
{
	t_shell			shell = {0};
	char			*out;
	struct passwd	*pw;

	stub_shell_init(&shell);
	pw = getpwuid(getuid());
	if (!pw || !pw->pw_dir)
	{
		MU_ASSERT("getpwuid skipped", 1);
		stub_shell_cleanup(&shell);
		return ;
	}
	out = expand_word(&shell, "~");
	MU_ASSERT_STR("tilde no HOME uses passwd", pw->pw_dir, out);
	free(out);
	stub_shell_cleanup(&shell);
}

/* ===== field_split via expand_word_to_fields =========================== */

static int	count_fields(char **fields)
{
	int	n;

	n = 0;
	while (fields && fields[n])
		n++;
	return (n);
}

static void	free_fields(char **fields)
{
	int	i;

	if (!fields)
		return ;
	i = 0;
	while (fields[i])
		free(fields[i++]);
	free(fields);
}

static void	test_split_default_ifs(void)
{
	t_shell	shell = {0};
	char	**f;

	stub_shell_init(&shell);
	inject_var(&shell, "X", "one two three");
	f = expand_word_to_fields(&shell, "$X");
	MU_ASSERT_INT(3, count_fields(f));
	MU_ASSERT_STR("one", "one", f[0]);
	MU_ASSERT_STR("two", "two", f[1]);
	MU_ASSERT_STR("three", "three", f[2]);
	free_fields(f);
	stub_shell_cleanup(&shell);
}

static void	test_split_quoted_value(void)
{
	t_shell	shell = {0};
	char	**f;

	stub_shell_init(&shell);
	inject_var(&shell, "X", "one two three");
	f = expand_word_to_fields(&shell, "\"$X\"");
	MU_ASSERT_INT(1, count_fields(f));
	MU_ASSERT_STR("single field", "one two three", f[0]);
	free_fields(f);
	stub_shell_cleanup(&shell);
}

static void	test_split_literal_spaces_protected(void)
{
	t_shell	shell = {0};
	char	**f;

	stub_shell_init(&shell);
	f = expand_word_to_fields(&shell, "'a b c'");
	MU_ASSERT_INT(1, count_fields(f));
	MU_ASSERT_STR("quoted literal", "a b c", f[0]);
	free_fields(f);
	stub_shell_cleanup(&shell);
}

static void	test_split_custom_ifs(void)
{
	t_shell	shell = {0};
	char	**f;

	stub_shell_init(&shell);
	inject_var(&shell, "IFS", ",");
	inject_var(&shell, "L", "a,b,c");
	f = expand_word_to_fields(&shell, "$L");
	MU_ASSERT_INT(3, count_fields(f));
	MU_ASSERT_STR("a", "a", f[0]);
	MU_ASSERT_STR("b", "b", f[1]);
	MU_ASSERT_STR("c", "c", f[2]);
	free_fields(f);
	stub_shell_cleanup(&shell);
}

static void	test_split_adjacent_delims(void)
{
	t_shell	shell = {0};
	char	**f;

	stub_shell_init(&shell);
	inject_var(&shell, "IFS", ",");
	inject_var(&shell, "L", "a,,b");
	f = expand_word_to_fields(&shell, "$L");
	MU_ASSERT_INT(3, count_fields(f));
	MU_ASSERT_STR("a", "a", f[0]);
	MU_ASSERT_STR("empty middle", "", f[1]);
	MU_ASSERT_STR("b", "b", f[2]);
	free_fields(f);
	stub_shell_cleanup(&shell);
}

static void	test_split_mixed_ws_nonws(void)
{
	t_shell	shell = {0};
	char	**f;

	stub_shell_init(&shell);
	inject_var(&shell, "IFS", ", ");
	inject_var(&shell, "L", "a , , b");
	f = expand_word_to_fields(&shell, "$L");
	MU_ASSERT_INT(3, count_fields(f));
	MU_ASSERT_STR("a", "a", f[0]);
	MU_ASSERT_STR("middle empty", "", f[1]);
	MU_ASSERT_STR("b", "b", f[2]);
	free_fields(f);
	stub_shell_cleanup(&shell);
}

static void	test_split_leading_trailing_ws(void)
{
	t_shell	shell = {0};
	char	**f;

	stub_shell_init(&shell);
	inject_var(&shell, "L", "  hello   world  ");
	f = expand_word_to_fields(&shell, "$L");
	MU_ASSERT_INT(2, count_fields(f));
	MU_ASSERT_STR("hello", "hello", f[0]);
	MU_ASSERT_STR("world", "world", f[1]);
	free_fields(f);
	stub_shell_cleanup(&shell);
}

static void	test_split_empty_ifs_disables(void)
{
	t_shell	shell = {0};
	char	**f;

	stub_shell_init(&shell);
	inject_var(&shell, "IFS", "");
	inject_var(&shell, "L", "a b c");
	f = expand_word_to_fields(&shell, "$L");
	MU_ASSERT_INT(1, count_fields(f));
	MU_ASSERT_STR("single", "a b c", f[0]);
	free_fields(f);
	stub_shell_cleanup(&shell);
}

static void	test_split_unset_var_zero_fields(void)
{
	t_shell	shell = {0};
	char	**f;

	stub_shell_init(&shell);
	f = expand_word_to_fields(&shell, "$NOPE");
	MU_ASSERT_INT(0, count_fields(f));
	free_fields(f);
	stub_shell_cleanup(&shell);
}

/* ===== expand_command ================================================== */

static void	test_expand_command_argv(void)
{
	t_shell	shell = {0};
	t_cmd	*cmd;

	stub_shell_init(&shell);
	inject_var(&shell, "L", "one two three");
	cmd = malloc(sizeof(t_cmd));
	cmd->argv = malloc(sizeof(char *) * 3);
	cmd->argv[0] = ft_strdup("echo");
	cmd->argv[1] = ft_strdup("$L");
	cmd->argv[2] = NULL;
	cmd->argc = 2;
	cmd->assignments = NULL;
	cmd->redirs = NULL;
	MU_ASSERT_INT(0, expand_command(&shell, cmd));
	MU_ASSERT_INT(4, cmd->argc);
	MU_ASSERT_STR("echo", "echo", cmd->argv[0]);
	MU_ASSERT_STR("one", "one", cmd->argv[1]);
	MU_ASSERT_STR("two", "two", cmd->argv[2]);
	MU_ASSERT_STR("three", "three", cmd->argv[3]);
	MU_ASSERT("argv NULL terminated", cmd->argv[4] == NULL);
	ast_free(ast_new_command(cmd));
	stub_shell_cleanup(&shell);
}

static void	test_expand_command_assignment(void)
{
	t_shell	shell = {0};
	t_cmd	*cmd;

	stub_shell_init(&shell);
	inject_var(&shell, "X", "world");
	cmd = malloc(sizeof(t_cmd));
	cmd->argv = NULL;
	cmd->argc = 0;
	cmd->redirs = NULL;
	cmd->assignments = ft_lstnew(ft_strdup("GREETING=hello $X"));
	MU_ASSERT_INT(0, expand_command(&shell, cmd));
	MU_ASSERT_STR("assignment expanded",
		"GREETING=hello world",
		(char *)cmd->assignments->content);
	ast_free(ast_new_command(cmd));
	stub_shell_cleanup(&shell);
}

static void	test_expand_command_redir_target(void)
{
	t_shell	shell = {0};
	t_cmd	*cmd;
	t_redir	*redir;

	stub_shell_init(&shell);
	inject_var(&shell, "OUT", "/tmp/out");
	cmd = malloc(sizeof(t_cmd));
	cmd->argv = NULL;
	cmd->argc = 0;
	cmd->assignments = NULL;
	redir = make_redir(TOK_REDIR_OUT, -1, "$OUT.txt");
	cmd->redirs = ft_lstnew(redir);
	MU_ASSERT_INT(0, expand_command(&shell, cmd));
	MU_ASSERT_STR("redir target expanded", "/tmp/out.txt", redir->target);
	ast_free(ast_new_command(cmd));
	stub_shell_cleanup(&shell);
}

static void	test_expand_command_ambiguous_redir(void)
{
	t_shell	shell = {0};
	t_cmd	*cmd;
	t_redir	*redir;
	int		ret;

	stub_shell_init(&shell);
	inject_var(&shell, "MULTI", "/tmp/a /tmp/b");
	cmd = malloc(sizeof(t_cmd));
	cmd->argv = NULL;
	cmd->argc = 0;
	cmd->assignments = NULL;
	redir = make_redir(TOK_REDIR_OUT, -1, "$MULTI");
	cmd->redirs = ft_lstnew(redir);
	ret = expand_command(&shell, cmd);
	MU_ASSERT("ambiguous redir is an error", ret != 0);
	ast_free(ast_new_command(cmd));
	stub_shell_cleanup(&shell);
}

static void	test_expand_command_heredoc_target_skipped(void)
{
	t_shell	shell = {0};
	t_cmd	*cmd;
	t_redir	*redir;

	stub_shell_init(&shell);
	inject_var(&shell, "EOF", "boom");
	cmd = malloc(sizeof(t_cmd));
	cmd->argv = NULL;
	cmd->argc = 0;
	cmd->assignments = NULL;
	redir = make_redir(TOK_HEREDOC, 0, "$EOF");
	cmd->redirs = ft_lstnew(redir);
	MU_ASSERT_INT(0, expand_command(&shell, cmd));
	MU_ASSERT_STR("heredoc target untouched", "$EOF", redir->target);
	ast_free(ast_new_command(cmd));
	stub_shell_cleanup(&shell);
}

/* ===== Suite registration ============================================== */

void	test_expander_suite(void)
{
	test_xbuf_basic();
	test_xbuf_growth();
	test_xbuf_puts();
	test_expand_plain();
	test_expand_single_quotes();
	test_expand_double_quotes();
	test_expand_mixed_quotes();
	test_expand_backslash_unquoted();
	test_expand_backslash_dq();
	test_expand_simple_var();
	test_expand_braced_var();
	test_expand_unset_var();
	test_expand_question();
	test_expand_dollar_dollar();
	test_expand_dollar_zero();
	test_expand_lone_dollar();
	test_expand_tilde();
	test_expand_tilde_user();
	test_expand_tilde_no_home();
	test_split_default_ifs();
	test_split_quoted_value();
	test_split_literal_spaces_protected();
	test_split_custom_ifs();
	test_split_adjacent_delims();
	test_split_mixed_ws_nonws();
	test_split_leading_trailing_ws();
	test_split_empty_ifs_disables();
	test_split_unset_var_zero_fields();
	test_expand_command_argv();
	test_expand_command_assignment();
	test_expand_command_redir_target();
	test_expand_command_ambiguous_redir();
	test_expand_command_heredoc_target_skipped();
}

#else

void	test_expander_suite(void)
{
}

#endif
