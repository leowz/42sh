/**
 * @file signal_child.c
 * @brief Signal handling setup for child processes in 42sh.
 * @author wengzhang, pulgamecanica
*/

#include "signals.h"
#include <stddef.h>

/**
 * @details After fork the child inherits the parent's signal dispositions (SIG_IGN
 *          for interactive mode).  We must reset them to SIG_DFL so the executed
 *          program responds to signals normally.
 */
void	signals_setup_child(void)
{
	struct sigaction	sa;

	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = SIG_DFL;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGTSTP, &sa, NULL);
	sigaction(SIGTTIN, &sa, NULL);
	sigaction(SIGTTOU, &sa, NULL);
	sigaction(SIGPIPE, &sa, NULL);
}

/**
 * @details A subshell (`( ... )`) keeps running shell code after fork — it will
 *          call tcsetpgrp/tcsetattr when launching its own children, so it must
 *          keep SIGTTOU/SIGTTIN ignored (inherited from the parent shell) or it
 *          self-stops the moment it restores the terminal. Job-control signals
 *          for the *user* (SIGINT/SIGQUIT/SIGTSTP) and SIGPIPE go back to default.
 */
void	signals_setup_subshell(void)
{
	struct sigaction	sa;

	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = SIG_DFL;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGTSTP, &sa, NULL);
	sigaction(SIGPIPE, &sa, NULL);
}
