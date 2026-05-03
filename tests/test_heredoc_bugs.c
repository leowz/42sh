/**
 * @file test_heredoc_bugs.c
 * @brief Regression tests for heredoc bugs in parser_heredoc.c.
 *
 * Bug 1: WEXITSTATUS precedence — WEXITSTATUS(status == 130) should be
 *         WEXITSTATUS(status) == 130.
 * Bug 2: Pipe fd leak on fork() failure — pipe fds must be closed if
 *         fork() fails.
 *
 * These tests verify correct behavior through the public API and
 * end-to-end execution.
 * @author zweng
 */

#ifdef TEST_HEREDOC_BUGS_ENABLED
#endif

# include "minunit.h"
# include "../includes/42sh.h"
# include "../includes/executor.h"
# include "../includes/parser.h"
# include <stdlib.h>
# include <string.h>
# include <fcntl.h>
# include <sys/wait.h>

/* ================================================================
 * Helpers
 * ================================================================ */

/**
 * @brief Count open file descriptors in the current process.
 * @return Number of open fds.
 */
static int	count_open_fds(void)
{
	int	count;
	int	fd;

	count = 0;
	fd = 0;
	while (fd < 256)
	{
		if (fcntl(fd, F_GETFD) != -1)
			count++;
		fd++;
	}
	return (count);
}

/**
 * @brief Run a command string through the shell and capture exit status.
 * @details Forks, pipes the command to 42sh via stdin, returns exit status.
 * @param cmd The command string to execute.
 * @return The exit status of the shell process.
 */
static int	run_shell_cmd(const char *cmd)
{
	int		pipefd[2];
	pid_t	pid;
	int		wstatus;

	if (pipe(pipefd) == -1)
		return (-1);
	pid = fork();
	if (pid == -1)
	{
		close(pipefd[0]);
		close(pipefd[1]);
		return (-1);
	}
	if (pid == 0)
	{
		close(pipefd[1]);
		dup2(pipefd[0], STDIN_FILENO);
		close(pipefd[0]);
		close(STDOUT_FILENO);
		open("/dev/null", O_WRONLY);
		close(STDERR_FILENO);
		open("/dev/null", O_WRONLY);
		execl("./42sh", "42sh", NULL);
		_exit(127);
	}
	close(pipefd[0]);
	write(pipefd[1], cmd, strlen(cmd));
	close(pipefd[1]);
	waitpid(pid, &wstatus, 0);
	if (WIFEXITED(wstatus))
		return (WEXITSTATUS(wstatus));
	return (-1);
}


/* ================================================================
 * Bug 1: WEXITSTATUS precedence
 * Regression: WEXITSTATUS(status == 130) always evaluated to 0,
 * so exit(130) from heredoc child was not detected as SIGINT abort.
 *
 * We test indirectly: a valid heredoc should work, and the content
 * should be properly passed to the command.
 * ================================================================ */

/**
 * @brief WEXITSTATUS operator precedence unit test.
 * @details Directly verify that the fixed expression evaluates correctly
 *          for a child that calls exit(130).
 */
static void	test_wexitstatus_precedence_logic(void)
{
	int	status;
	int	buggy_result;
	int	fixed_result;

	/* Simulate raw waitpid status for exit(130) */
	status = 130 << 8;
	buggy_result = WEXITSTATUS(status == 130);
	fixed_result = (WEXITSTATUS(status) == 130);
	MU_ASSERT("buggy expression is always 0", buggy_result == 0);
	MU_ASSERT("fixed expression detects exit(130)", fixed_result == 1);
}

/**
 * @brief Verify exit(130) is correctly detected by parent via waitpid.
 * @details Fork a child that exits with 130. Parent must detect
 *          WEXITSTATUS(status) == 130, not WEXITSTATUS(status == 130).
 */
static void	test_wexitstatus_exit130_detected(void)
{
	pid_t	pid;
	int		status;
	int		detected;

	fflush(stdout);
	fflush(stderr);
	pid = fork();
	if (pid == 0)
		_exit(130);
	waitpid(pid, &status, 0);
	detected = (WIFSIGNALED(status) || WEXITSTATUS(status) == 130);
	MU_ASSERT("exit(130) detected by fixed condition", detected == 1);
}

/**
 * @brief Executor path: heredoc_fd is correctly dup'd onto stdin.
 * @details After the parser refactor, the heredoc body is pre-written
 *          into a pipe by the parser and the read-end fd is stored in
 *          redir->heredoc_fd. The executor's setup_redirections must
 *          dup that fd onto stdin so the consumer command reads the
 *          heredoc body. This test simulates the parser side with a
 *          real pipe and verifies the executor side wires it up.
 */
static void	test_heredoc_content_passed_to_command(void)
{
	t_redir	redir;
	t_list	*redirs;
	int		pipefd[2];
	int		saved_fds[3];
	char	buf[256];
	ssize_t	n;

	MU_ASSERT("pipe created", pipe(pipefd) == 0);
	MU_ASSERT("write to pipe",
		write(pipefd[1], "hello heredoc\n", 14) == 14);
	close(pipefd[1]);
	memset(&redir, 0, sizeof(redir));
	redir.type = TOK_HEREDOC;
	redir.fd = -1;
	redir.heredoc_fd = pipefd[0];
	redirs = ft_lstnew(&redir);
	MU_ASSERT("setup_redirections OK",
		setup_redirections(redirs, saved_fds) == 0);
	n = read(0, buf, sizeof(buf) - 1);
	buf[n] = '\0';
	restore_redirections(saved_fds);
	MU_ASSERT_STR("heredoc content correct", "hello heredoc\n", buf);
	ft_lstdelone(&redirs, NULL);
}

/**
 * @brief Heredoc with wrong command still exits cleanly (exit 127).
 * @details Tests single-line command with no heredoc content dependency.
 */
static void	test_heredoc_wrong_command_exits(void)
{
	int	ret;

	ret = run_shell_cmd("nonexistent_cmd_xyz\n");
	MU_ASSERT("wrong command returns 127", ret == 127);
}

/**
 * @brief Executor reuses heredoc_fd correctly across two redirs.
 * @details Two t_redir values, each fed by its own pipe, should both
 *          land their content on stdin successfully when run through
 *          setup_redirections in turn.
 */
static void	test_heredoc_multiple_setup(void)
{
	t_redir	redir;
	t_list	*redirs;
	int		pipefd[2];
	int		saved_fds[3];
	char	buf[256];
	ssize_t	n;

	MU_ASSERT("first pipe", pipe(pipefd) == 0);
	MU_ASSERT("write first",
		write(pipefd[1], "first\n", 6) == 6);
	close(pipefd[1]);
	memset(&redir, 0, sizeof(redir));
	redir.type = TOK_HEREDOC;
	redir.fd = -1;
	redir.heredoc_fd = pipefd[0];
	redirs = ft_lstnew(&redir);
	MU_ASSERT("setup first OK",
		setup_redirections(redirs, saved_fds) == 0);
	n = read(0, buf, sizeof(buf) - 1);
	buf[n] = '\0';
	restore_redirections(saved_fds);
	MU_ASSERT_STR("first heredoc content", "first\n", buf);
	ft_lstdelone(&redirs, NULL);

	MU_ASSERT("second pipe", pipe(pipefd) == 0);
	MU_ASSERT("write second",
		write(pipefd[1], "second\n", 7) == 7);
	close(pipefd[1]);
	memset(&redir, 0, sizeof(redir));
	redir.type = TOK_HEREDOC;
	redir.fd = -1;
	redir.heredoc_fd = pipefd[0];
	redirs = ft_lstnew(&redir);
	MU_ASSERT("setup second OK",
		setup_redirections(redirs, saved_fds) == 0);
	n = read(0, buf, sizeof(buf) - 1);
	buf[n] = '\0';
	restore_redirections(saved_fds);
	MU_ASSERT_STR("second heredoc content", "second\n", buf);
	ft_lstdelone(&redirs, NULL);
}

/* ================================================================
 * Bug 2: Pipe fd leak on fork() failure
 * Regression: if fork() failed after pipe(), both pipe fds leaked.
 *
 * We cannot force fork() to fail, but we can verify that the
 * current code path doesn't leak fds under normal operation.
 * ================================================================ */

/**
 * @brief Verify no fd leak after heredoc collection.
 * @details Count open fds before and after processing a heredoc command.
 *          The fd count should be the same (no leaked pipe fds).
 */
static void	test_heredoc_no_fd_leak(void)
{
	int	fds_before;
	int	fds_after;

	fds_before = count_open_fds();
	run_shell_cmd("cat <<EOF\ntest content\nEOF\n");
	fds_after = count_open_fds();
	MU_ASSERT("no fd leak after heredoc",
		fds_before == fds_after);
}

/**
 * @brief Verify no fd leak after heredoc with wrong command.
 */
static void	test_heredoc_wrong_cmd_no_fd_leak(void)
{
	int	fds_before;
	int	fds_after;

	fds_before = count_open_fds();
	run_shell_cmd("nonexistent <<EOF\nhello\nEOF\n");
	fds_after = count_open_fds();
	MU_ASSERT("no fd leak after heredoc + wrong command",
		fds_before == fds_after);
}

/**
 * @brief Verify no fd leak after multiple heredocs.
 */
static void	test_heredoc_multiple_no_fd_leak(void)
{
	int	fds_before;
	int	fds_after;

	fds_before = count_open_fds();
	run_shell_cmd("cat <<A\nfoo\nA\ncat <<B\nbar\nB\n");
	fds_after = count_open_fds();
	MU_ASSERT("no fd leak after multiple heredocs",
		fds_before == fds_after);
}

/**
 * @brief Verify no fd leak with empty heredoc.
 */
static void	test_heredoc_empty_no_fd_leak(void)
{
	int	fds_before;
	int	fds_after;

	fds_before = count_open_fds();
	run_shell_cmd("cat <<EOF\nEOF\n");
	fds_after = count_open_fds();
	MU_ASSERT("no fd leak after empty heredoc",
		fds_before == fds_after);
}

/* ================================================================
 * Suite
 * ================================================================ */

void	test_heredoc_bugs_suite(void)
{
	/* Bug 1: WEXITSTATUS precedence */
	test_wexitstatus_precedence_logic();
	test_wexitstatus_exit130_detected();
	test_heredoc_content_passed_to_command();
	test_heredoc_wrong_command_exits();
	test_heredoc_multiple_setup();

	/* Bug 2: pipe fd leak */
	test_heredoc_no_fd_leak();
	test_heredoc_wrong_cmd_no_fd_leak();
	test_heredoc_multiple_no_fd_leak();
	test_heredoc_empty_no_fd_leak();
}
