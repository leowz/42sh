/**
 * @file test_builtin_alias.c
 * @brief Tests for the `alias` builtin.
 *
 * Covers defining aliases, NAME=VALUE parsing (including `=` in the
 * value), querying a single name, listing all aliases sorted by name,
 * and POSIX single-quote escaping of values in the listing.
 * @author zweng
 */

#ifdef TEST_ALIAS_ENABLED

# include "minunit.h"
# include "../includes/42sh.h"
# include "../includes/builtins.h"
# include "../includes/aliases.h"
# include <stdlib.h>
# include <string.h>
# include <unistd.h>

static void	alias_b_init(t_shell *shell)
{
	memset(shell, 0, sizeof(*shell));
}

/**
 * @brief Run builtin_alias capturing stdout into @p buf.
 * @return The builtin's return value.
 */
static int	capture_alias(t_shell *shell, int argc, char **argv,
		char *buf, size_t size)
{
	int		pipefd[2];
	int		saved;
	int		ret;
	ssize_t	n;
	ssize_t	total;

	total = 0;
	memset(buf, 0, size);
	if (pipe(pipefd) == -1)
		return (-1);
	saved = dup(1);
	dup2(pipefd[1], 1);
	close(pipefd[1]);
	ret = builtin_alias(shell, argc, argv);
	dup2(saved, 1);
	close(saved);
	while ((n = read(pipefd[0], buf + total, size - 1 - total)) > 0)
		total += n;
	buf[total] = '\0';
	close(pipefd[0]);
	return (ret);
}

static void	test_alias_builtin_sets(void)
{
	t_shell	shell = {0};
	char	*argv[] = {"alias", "ll=ls -la", NULL};

	alias_b_init(&shell);
	MU_ASSERT_INT(0, builtin_alias(&shell, 2, argv));
	MU_ASSERT_STR("alias defined via builtin", "ls -la",
		alias_get_value(&shell, "ll"));
	alias_clear(&shell);
}

static void	test_alias_builtin_equals_in_value(void)
{
	t_shell	shell = {0};
	char	*argv[] = {"alias", "cfg=a=b=c", NULL};

	alias_b_init(&shell);
	MU_ASSERT_INT(0, builtin_alias(&shell, 2, argv));
	MU_ASSERT_STR("only first = splits", "a=b=c",
		alias_get_value(&shell, "cfg"));
	alias_clear(&shell);
}

static void	test_alias_builtin_query_missing(void)
{
	t_shell	shell = {0};
	char	*argv[] = {"alias", "ghost", NULL};

	alias_b_init(&shell);
	MU_ASSERT_INT(1, builtin_alias(&shell, 2, argv));
	alias_clear(&shell);
}

static void	test_alias_builtin_query_existing(void)
{
	t_shell	shell = {0};
	char	buf[256];
	char	*set[] = {"alias", "ll=ls -la", NULL};
	char	*get[] = {"alias", "ll", NULL};

	alias_b_init(&shell);
	builtin_alias(&shell, 2, set);
	MU_ASSERT_INT(0, capture_alias(&shell, 2, get, buf, sizeof(buf)));
	MU_ASSERT_STR("query prints alias line", "alias ll='ls -la'\n", buf);
	alias_clear(&shell);
}

static void	test_alias_builtin_list_sorted(void)
{
	t_shell	shell = {0};
	char	buf[256];
	char	*argv[] = {"alias", NULL};

	alias_b_init(&shell);
	alias_set(&shell, "zz", "Z");
	alias_set(&shell, "aa", "A");
	alias_set(&shell, "mm", "M");
	capture_alias(&shell, 1, argv, buf, sizeof(buf));
	MU_ASSERT_STR("no-arg listing is sorted by name",
		"alias aa='A'\nalias mm='M'\nalias zz='Z'\n", buf);
	alias_clear(&shell);
}

static void	test_alias_builtin_quote_escaping(void)
{
	t_shell	shell = {0};
	char	buf[256];
	char	*get[] = {"alias", "x", NULL};

	alias_b_init(&shell);
	alias_set(&shell, "x", "a'b");
	capture_alias(&shell, 2, get, buf, sizeof(buf));
	MU_ASSERT_STR("embedded quote uses '\\'' escaping",
		"alias x='a'\\''b'\n", buf);
	alias_clear(&shell);
}

static void	test_unalias_removes(void)
{
	t_shell	shell = {0};
	char	*argv[] = {"unalias", "ll", NULL};

	alias_b_init(&shell);
	alias_set(&shell, "ll", "ls -la");
	MU_ASSERT_INT(0, builtin_unalias(&shell, 2, argv));
	MU_ASSERT("alias removed by unalias",
		alias_get_value(&shell, "ll") == NULL);
	alias_clear(&shell);
}

static void	test_unalias_missing(void)
{
	t_shell	shell = {0};
	char	*argv[] = {"unalias", "ghost", NULL};

	alias_b_init(&shell);
	MU_ASSERT_INT(1, builtin_unalias(&shell, 2, argv));
	alias_clear(&shell);
}

static void	test_unalias_all(void)
{
	t_shell	shell = {0};
	char	*argv[] = {"unalias", "-a", NULL};

	alias_b_init(&shell);
	alias_set(&shell, "a", "1");
	alias_set(&shell, "b", "2");
	alias_set(&shell, "c", "3");
	MU_ASSERT_INT(0, builtin_unalias(&shell, 2, argv));
	MU_ASSERT("unalias -a clears the table", shell.aliases == NULL);
	alias_clear(&shell);
}

static void	test_unalias_no_args(void)
{
	t_shell	shell = {0};
	char	*argv[] = {"unalias", NULL};

	alias_b_init(&shell);
	MU_ASSERT_INT(2, builtin_unalias(&shell, 1, argv));
	alias_clear(&shell);
}

void	test_builtin_alias_suite(void)
{
	test_alias_builtin_sets();
	test_alias_builtin_equals_in_value();
	test_alias_builtin_query_missing();
	test_alias_builtin_query_existing();
	test_alias_builtin_list_sorted();
	test_alias_builtin_quote_escaping();
	test_unalias_removes();
	test_unalias_missing();
	test_unalias_all();
	test_unalias_no_args();
}

#else

void	test_builtin_alias_suite(void)
{
}

#endif
