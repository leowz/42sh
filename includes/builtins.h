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
 * @brief Print the current working directory.
 * @return 0 on success, 1 on getcwd failure or usage error.
 *
 * **Usage**: `pwd [-L|-P]`
 *
 * | Option | Effect                                                  |
 * |--------|---------------------------------------------------------|
 * | `-L`   | Logical: print `$PWD` (default).                        |
 * | `-P`   | Physical: print `getcwd(3)` output (symlinks resolved). |
 *
 * The default `-L` form is what makes `cd /bin; pwd` report `/bin` even
 * when `/bin` is a symlink to `/usr/bin` — the shell tracks the logical
 * path the user typed, not what the kernel resolved.
 */
int builtin_pwd(struct s_shell *shell, int argc, char **argv);

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
 * @return 0 if every name was identified, 1 if any name was not found,
 *         2 on an invalid option.
 *
 * **Usage**: `type [-tpa] name [name ...]`
 *
 * | Option | Effect                                                       |
 * |--------|--------------------------------------------------------------|
 * | (none) | `name is a shell builtin` / `name is /path` / `name not found` |
 * | `-t`   | Print one word: `builtin` or `file` (nothing if not found).  |
 * | `-p`   | Print the `$PATH` resolution of a disk command only.         |
 * | `-a`   | Print the builtin entry and every `$PATH` match.             |
 *
 * Options may be combined (`-ta`); `--` ends option parsing. An unknown
 * option is reported on stderr and yields exit status 2.
 *
 * **Examples**:
 * @code{.sh}
 * type cd ls bogus
 * # cd is a shell builtin
 * # ls is /usr/bin/ls
 * # bogus not found
 * type -t ls        # file
 * type -p ls        # /usr/bin/ls
 * @endcode
 */
int builtin_type(struct s_shell *shell, int argc, char **argv);

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

/**
 * @brief The set builtin command.
 *
 * **Usage**: `set [NAME=VALUE ...]`
 *
 * - With no arguments, displays all variables in `NAME=VALUE` format.
 * - Each `NAME=VALUE` argument sets a variable.
 *
 * **Examples**:
 * @code{.sh}
 * set                  # display all variables
 * set var1=value1      # set a variable
 * set x=10 y=hello     # set multiple variables
 * @endcode
 *
 * @param shell Pointer to the shell state
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, 1 on error
 */
int	builtin_set(struct s_shell *shell, int argc, char **argv);


/**
 * @brief The unset builtin command.
 *
 * **Usage**: `unset name [name ...]`
 *
 * Removes one or more variables by name. Each argument should be a valid
 * variable name (identifier). If a name is not found, unset succeeds silently
 * (POSIX behavior).
 *
 * **Examples**:
 * @code{.sh}
 * unset var1           # remove a variable
 * unset x y z          # remove multiple variables
 * unset nonexistent    # succeeds silently
 * @endcode
 *
 * @param shell Pointer to the shell state
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, 1 on error (invalid identifier)
 */
int	builtin_unset(struct s_shell *shell, int argc, char **argv);

/**
 * @brief The export builtin command.
 *
 * **Usage**: `export [name] [name=value ...]`
 *
 * - With no arguments, displays all exported variables in exportable format.
 * - With `name` (identifier), marks the variable for export to child processes.
 * - With `NAME=VALUE` argument, sets the variable and marks it for export.
 *
 * **Examples**:
 * @code{.sh}
 * export                       # display all exported variables
 * export PATH                  # export existing PATH variable
 * export var1=value1           # set and export a variable
 * export x=10 MYVAR=hello      # set and export multiple variables
 * @endcode
 *
 * @param shell Pointer to the shell state
 * @param argc Argument count
 * @param argv Argument vector
 * @return 0 on success, 1 on error
 */
int	builtin_export(struct s_shell *shell, int argc, char **argv);

/**
 * @brief The alias builtin command.
 *
 * **Usage**: `alias [name[=value] ...]`
 *
 * - With no arguments, lists every alias as `alias name='value'`, sorted
 *   by name, with embedded single quotes escaped as `'\''`.
 * - With `name=value`, defines or redefines an alias.
 * - With `name`, prints that alias, or reports it as not found.
 *
 * @return 0 on success, 1 if a queried name is not an alias.
 */
int	builtin_alias(struct s_shell *shell, int argc, char **argv);

/**
 * @brief The unalias builtin command.
 *
 * **Usage**: `unalias [-a] name [name ...]`
 *
 * - `-a` removes every alias.
 * - Each `name` removes one alias; an unknown name is reported on stderr.
 *
 * @return 0 on success, 1 if a name was not an alias or on usage error.
 */
int	builtin_unalias(struct s_shell *shell, int argc, char **argv);

#endif
