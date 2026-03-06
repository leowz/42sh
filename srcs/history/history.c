/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   history.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pulgamecanica <pulgamecanica@student.42.fr> +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/26 00:00:00 by pulgamecanica    #+#    #+#             */
/*   Updated: 2026/02/26 00:00:00 by pulgamecanica    ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "42sh.h"

/**
 * @brief Initialize the history subsystem.
 * @details This sets up readline's history handling and loads the history file if available.
 * @param shell The main shell state struct, used to store the history file path.
 */
void history_init(t_shell *shell)
{
	rl_readline_name = "42sh";
	using_history();
	shell->history_file = history_file_path();
	history_load(shell->history_file);
#ifdef FT_EXTRA_VERBOSE
	fprintf(stderr, "  \033[2mDebug → History initialized with file '%s'.\033[0m\n", shell->history_file ? shell->history_file : "none");
#endif
}

/**
 * @brief Determine the path to the history file based on environment variables.
 * @details Follows POSIX-compliant lookup order: first checks $HISTFILE,
 * then falls back to $HOME/.sh_history. Returns a malloc'd string that the caller must free.
 * @returns A malloc'd string with the history file path, or NULL if neither variable is set or valid.
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
 * @brief Load the command history from a file into readline's in-memory history list.
 * @details This function uses readline's `read_history()` to populate the history list.
 * If `file_path` is NULL, this function does nothing.
 * @param file_path The path to the history file to load, or NULL to skip loading.
 */
void	history_load(const char *file_path)
{
	if (!file_path)
		return ;
	read_history(file_path);
}

/**
 * @brief Save the in-memory command history list to a file.
 * @details This function uses readline's `write_history()` to write the history list to disk.
 * If `file_path` is NULL, this function does nothing.
 * @param file_path The path to the history file to save, or NULL to skip saving.
 */
void	history_save(const char *file_path)
{
	if (!file_path)
		return ;
	write_history(file_path);
#ifdef FT_EXTRA_VERBOSE
	fprintf(stderr, "  \033[2mDebug → History saved to file '%s'.\033[0m\n", file_path);
#endif
}
