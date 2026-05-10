/**
 * @file builtins.h
 * @brief Header for builtin function registry and declarations.
 * @details Defines the t_builtin_fn type and declares builtin functions.
 *          Also includes the builtin lookup function.
 * @author zweng, jguillem, pulgamecanica
 */

#ifndef BUILTINS_H
#define BUILTINS_H

#include "42sh.h"

/** Forward declaration to avoid circular dependency with 42sh.h */
typedef struct s_shell t_shell;

/**
 * @brief Builtin function type.
 * @details Every builtin shares this signature. `argv[0]` is the builtin
 *          name itself, `argv[argc]` is `NULL`. Return value is the exit
 *          status the shell should observe.
 * @param shell Pointer to the central shell state, for accessing variables,
 *              jobs, etc.
 * @param argc  Argument count (number of elements in `argv`).
 * @param argv  Argument vector (`argv[0]` is the command name).
 * @return Exit status code (0 for success, non-zero for failure).
 */
typedef int (*t_builtin_fn)(struct s_shell *shell, int argc, char **argv);

/**
 * @brief Look up a builtin function by name.
 * @return Pointer to the builtin function, or `NULL` if `name` is not a
 *         registered builtin.
 */
t_builtin_fn builtin_get(const char *name);

/**
 * @brief Test whether `name` is a registered builtin.
 * @return Non-zero if `name` is a builtin, 0 otherwise.
 */
int builtin_is_builtin(const char *name);

/**
 * @brief Print arguments separated by spaces, followed by a newline.
 * @return Always 0.
 *
 * **Usage**: `echo [-neE] [arg ...]`
 *
 * Options may be combined (e.g. `-nE`). Unknown letters disable option
 * parsing for that token, so `-x` is printed literally.
 *
 * | Option | Effect                                                |
 * |--------|-------------------------------------------------------|
 * | `-n`   | Suppress the trailing newline.                        |
 * | `-e`   | Interpret backslash escapes (`\n`, `\t`, `\xNN`, ...).|
 * | `-E`   | Disable backslash interpretation (default).           |
 *
 * **Examples**:
 * @code{.sh}
 * echo hello world         # hello world
 * echo -n no-newline       # no-newline
 * echo -e 'a\tb\nc'        # a<TAB>b<NEWLINE>c
 * echo -E 'literal\\n'     # literal\n
 * @endcode
 */
int builtin_echo(struct s_shell *shell, int argc, char **argv);

/**
 * @brief Change the shell's current working directory.
 * @return 0 on success, 1 on filesystem error (no such directory, not a
 *         directory, permission denied, name too long, `HOME`/`OLDPWD`
 *         unset), 2 on usage error (invalid option, too many arguments).
 *
 * **Usage**: `cd [-L|-P] [--] [dir]`
 *
 * With no argument, changes to `$HOME`. The argument `-` switches to the
 * previous directory (`$OLDPWD`) and prints the new path. `--` ends
 * option parsing so a directory literally named `-foo` can be reached.
 *
 * | Option | Effect                                                  |
 * |--------|---------------------------------------------------------|
 * | `-L`   | Logical: keep symlinks in `PWD` (default).              |
 * | `-P`   | Physical: resolve symlinks via `realpath`.              |
 *
 * After a successful change, `OLDPWD` is set to the previous `PWD` and
 * `PWD` is updated.
 *
 * **Examples**:
 * @code{.sh}
 * cd                       # go to $HOME
 * cd /tmp                  # absolute path
 * cd ..                    # parent directory
 * cd -                     # back to previous directory, prints it
 * cd -P /var/run           # follow symlinks
 * cd -- -weird-dir-name    # directory whose name starts with '-'
 * @endcode
 */
int builtin_cd(struct s_shell *shell, int argc, char **argv);

/**
 * @brief Exit the shell.
 * @return On success the shell terminates and never returns to the
 *         caller. On usage error: 2 (non-numeric argument) or 1 (more
 *         than one argument — the shell is *not* exited in this case).
 *
 * **Usage**: `exit [n]`
 *
 * If `n` is supplied, the shell exits with status `n & 0xFF` (POSIX
 * truncation to 8 bits). With no argument, exits with the status of the
 * most recently executed command (`$?`).
 *
 * **Examples**:
 * @code{.sh}
 * exit                     # exit with $?
 * exit 0                   # exit success
 * exit 42                  # exit with status 42
 * exit -1                  # exit with status 255 (-1 & 0xFF)
 * exit foo                 # error: "numeric argument required", exit 2
 * exit 1 2                 # error: "too many arguments", returns 1
 * @endcode
 */
int builtin_exit(struct s_shell *shell, int argc, char **argv);

/**
 * @brief Describe how each name would be interpreted as a command.
 * @return 0 if every name was identified as a builtin or external
 *         command, 1 if any name was not found.
 *
 * **Usage**: `type name [name ...]`
 *
 * For each name, prints one of:
 *   - `name is a shell builtin` — name is registered with `builtin_get`.
 *   - `name is /path/to/name`   — first match found by walking `$PATH`.
 *   - `42sh: type: name: not found` (on stderr) — neither.
 *
 * **Examples**:
 * @code{.sh}
 * type cd ls bogus
 * # cd is a shell builtin
 * # ls is /usr/bin/ls
 * # bogus not found
 * @endcode
 */
int builtin_type(struct s_shell *shell, int argc, char **argv);

/**
 * @brief Set or export shell variables. (Stub - not implemented.)
 * @return 0 on success, non-zero on usage error.
 */
int builtin_export(struct s_shell *shell, int argc, char **argv);

/**
 * @brief Remove shell variables. (Stub - not implemented.)
 * @return 0 on success, non-zero on usage error.
 */
int builtin_unset(struct s_shell *shell, int argc, char **argv);

/**
 * @brief Set shell options or positional parameters. (Stub - not implemented.)
 * @return 0 on success, non-zero on usage error.
 */
int builtin_set(struct s_shell *shell, int argc, char **argv);

/**
 * @brief List active jobs known to the shell.
 * @return Always 0.
 *
 * **Usage**: `jobs`
 *
 * Output format, one line per job:
 * @code{.txt}
 * [<id>]  <marker> <status>\t<command-line>
 * @endcode
 *
 * `<marker>` is `+` for the current job and a space otherwise. `<status>`
 * is `Running`, `Stopped`, or `Done`. Listing a job clears its pending
 * notification flag.
 *
 * **Example**:
 * @code{.sh}
 * sleep 100 &
 * sleep 200 &
 * jobs
 * # [1]    Running        sleep 100
 * # [2]  + Running        sleep 200
 * @endcode
 */
int builtin_jobs(struct s_shell *shell, int argc, char **argv);

/**
 * @brief Resume a job in the foreground.
 * @return Exit status of the resumed job, or 1 if no matching job exists.
 *
 * **Usage**: `fg [job_spec]`
 *
 * Brings the selected job to the foreground, prints its command line,
 * and waits for it to finish or stop. With no argument, acts on the
 * current job (`%+`).
 *
 * `job_spec` accepts:
 *
 * | Spec       | Meaning                                            |
 * |------------|----------------------------------------------------|
 * | (omitted)  | The current job.                                   |
 * | `%`, `%%`, `%+` | The current job.                              |
 * | `%-`       | The previous current job.                          |
 * | `%<n>`     | Job whose id is `<n>` (the number in `[ ]`).       |
 * | `%<str>`   | Most recent job whose command line begins with `<str>`. |
 *
 * **Examples**:
 * @code{.sh}
 * sleep 100 &              # [1] 12345
 * fg %1                    # resume job 1 in foreground
 * fg                       # resume the current job
 * fg %sleep                # resume job whose command begins with "sleep"
 * @endcode
 */
int builtin_fg(struct s_shell *shell, int argc, char **argv);

/**
 * @brief Resume a stopped job in the background.
 * @return 0 on success, 1 if no matching job exists.
 *
 * **Usage**: `bg [job_spec]`
 *
 * Sends `SIGCONT` to the selected job and lets it continue without
 * occupying the terminal. The job spec syntax is identical to `fg`
 * (see ::builtin_fg). With no argument, acts on the current job.
 *
 * **Examples**:
 * @code{.sh}
 * sleep 100                # ... press Ctrl-Z
 * # [1]+  Stopped          sleep 100
 * bg                       # resume in background
 * bg %1                    # same, by job id
 * @endcode
 */
int builtin_bg(struct s_shell *shell, int argc, char **argv);

/**
 * @brief Print the command history.
 * @return Always 0.
 *
 * **Usage**: `history [n]`
 *
 * With no argument, prints every entry currently held by readline. With
 * a positive integer `n`, prints only the last `n` entries. Non-positive
 * `n` prints nothing. Each line is `<index>  <command>` where `<index>`
 * starts at `history_base` (typically 1).
 *
 * **Examples**:
 * @code{.sh}
 * history                  # print full history
 * history 5                # print last 5 commands
 * @endcode
 */
int builtin_history(struct s_shell *shell, int argc, char **argv);

#endif
