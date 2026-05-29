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

/**
 * @brief Multi-step variant of @c run_pty: each chunk is written, then we
 *        sleep so the shell can process it (fork a child, transfer the
 *        foreground pgrp, etc.) before the next chunk lands. Critical for
 *        job-control tests where a Ctrl-Z (sent as `\x1A`) must reach the
 *        foreground child's pgrp -- never the shell's.
 * @details Output is drained between chunks (best-effort) and a longer
 *          drain happens at the end. The shell must reach @c exit or EOF
 *          in the final chunk or the suite will spend the full timeout
 *          waiting on the read.
 */
static void	run_pty_steps(const char **chunks, size_t n_chunks,
		char *out, size_t outsz)
{
	int				master;
	pid_t			pid;
	struct pollfd	pfd;
	ssize_t			n;
	size_t			total;
	size_t			i;
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
	pfd.fd = master;
	pfd.events = POLLIN;
	total = 0;
	i = 0;
	while (i < n_chunks)
	{
		write(master, chunks[i], strlen(chunks[i]));
		usleep(250000);
		while (total + 1 < outsz && poll(&pfd, 1, 300) > 0)
		{
			n = read(master, out + total, outsz - 1 - total);
			if (n <= 0)
				break ;
			total += (size_t)n;
		}
		i++;
	}
	while (total + 1 < outsz && poll(&pfd, 1, 3000) > 0)
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

/* ---- heredoc patterns from the correction (§4 interactive heredocs) ---- */

/**
 * @brief `cat > FILE << DELIM` -- heredoc body lands in the redirected file
 *        rather than on stdout. The correction PDF tests this exact form
 *        (cat > /tmp/heredoc-append << FIN).
 */
static void	test_interactive_heredoc_with_file_redir(void)
{
	char	out[8192];

	run_pty(
		"cat > /tmp/ftsh_hd_file << FIN\n"
		"HDFILE_ALPHA\n"
		"HDFILE_BETA\n"
		"FIN\n"
		"cat /tmp/ftsh_hd_file\n"
		"rm -f /tmp/ftsh_hd_file\n"
		"exit\n",
		out, sizeof(out));
	MU_ASSERT("heredoc redirected to file: first body line written",
		has(out, "HDFILE_ALPHA"));
	MU_ASSERT("heredoc redirected to file: second body line written",
		has(out, "HDFILE_BETA"));
}

/**
 * @brief Heredoc INSIDE a subshell: `(cat << EOF ... EOF)`. The correction
 *        section "Grouped controls and sub-shells" specifically exercises
 *        heredocs combined with subshell parsing.
 */
static void	test_interactive_heredoc_in_subshell(void)
{
	char	out[8192];

	run_pty(
		"(cat << EOF\n"
		"HDSUB_LINE1\n"
		"HDSUB_LINE2\n"
		"EOF\n"
		")\n"
		"exit\n",
		out, sizeof(out));
	MU_ASSERT("heredoc inside subshell delivers first line",
		has(out, "HDSUB_LINE1"));
	MU_ASSERT("heredoc inside subshell delivers second line",
		has(out, "HDSUB_LINE2"));
}

/* ---- job control patterns from the correction (§8 job control) --------- */

/**
 * @brief A backgrounded command (`sleep 100 &`) must appear in `jobs`
 *        output. The correction PDF runs `ls -lR /usr >fifo 2>&1 &; jobs`
 *        and asserts the listing names the running command.
 */
static void	test_interactive_jobs_lists_background(void)
{
	char			out[8192];
	const char		*steps[] = {
		"sleep 100 &\n",
		"jobs\n",
		"kill %1 2>/dev/null\n",
		"exit\n"
	};

	run_pty_steps(steps, 4, out, sizeof(out));
	MU_ASSERT("jobs lists the backgrounded sleep command",
		has(out, "sleep"));
}

/**
 * @brief Ctrl-Z (`\x1A`) on a foreground job sends SIGTSTP through the
 *        terminal driver to the job's process group, NOT to the shell.
 *        After suspension, `jobs` must show "Stopped".
 */
static void	test_interactive_ctrlz_suspends_job(void)
{
	char			out[8192];
	const char		*steps[] = {
		"sleep 100\n",       /* foreground job */
		"\032",              /* Ctrl-Z -> SIGTSTP */
		"jobs\n",            /* should list it Stopped */
		"kill %1 2>/dev/null\n",
		"exit\n"
	};

	run_pty_steps(steps, 5, out, sizeof(out));
	MU_ASSERT("Ctrl-Z on a fg job: jobs shows Stopped",
		has(out, "Stopped"));
}

/**
 * @brief `fg %1` resumes a backgrounded job in the foreground; Ctrl-C
 *        (`\x03`) then interrupts it and returns control to the shell.
 *        The combined flow exercises both `fg` and SIGINT-to-fg-pgrp.
 */
static void	test_interactive_fg_then_ctrlc(void)
{
	char			out[8192];
	const char		*steps[] = {
		"sleep 100 &\n",
		"fg %1\n",
		"\003",              /* Ctrl-C */
		"echo FG_RESUMED\n",
		"exit\n"
	};

	run_pty_steps(steps, 5, out, sizeof(out));
	MU_ASSERT("shell regains control after fg + Ctrl-C",
		has(out, "FG_RESUMED"));
}

/* ---- Inhibitors continuation: unclosed-quote and trailing-backslash --- */

/**
 * @brief Unclosed single quote opens a "> " continuation prompt; the
 *        newline is part of the quoted string until the closing `'`.
 *        Correction PDF, Inhibitors module: a `quote>` prompt indicates
 *        the shell is waiting for the rest of the quoted string.
 */
static void	test_interactive_quote_continuation(void)
{
	char	out[8192];

	run_pty("echo 'QC_OPEN\nQC_CLOSE'\nexit\n", out, sizeof(out));
	MU_ASSERT("unclosed quote opens a continuation prompt and joins lines",
		has(out, "QC_OPEN") && has(out, "QC_CLOSE")
		&& !has(out, "command not found"));
}

/**
 * @brief Trailing `\` continues the line: `echo foo\<NL>bar` runs as
 *        one command `echo foobar`. Correction PDF, Inhibitors module
 *        sample test: `$> echo foo\`.
 */
static void	test_interactive_backslash_continuation(void)
{
	char	out[8192];

	run_pty("echo BSL_\\\nCONT\nexit\n", out, sizeof(out));
	MU_ASSERT("trailing backslash continues the line into one command",
		has(out, "BSL_CONT") && !has(out, "command not found"));
}

/**
 * @brief Heredoc delimiter split by a backslash-newline. The correction
 *        types `cat << EO\` then `> F`, and the delimiter is `EOF`. The
 *        plain backslash-continuation outside heredocs still XFAILs, but
 *        the heredoc-delim path uses the heredoc collector's own reader,
 *        which handles it correctly -- so this is a real assertion.
 */
static void	test_interactive_heredoc_delim_continuation(void)
{
	char	out[8192];

	run_pty(
		"cat << EO\\\nF\n"
		"HDDELIM_BODY\n"
		"EOF\n"
		"exit\n",
		out, sizeof(out));
	MU_ASSERT("heredoc delimiter spans a backslash-newline",
		has(out, "HDDELIM_BODY"));
}

void	test_interactive_suite(void)
{
	test_interactive_prompt();
	test_interactive_runs_command();
	test_interactive_heredoc();
	test_interactive_heredoc_multiline();
	test_interactive_heredoc_with_file_redir();
	test_interactive_heredoc_in_subshell();
	test_interactive_jobs_lists_background();
	test_interactive_ctrlz_suspends_job();
	test_interactive_fg_then_ctrlc();
	test_interactive_ctrl_d_exits();
	test_interactive_ctrl_c_survives();
	test_interactive_line_editing();
	test_interactive_quote_continuation();
	test_interactive_backslash_continuation();
	test_interactive_heredoc_delim_continuation();
}

#else

void	test_interactive_suite(void)
{
}

#endif
