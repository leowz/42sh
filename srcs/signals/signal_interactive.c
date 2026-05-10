#include "signals.h"
#include <stdio.h>
#include <unistd.h>
#include <readline/readline.h>

volatile sig_atomic_t	g_signal_received;

/**
 * @brief SIGINT handler for interactive mode (at prompt).
 *
 * @details Ctrl-C abandons the current input line, prints a newline,
 * and redisplays a fresh prompt. The old text stays visible above.
 *
 * @details We use rl_replace_line + rl_on_new_line + rl_redisplay to tell readline
 * to clear its internal buffer and show a new prompt on the next line.
 */
static void	sigint_handler_interactive(int sig)
{
	g_signal_received = sig;
	write(STDOUT_FILENO, "\n", 1);
	rl_replace_line("", 0);
	rl_on_new_line();
	rl_redisplay();
}

/**
 * @details SIGINT  - custom handler: newline + redisplay prompt (like bash)
 * @details SIGQUIT - ignored (Ctrl-\ does nothing at prompt)
 * @details SIGTSTP - ignored (Ctrl-Z at prompt is a no-op; children handle it)
 * @details SIGTTIN - ignored (shell never blocks reading from a bg tty)
 * @details SIGTTOU - ignored (tcsetpgrp/tcsetattr from shell must not self-stop)
 */
void	signals_setup_interactive(void)
{
	struct sigaction	sa;

	g_signal_received = 0;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = sigint_handler_interactive;
	sigaction(SIGINT, &sa, NULL);
	sa.sa_handler = SIG_IGN;
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGTSTP, &sa, NULL);
	sigaction(SIGTTIN, &sa, NULL);
	sigaction(SIGTTOU, &sa, NULL);
}
