/**
 * @file job_control_init.c
 * @brief Put the shell in its own process group and claim the terminal.
 * @author pulgamecanica
 *
 * Must run once, before the first foreground fork.  After this the shell is
 * the foreground pgrp on `terminal_fd`, so it can hand the terminal off to
 * child pgids with `tcsetpgrp` and take it back without self-signalling
 * (SIGTTOU is already ignored by `signals_setup_interactive`).
 */

#include "42sh.h"
#include "job_control.h"

int	job_control_init(t_shell *shell)
{
	if (!shell->interactive)
		return (0);
	while (tcgetpgrp(shell->terminal_fd) != (shell->shell_pgid = getpgrp()))
		kill(-shell->shell_pgid, SIGTTIN);
	shell->shell_pgid = getpid();
	if (setpgid(shell->shell_pgid, shell->shell_pgid) < 0
		&& errno != EPERM)
		return (1);
	tcsetpgrp(shell->terminal_fd, shell->shell_pgid);
	tcgetattr(shell->terminal_fd, &shell->original_termios);
	return (0);
}
