/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   command_search.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: wengzhang <marvin@42.fr>                   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/27 00:00:00 by wengzhang         #+#    #+#             */
/*   Updated: 2026/03/27 00:00:00 by wengzhang        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "42sh.h"
#include "executor.h"

/*
** Build "dir/name" path. Caller must free the result.
*/
static char	*build_path(const char *dir, const char *name)
{
	char	*tmp;
	char	*full;

	tmp = ft_strjoin(dir, "/");
	if (!tmp)
		return (NULL);
	full = ft_strjoin(tmp, name);
	free(tmp);
	return (full);
}

/*
** Search each directory in PATH for an executable named `name`.
** Returns heap-allocated full path, or NULL if not found.
*/
static void	free_split(char **arr)
{
	int	i;

	i = 0;
	while (arr[i])
		free(arr[i++]);
	free(arr);
}

static char	*search_path(const char *path_var, const char *name)
{
	char	**dirs;
	char	*full;
	int		i;

	dirs = ft_strsplit(path_var, ':');
	if (!dirs)
		return (NULL);
	i = 0;
	while (dirs[i])
	{
		full = build_path(dirs[i], name);
		if (full && access(full, X_OK) == 0)
		{
			free_split(dirs);
			return (full);
		}
		free(full);
		i++;
	}
	free_split(dirs);
	return (NULL);
}

/*
** Find an executable command.
** - If name contains '/', treat it as a path directly.
** - Otherwise search each directory in $PATH.
** Returns heap-allocated path on success, NULL on failure.
*/
char	*find_command(t_shell *shell, const char *name)
{
	const char	*path_var;

	if (!name || !*name)
		return (NULL);
	if (ft_strchr(name, '/'))
	{
		if (access(name, X_OK) == 0)
			return (ft_strdup(name));
		return (NULL);
	}
	path_var = var_get_value(shell, "PATH");
	if (!path_var)
		return (NULL);
	return (search_path(path_var, name));
}
