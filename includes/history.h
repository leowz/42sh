/**
 * @file history.h
 * @brief Collection of functions for managing the shell's command history.
 * @author pulgamecanica
 */

#ifndef HISTORY_H
#define HISTORY_H

typedef struct s_shell
  t_shell; /**> Forward declaration to avoid circular dependency with 42sh.h */

/**
 * @brief Initialize the history subsystem.
 */
void history_init(t_shell *shell);

/**
 * @brief Determine the path to the history file based on environment variables.
 */
char *history_file_path(void);

/**
 * @brief Load the command history from a file into readline's in-memory history
 * list.
 */
void history_load(const char *file_path);

/**
 * @brief Save the in-memory command history list to a file.
 */
void history_save(const char *file_path);

#endif
