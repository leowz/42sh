#include "42sh.h"

/**
 * @brief Process any pending signal that was caught during the last prompt.
 *
 * Called in the main loop before each prompt iteration.
 * Sets $? to the appropriate value (130 for SIGINT = 128 + 2).
 */
void	signals_check(t_shell *shell)
{
	int	sig;

	sig = g_signal_received;
	g_signal_received = 0;
	if (sig == SIGINT)
		shell->last_exit_status = 130;
}
