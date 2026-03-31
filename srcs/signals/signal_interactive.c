#include "signals.h"
#include <stdio.h>
#include <unistd.h>
#include <readline/readline.h>

volatile sig_atomic_t	g_signal_received;

/**
 * @brief SIGINT handler for interactive mode (at prompt).
 *
 * Bash behavior: Ctrl-C abandons the current input line, prints a newline,
 * and redisplays a fresh prompt. The old text stays visible above.
 *
 * We use rl_replace_line + rl_on_new_line + rl_redisplay to tell readline
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
 * @brief Set up signal handlers for interactive mode (at the prompt).
 *
 * SIGINT  — custom handler: newline + redisplay prompt (like bash)
 * SIGQUIT — ignored (Ctrl-\ does nothing at prompt) (for now... I think)
 * SIGTSTP — ignored for now (Ctrl-Z does nothing at prompt, will handle
 *           fg/bg job control later)
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
}
