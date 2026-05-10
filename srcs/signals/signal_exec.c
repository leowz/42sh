/**
 * @file signal_exec.c
 * @brief Signal handling for execution context.
 * @author pulgamecanica wengzhang
 */

#include "signals.h"
#include <stddef.h>

/**
 * @details SIGINT  - ignored (let signal reach the child process)
 * @details SIGQUIT - ignored (let signal reach the child process)
 * @details SIGTSTP - ignored (let signal reach the child process)
 *
 * @note The parent simply waits; the child handles or dies from the signal.
 */
void	signals_setup_executing(void)
{
	struct sigaction	sa;

	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sa.sa_handler = SIG_IGN;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGTSTP, &sa, NULL);
}
