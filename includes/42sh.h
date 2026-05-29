/**
 * @file 42sh.h
 * @brief Header for the 42sh shell, includes all sub-systems and
 *        defines the central `t_shell` state struct.
 * @author wengzhang, jguillem, jspitz, pulgamecanica, zweng
 */

#ifndef SHELL_42SH_H
#define SHELL_42SH_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <readline/history.h>
#include <readline/readline.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <term.h>
#include <termios.h>
#include <unistd.h>
#include "aliases.h"
#include "history.h"
#include "job_control.h"
#include "libft.h"
#include "signals.h"
#include "variables.h"

/** Casts a `t_list` node's content to `t_job *`. */
#define LST_JOB(n) ((t_job *)(n)->content)

/** Casts a `t_list` node's content to `t_process *`. */
#define LST_PROC(n) ((t_process *)(n)->content)

/**
 * @brief Central runtime state of the shell - one instance lives in `main()`.
 * @details This struct holds all global state, including variables, jobs,
 *          history file path, and terminal settings.
 */
typedef struct s_shell {
  t_list *aliases;         /**< `t_alias*` list; alias table. */
  t_hash *cmd_hash;        /**< Cached PATH lookups for the `hash` builtin. */
  char   *cmd_entrypoint;  /**< `./42sh -c <cmd>` command string, or NULL. */
  t_job  *current_job;     /**< Most recent job (`%+` / `%%`). */
  char   **env;            /**< Cached NULL-terminated array for execve. */
  int    env_dirty;        /**< 1 when `env` needs rebuild before next execve. */
  int    exit_confirmed;   /**< Double-exit guard when stopped jobs exist. */
  char   *history_file;    /**< Path from $HISTFILE or $HOME/.sh_history. */
  t_list *heredoc_body_queue; /**< Pre-collected heredoc bodies (FIFO of `char *`); populated by `shell_read_logical_line`, drained by the heredoc collector. NULL outside REPL pre-collection. */
  int    interactive;      /**< 1 if stdin is a TTY (prompt + readline active). */
  t_list *jobs;            /**< `t_job*` list; all known jobs. */
  int    last_exit_status; /**< Value of `$?`. */
  struct termios
         original_termios; /**< Saved terminal attributes, restored on exit. */
  int    running;          /**< Main loop flag; set to 0 to exit. */
  pid_t  shell_pgid;       /**< Shell's own process group id. */
  int    terminal_fd;      /**< File descriptor of the controlling terminal. */
  t_list *variables;       /**< `t_var*` list; all shell/env variables. */
} t_shell;

/**
 * @brief Read one line of shell input — the single owner of stdin.
 * @details readline in interactive mode, getline on the shared `stdin`
 *          FILE* otherwise. The REPL and the heredoc collector both call
 *          this so command lines and heredoc bodies stay synchronised.
 * @return Heap-allocated line without trailing newline, or NULL on EOF.
 */
char	*shell_read_line(t_shell *shell, const char *prompt);

/**
 * @brief Read a logical line, joining physical lines on unclosed quotes or
 *        trailing-backslash line-continuation (REPL only, not heredoc).
 * @return Heap-allocated buffer, or NULL on EOF before any input.
 */
char	*shell_read_logical_line(t_shell *shell, const char *primary);

#endif
