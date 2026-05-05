/**
 * @file test_builtin_exit.c
 * @brief Unit tests for the exit builtin command.
 * @author zweng
 */

#ifdef TEST_BUILTIN_EXIT_ENABLED
#endif

#define BUFSIZE 256

# include "minunit.h"
# include "builtins.h"
# include <stdlib.h>
# include <string.h>
# include <unistd.h>

/**
 * @brief Capture stderr output from a builtin call.
 * @details Redirects stderr to a pipe, runs the builtin, reads the output.
 */
static int	capture_exit_stderr(t_shell *shell, int argc, char **argv,
	char *buf, size_t buf_size)
{
	int		pipefd[2];
	int		saved_stderr;
	int		ret;
	ssize_t	n;
	ssize_t	total;

	total = 0;
	memset(buf, 0, buf_size);
	if (pipe(pipefd) == -1)
		return (-1);
	saved_stderr = dup(STDERR_FILENO);
	if (saved_stderr == -1)
		return (-1);
	if (dup2(pipefd[1], STDERR_FILENO) == -1)
		return (-1);
	close(pipefd[1]);
	ret = builtin_exit(shell, argc, argv);
	if (dup2(saved_stderr, STDERR_FILENO) == -1)
		return (-1);
	close(saved_stderr);
	while ((n = read(pipefd[0], buf + total, buf_size - 1 - total)) > 0)
		total += n;
	buf[total] = '\0';
	close(pipefd[0]);
	return (ret);
}

/* --- exit with no arguments --- */

static void	test_exit_no_args_status_zero(void)
{
	t_shell	shell;
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	shell.running = 1;
	shell.last_exit_status = 0;
	char *argv[] = {"exit"};
	ret = capture_exit_stderr(&shell, 1, argv, buf, BUFSIZE);
	MU_ASSERT_INT(0, ret);
	MU_ASSERT_INT(0, shell.running);
	MU_ASSERT("no args exits with current status", shell.last_exit_status == 0);
}

static void	test_exit_no_args_preserves_last_status(void)
{
	t_shell	shell;
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	shell.running = 1;
	shell.last_exit_status = 42;
	char *argv[] = {"exit"};
	ret = capture_exit_stderr(&shell, 1, argv, buf, BUFSIZE);
	MU_ASSERT_INT(42, ret);
	MU_ASSERT_INT(0, shell.running);
	MU_ASSERT_INT(42, shell.last_exit_status);
}

/* --- exit with valid numeric argument --- */

static void	test_exit_numeric_zero(void)
{
	t_shell	shell;
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	shell.running = 1;
	char *argv[] = {"exit", "0"};
	ret = capture_exit_stderr(&shell, 2, argv, buf, BUFSIZE);
	MU_ASSERT_INT(0, ret);
	MU_ASSERT_INT(0, shell.running);
	MU_ASSERT_INT(0, shell.last_exit_status);
}

static void	test_exit_numeric_42(void)
{
	t_shell	shell;
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	shell.running = 1;
	char *argv[] = {"exit", "42"};
	ret = capture_exit_stderr(&shell, 2, argv, buf, BUFSIZE);
	MU_ASSERT_INT(42, ret);
	MU_ASSERT_INT(0, shell.running);
	MU_ASSERT_INT(42, shell.last_exit_status);
}

static void	test_exit_numeric_255(void)
{
	t_shell	shell;
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	shell.running = 1;
	char *argv[] = {"exit", "255"};
	ret = capture_exit_stderr(&shell, 2, argv, buf, BUFSIZE);
	MU_ASSERT_INT(255, ret);
	MU_ASSERT_INT(0, shell.running);
	MU_ASSERT_INT(255, shell.last_exit_status);
}

static void	test_exit_numeric_256_wraps(void)
{
	t_shell	shell;
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	shell.running = 1;
	char *argv[] = {"exit", "256"};
	ret = capture_exit_stderr(&shell, 2, argv, buf, BUFSIZE);
	MU_ASSERT_INT(0, ret);
	MU_ASSERT_INT(0, shell.running);
	MU_ASSERT("256 wraps to 0", shell.last_exit_status == 0);
}

static void	test_exit_numeric_negative(void)
{
	t_shell	shell;
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	shell.running = 1;
	char *argv[] = {"exit", "-1"};
	ret = capture_exit_stderr(&shell, 2, argv, buf, BUFSIZE);
	MU_ASSERT_INT(255, ret);
	MU_ASSERT_INT(0, shell.running);
	MU_ASSERT("-1 wraps to 255", shell.last_exit_status == 255);
}

static void	test_exit_numeric_positive_sign(void)
{
	t_shell	shell;
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	shell.running = 1;
	char *argv[] = {"exit", "+5"};
	ret = capture_exit_stderr(&shell, 2, argv, buf, BUFSIZE);
	MU_ASSERT_INT(5, ret);
	MU_ASSERT_INT(0, shell.running);
	MU_ASSERT_INT(5, shell.last_exit_status);
}

/* --- exit with non-numeric argument --- */

static void	test_exit_non_numeric_alpha(void)
{
	t_shell	shell;
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	shell.running = 1;
	char *argv[] = {"exit", "foo"};
	ret = capture_exit_stderr(&shell, 2, argv, buf, BUFSIZE);
	MU_ASSERT_INT(2, ret);
	MU_ASSERT_INT(0, shell.running);
	MU_ASSERT_INT(2, shell.last_exit_status);
	MU_ASSERT_STR("numeric argument required msg",
		"42sh: exit: foo: numeric argument required\n", buf);
}

static void	test_exit_non_numeric_mixed(void)
{
	t_shell	shell;
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	shell.running = 1;
	char *argv[] = {"exit", "12abc"};
	ret = capture_exit_stderr(&shell, 2, argv, buf, BUFSIZE);
	MU_ASSERT_INT(2, ret);
	MU_ASSERT_INT(0, shell.running);
	MU_ASSERT_STR("mixed alphanumeric is non-numeric",
		"42sh: exit: 12abc: numeric argument required\n", buf);
}

static void	test_exit_non_numeric_empty(void)
{
	t_shell	shell;
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	shell.running = 1;
	char *argv[] = {"exit", ""};
	ret = capture_exit_stderr(&shell, 2, argv, buf, BUFSIZE);
	MU_ASSERT_INT(2, ret);
	MU_ASSERT_INT(0, shell.running);
	MU_ASSERT_STR("empty string is non-numeric",
		"42sh: exit: : numeric argument required\n", buf);
}

static void	test_exit_non_numeric_bare_sign(void)
{
	t_shell	shell;
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	shell.running = 1;
	char *argv[] = {"exit", "-"};
	ret = capture_exit_stderr(&shell, 2, argv, buf, BUFSIZE);
	MU_ASSERT_INT(2, ret);
	MU_ASSERT_INT(0, shell.running);
	MU_ASSERT_STR("bare minus is non-numeric",
		"42sh: exit: -: numeric argument required\n", buf);
}

/* --- exit with too many arguments --- */

static void	test_exit_too_many_args(void)
{
	t_shell	shell;
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	shell.running = 1;
	char *argv[] = {"exit", "1", "2"};
	ret = capture_exit_stderr(&shell, 3, argv, buf, BUFSIZE);
	MU_ASSERT_INT(1, ret);
	MU_ASSERT("too many args does NOT exit", shell.running == 1);
	MU_ASSERT_INT(1, shell.last_exit_status);
	MU_ASSERT_STR("too many arguments msg",
		"42sh: exit: too many arguments\n", buf);
}

static void	test_exit_too_many_args_three(void)
{
	t_shell	shell;
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	shell.running = 1;
	char *argv[] = {"exit", "1", "2", "3"};
	ret = capture_exit_stderr(&shell, 4, argv, buf, BUFSIZE);
	MU_ASSERT_INT(1, ret);
	MU_ASSERT("three extra args does NOT exit", shell.running == 1);
}

/* --- exit with non-numeric + too many args: non-numeric takes priority --- */

static void	test_exit_non_numeric_takes_priority(void)
{
	t_shell	shell;
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	shell.running = 1;
	char *argv[] = {"exit", "foo", "bar"};
	ret = capture_exit_stderr(&shell, 3, argv, buf, BUFSIZE);
	MU_ASSERT_INT(2, ret);
	MU_ASSERT_INT(0, shell.running);
	MU_ASSERT_STR("non-numeric error takes priority over too-many-args",
		"42sh: exit: foo: numeric argument required\n", buf);
}

/* --- interactive mode prints "exit" --- */

static void	test_exit_interactive_prints_exit(void)
{
	t_shell	shell;
	char	buf[BUFSIZE];

	memset(&shell, 0, sizeof(shell));
	shell.running = 1;
	shell.interactive = 1;
	char *argv[] = {"exit"};
	capture_exit_stderr(&shell, 1, argv, buf, BUFSIZE);
	MU_ASSERT_STR("interactive mode prints exit", "exit\n", buf);
}

static void	test_exit_non_interactive_silent(void)
{
	t_shell	shell;
	char	buf[BUFSIZE];

	memset(&shell, 0, sizeof(shell));
	shell.running = 1;
	shell.interactive = 0;
	char *argv[] = {"exit"};
	capture_exit_stderr(&shell, 1, argv, buf, BUFSIZE);
	MU_ASSERT_STR("non-interactive mode is silent", "", buf);
}

void	test_builtin_exit_suite(void)
{
	test_exit_no_args_status_zero();
	test_exit_no_args_preserves_last_status();
	test_exit_numeric_zero();
	test_exit_numeric_42();
	test_exit_numeric_255();
	test_exit_numeric_256_wraps();
	test_exit_numeric_negative();
	test_exit_numeric_positive_sign();
	test_exit_non_numeric_alpha();
	test_exit_non_numeric_mixed();
	test_exit_non_numeric_empty();
	test_exit_non_numeric_bare_sign();
	test_exit_too_many_args();
	test_exit_too_many_args_three();
	test_exit_non_numeric_takes_priority();
	test_exit_interactive_prints_exit();
	test_exit_non_interactive_silent();
}
