/**
 * @file signals.h
 * @brief Signal handling for the 42sh shell
 * @author pulgamecanica
 */

#ifndef SIGNALS_H
# define SIGNALS_H

# include <signal.h>

/**
 * @brief Global signal flag (volatile for signal handler safety)
*/
extern volatile sig_atomic_t	g_signal_received;

void							signals_setup_interactive(void);
void							signals_setup_executing(void);
void							signals_setup_child(void);

#endif
