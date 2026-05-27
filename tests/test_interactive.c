/**
 * @file test_interactive.c
 * @brief PTY-driven tests of 42sh's interactive behaviour.
 * @author zweng
 *
 * forkpty(3) gives 42sh a real controlling terminal, so it runs as a fully
 * interactive shell: prompt display, readline line editing, heredoc
 * continuation prompts and terminal signal handling. Each scenario types a
 * script -- always crafted to reach `exit` or send EOF so the shell
 * terminates and the capture ends cleanly -- then asserts on the captured
 * terminal session.
 *
 * Known interactive gaps versus the correction are recorded with
 * xfail_check: they are exercised and reported, but do not fail the suite,
 * mirroring the modules.sh scoreboard convention.
 */

#ifdef TEST_INTERACTIVE_ENABLED

# include "minunit.h"
# include <pty.h>
# include <unistd.h>
# include <poll.h>
# include <signal.h>
# include <sys/wait.h>
# include <string.h>
# include <stdio.h>

/**
 * @brief Run ./42sh under a pseudo-terminal, type @p input, and capture the
 *        whole terminal session (prompts, input echo, output) into @p out.
 * @details The read is bounded by poll(2); @p input must reach `exit` or
 *          send EOF so the shell exits and the read ends. The child is
 *          killed afterwards, so a misbehaving shell cannot hang the suite.
 */
static void	run_pty(const char *input, char *out, size_t outsz)
{
	int				master;
	pid_t			pid;
	struct pollfd	pfd;
	ssize_t			n;
	size_t			total;
	int				st;

	out[0] = '\0';
	fflush(stdout);
	pid = forkpty(&master, NULL, NULL, NULL);
	if (pid == -1)
		return ;
	if (pid == 0)
	{
		execl("./42sh", "42sh", (char *)NULL);
		_exit(127);
	}
	usleep(150000);
	write(master, input, strlen(input));
	pfd.fd = master;
	pfd.events = POLLIN;
	total = 0;
	while (total + 1 < outsz && poll(&pfd, 1, 5000) > 0)
	{
		n = read(master, out + total, outsz - 1 - total);
		if (n <= 0)
			break ;
		total += (size_t)n;
	}
	out[total] = '\0';
	close(master);
	kill(pid, SIGKILL);
	waitpid(pid, &st, 0);
}

/** @brief True if @p needle occurs anywhere in @p hay. */
static int	has(const char *hay, const char *needle)
{
	return (strstr(hay, needle) != NULL);
}

/**
 * @brief Report a known, accepted interactive gap. A still-failing check
 *        prints XFAIL; an unexpected pass prints XPASS. Neither affects the
 *        suite's pass/fail counters -- mirrors the modules.sh convention.
 */
static void	xfail_check(const char *what, int works)
{
	if (works)
		printf("  \033[1;33mXPASS\033[0m %s (gap now works - promote it)\n",
			what);
	else
		printf("  \033[1;33mXFAIL\033[0m %s (known gap, accepted)\n", what);
}

static void	test_interactive_prompt(void)
{
	char	out[8192];

	run_pty("exit\n", out, sizeof(out));
	MU_ASSERT("interactive shell displays a prompt", has(out, "42sh$ "));
}

static void	test_interactive_runs_command(void)
{
	char	out[8192];

	run_pty("echo HELLO_INTERACTIVE\nexit\n", out, sizeof(out));
	MU_ASSERT("a command typed at the prompt runs",
		has(out, "HELLO_INTERACTIVE"));
}

static void	test_interactive_heredoc(void)
{
	char	out[8192];

	run_pty("cat << EOF\nHEREDOC_BODY\nEOF\nexit\n", out, sizeof(out));
	MU_ASSERT("interactive heredoc shows a continuation prompt",
		has(out, "> "));
	MU_ASSERT("interactive heredoc delivers the body",
		has(out, "HEREDOC_BODY"));
}

static void	test_interactive_heredoc_multiline(void)
{
	char	out[8192];

	run_pty("cat << EOF\nLINE_ALPHA\nLINE_BETA\nEOF\nexit\n",
		out, sizeof(out));
	MU_ASSERT("multiline heredoc keeps the first line",
		has(out, "LINE_ALPHA"));
	MU_ASSERT("multiline heredoc keeps the second line",
		has(out, "LINE_BETA"));
}

static void	test_interactive_ctrl_d_exits(void)
{
	char	out[8192];

	run_pty("\004", out, sizeof(out));
	MU_ASSERT("Ctrl-D on an empty prompt exits the shell",
		has(out, "exit"));
}

static void	test_interactive_ctrl_c_survives(void)
{
	char	out[8192];

	run_pty("\003echo CTRLC_SURVIVED\nexit\n", out, sizeof(out));
	MU_ASSERT("the shell survives Ctrl-C and keeps running",
		has(out, "CTRLC_SURVIVED"));
}

static void	test_interactive_line_editing(void)
{
	char	out[8192];

	run_pty("echo aaQ\177bb\nexit\n", out, sizeof(out));
	MU_ASSERT("readline backspace edits the command line",
		has(out, "aabb"));
}

/* ---- known interactive gaps versus the correction (xfail) -------------- */

static void	test_interactive_quote_continuation(void)
{
	char	out[8192];

	run_pty("echo 'QC_OPEN\nQC_CLOSE'\nexit\n", out, sizeof(out));
	xfail_check("unclosed quote opens a continuation prompt",
		!has(out, "command not found"));
}

static void	test_interactive_backslash_continuation(void)
{
	char	out[8192];

	run_pty("echo BSL_\\\nCONT\nexit\n", out, sizeof(out));
	xfail_check("trailing backslash continues the line",
		!has(out, "command not found"));
}

void	test_interactive_suite(void)
{
	test_interactive_prompt();
	test_interactive_runs_command();
	test_interactive_heredoc();
	test_interactive_heredoc_multiline();
	test_interactive_ctrl_d_exits();
	test_interactive_ctrl_c_survives();
	test_interactive_line_editing();
	test_interactive_quote_continuation();
	test_interactive_backslash_continuation();
}

#else

void	test_interactive_suite(void)
{
}

#endif
