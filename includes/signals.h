/**
 * @file signals.h
 * @brief Signal handling for interactive mode, command execution, and child processes.
 * @author pulgamecanica
 */

#ifndef SIGNALS_H
#define SIGNALS_H

#include <signal.h>

typedef struct s_shell
  t_shell; /**> Forward declaration to avoid circular dependency with 42sh.h */

/**
 * @brief Global signal flag (volatile for signal handler safety)
 * @note if you remember in the ::parser_parse function we pass the shell
 *       this is the variable which can help us detect heredoc signals!
 */
extern volatile sig_atomic_t g_signal_received;

void signals_setup_interactive(void);
void signals_setup_executing(void);
void signals_setup_child(void);
void signals_check(t_shell *shell);

#endif
