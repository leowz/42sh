/**
 * @file history.c
 * @brief Command history management using GNU Readline.
 *
 * This module provides functions to initialize the command history subsystem,
 * determine the history file path, load history from a file, and save history to a file.
 *
 * The history file is determined by checking the $HISTFILE environment variable first,
 * and if that is not set, falling back to $HOME/.sh_history. The history is loaded
 * into memory at shell startup and saved back to disk on shell exit.
 *
 * @author pulgamecanica
 */

#include "42sh.h"

/**
 * @details This sets up readline's history handling and loads the history file if available.
 */
void history_init(t_shell *shell)
{
	rl_readline_name = "42sh";
	using_history();
	shell->history_file = history_file_path();
	history_load(shell->history_file);
#ifdef FT_EXTRA_VERBOSE
	fprintf(stderr, "  \033[2mDebug → History initialized with file '%s'.\033[0m\n",
		shell->history_file ? shell->history_file : "none");
#endif
}

/**
 * @details Follows POSIX-compliant lookup order: first checks $HISTFILE,
 *          then falls back to $HOME/.sh_history. Returns a malloc'd string that the caller must free.
 */
char	*history_file_path(void)
{
	char	*histfile;
	char	*home;

	histfile = getenv("HISTFILE");
	if (histfile && *histfile)
		return (ft_strdup(histfile));
	home = getenv("HOME");
	if (!home || !*home)
		return (NULL);
	return (ft_strjoin(home, "/.42sh_history"));
}

/**
 * @details This function uses readline's `read_history()` to populate the history list.
 *          If `file_path` is NULL, this function does nothing.
 */
void	history_load(const char *file_path)
{
	if (!file_path)
		return ;
	read_history(file_path);
}

/**
 * @details This function uses readline's `write_history()` to write the history list to disk.
 *          If `file_path` is NULL, this function does nothing.
 */
void	history_save(const char *file_path)
{
	if (!file_path)
		return ;
	write_history(file_path);
#ifdef FT_EXTRA_VERBOSE
	fprintf(stderr, "  \033[2mDebug → History saved to file '%s'.\033[0m\n",
		file_path);
#endif
}
