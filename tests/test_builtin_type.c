/**
 * @file test_builtin_type.c
 * @brief Unit tests for the type builtin command.
 * @author zweng
 */

#ifdef TEST_BUILTIN_TYPE_ENABLED
#endif

#define BUFSIZE 512

# include "minunit.h"
# include "builtins.h"
# include <stdlib.h>
# include <string.h>
# include <unistd.h>

/**
 * @brief Capture stdout output from a builtin call.
 */
static int	capture_type_stdout(t_shell *shell, int argc, char **argv,
	char *buf, size_t buf_size)
{
	int		pipefd[2];
	int		saved_stdout;
	int		ret;
	ssize_t	n;
	ssize_t	total;

	total = 0;
	memset(buf, 0, buf_size);
	if (pipe(pipefd) == -1)
		return (-1);
	saved_stdout = dup(STDOUT_FILENO);
	if (saved_stdout == -1)
		return (-1);
	fflush(stdout);
	if (dup2(pipefd[1], STDOUT_FILENO) == -1)
		return (-1);
	setvbuf(stdout, NULL, _IONBF, 0);
	close(pipefd[1]);
	ret = builtin_type(shell, argc, argv);
	fflush(stdout);
	if (dup2(saved_stdout, STDOUT_FILENO) == -1)
		return (-1);
	close(saved_stdout);
	while ((n = read(pipefd[0], buf + total, buf_size - 1 - total)) > 0)
		total += n;
	buf[total] = '\0';
	close(pipefd[0]);
	return (ret);
}

/**
 * @brief Capture stderr output from a builtin call.
 */
static int	capture_type_stderr(t_shell *shell, int argc, char **argv,
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
	ret = builtin_type(shell, argc, argv);
	if (dup2(saved_stderr, STDERR_FILENO) == -1)
		return (-1);
	close(saved_stderr);
	while ((n = read(pipefd[0], buf + total, buf_size - 1 - total)) > 0)
		total += n;
	buf[total] = '\0';
	close(pipefd[0]);
	return (ret);
}

/* --- type identifies builtins --- */

static void	test_type_builtin_echo(void)
{
	t_shell	shell;
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	char *argv[] = {"type", "echo"};
	ret = capture_type_stdout(&shell, 2, argv, buf, BUFSIZE);
	MU_ASSERT_INT(0, ret);
	MU_ASSERT_INT(0, shell.last_exit_status);
	MU_ASSERT_STR("echo is a builtin", "echo is a shell builtin\n", buf);
}

static void	test_type_builtin_exit(void)
{
	t_shell	shell;
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	char *argv[] = {"type", "exit"};
	ret = capture_type_stdout(&shell, 2, argv, buf, BUFSIZE);
	MU_ASSERT_INT(0, ret);
	MU_ASSERT_STR("exit is a builtin", "exit is a shell builtin\n", buf);
}

static void	test_type_builtin_type(void)
{
	t_shell	shell;
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	char *argv[] = {"type", "type"};
	ret = capture_type_stdout(&shell, 2, argv, buf, BUFSIZE);
	MU_ASSERT_INT(0, ret);
	MU_ASSERT_STR("type is a builtin", "type is a shell builtin\n", buf);
}

static void	test_type_builtin_history(void)
{
	t_shell	shell;
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	char *argv[] = {"type", "history"};
	ret = capture_type_stdout(&shell, 2, argv, buf, BUFSIZE);
	MU_ASSERT_INT(0, ret);
	MU_ASSERT_STR("history is a builtin",
		"history is a shell builtin\n", buf);
}

/* --- type with not found --- */

static void	test_type_not_found(void)
{
	t_shell	shell;
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	char *argv[] = {"type", "nonexistent_cmd_xyz"};
	ret = capture_type_stderr(&shell, 2, argv, buf, BUFSIZE);
	MU_ASSERT_INT(1, ret);
	MU_ASSERT_INT(1, shell.last_exit_status);
	MU_ASSERT_STR("not found error",
		"nonexistent_cmd_xyz not found\n", buf);
}

/* --- type with multiple builtins --- */

static void	test_type_multiple_builtins(void)
{
	t_shell	shell;
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	char *argv[] = {"type", "echo", "exit"};
	ret = capture_type_stdout(&shell, 3, argv, buf, BUFSIZE);
	MU_ASSERT_INT(0, ret);
	MU_ASSERT_STR("multiple builtins",
		"echo is a shell builtin\nexit is a shell builtin\n", buf);
}

/* --- type mixed: builtin + not found returns 1 --- */

static void	test_type_mixed_returns_failure(void)
{
	t_shell	shell;
	char	stdout_buf[BUFSIZE];
	char	stderr_buf[BUFSIZE];
	int		pipeout[2];
	int		pipeerr[2];
	int		saved_stdout;
	int		saved_stderr;
	int		ret;
	ssize_t	n;
	ssize_t	total;

	memset(&shell, 0, sizeof(shell));
	char *argv[] = {"type", "echo", "nonexistent_cmd_xyz"};
	pipe(pipeout);
	pipe(pipeerr);
	saved_stdout = dup(STDOUT_FILENO);
	saved_stderr = dup(STDERR_FILENO);
	fflush(stdout);
	dup2(pipeout[1], STDOUT_FILENO);
	dup2(pipeerr[1], STDERR_FILENO);
	setvbuf(stdout, NULL, _IONBF, 0);
	close(pipeout[1]);
	close(pipeerr[1]);
	ret = builtin_type(&shell, 3, argv);
	fflush(stdout);
	dup2(saved_stdout, STDOUT_FILENO);
	dup2(saved_stderr, STDERR_FILENO);
	close(saved_stdout);
	close(saved_stderr);
	memset(stdout_buf, 0, BUFSIZE);
	total = 0;
	while ((n = read(pipeout[0], stdout_buf + total,
					BUFSIZE - 1 - total)) > 0)
		total += n;
	stdout_buf[total] = '\0';
	close(pipeout[0]);
	memset(stderr_buf, 0, BUFSIZE);
	total = 0;
	while ((n = read(pipeerr[0], stderr_buf + total,
					BUFSIZE - 1 - total)) > 0)
		total += n;
	stderr_buf[total] = '\0';
	close(pipeerr[0]);
	MU_ASSERT_INT(1, ret);
	MU_ASSERT_STR("stdout has builtin info",
		"echo is a shell builtin\n", stdout_buf);
	MU_ASSERT_STR("stderr has not found",
		"nonexistent_cmd_xyz not found\n", stderr_buf);
}

/* --- type with no arguments --- */

static void	test_type_no_args(void)
{
	t_shell	shell;
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	char *argv[] = {"type"};
	ret = capture_type_stdout(&shell, 1, argv, buf, BUFSIZE);
	MU_ASSERT_INT(0, ret);
	MU_ASSERT_STR("no args produces no output", "", buf);
}

void	test_builtin_type_suite(void)
{
	test_type_builtin_echo();
	test_type_builtin_exit();
	test_type_builtin_type();
	test_type_builtin_history();
	test_type_not_found();
	test_type_multiple_builtins();
	test_type_mixed_returns_failure();
	test_type_no_args();
}
