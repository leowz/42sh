/**
 * @file command_search.c
 * @brief Command search functionality for 42sh.
 * @author wengzhang, pulgamecanica
 */

#include "42sh.h"
#include "executor.h"

/**
 * @brief Build "dir/name" path. Caller must free the result.
 * @param dir The directory path.
 * @param name The file name.
 * @return The full path, or NULL if allocation fails.
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

/**
 * @brief Search each directory in PATH for an executable named `name`.
 * @param path_var The PATH environment variable.
 * @param name The command name.
 * @return Heap-allocated full path, or NULL if not found.
 */
static void	free_split(char **arr)
{
	int	i;

	i = 0;
	while (arr[i])
		free(arr[i++]);
	free(arr);
}

/**
 * @brief Search each directory in PATH for an executable named `name`.
 * @param path_var The PATH environment variable.
 * @param name The command name.
 * @return Heap-allocated full path, or NULL if not found.
 */
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

/**
 * @brief Consult the command-path cache before walking PATH.
 * @details A cached path that no longer exists (file deleted, PATH
 *          changed, etc.) falls through to a fresh PATH walk. The hit
 *          counter is bumped only when the cache actually satisfies the
 *          lookup, so `hash` reflects real usage rather than just
 *          insertions.
 */
static char	*lookup_cached(t_shell *shell, const char *name)
{
	t_cmd_hash_value	*v;

	v = cmd_hash_get(shell, name);
	if (!v || !v->path)
		return (NULL);
	if (access(v->path, X_OK) != 0)
		return (NULL);
	v->hits++;
	return (ft_strdup(v->path));
}

/**
 * @details - If name contains '/', treat it as a path directly.
 * @details - Otherwise consult the hash cache, then search $PATH and
 *           remember the result on hit so the next lookup is O(1).
 */
char	*find_command(t_shell *shell, const char *name)
{
	const char	*path_var;
	char		*found;

	if (!name || !*name)
		return (NULL);
	if (ft_strchr(name, '/'))
	{
		if (access(name, X_OK) == 0)
			return (ft_strdup(name));
		return (NULL);
	}
	found = lookup_cached(shell, name);
	if (found)
		return (found);
	path_var = var_get_value(shell, "PATH");
	if (!path_var)
		return (NULL);
	found = search_path(path_var, name);
	if (found)
	{
		cmd_hash_set(shell, name, found);
		cmd_hash_get(shell, name)->hits = 1;
	}
	return (found);
}

/**
 * @brief Print the diagnostic for an unrunnable command and return its
 *        exit status.
 * @details Distinguishes "exists but not executable" (126, permission
 *          denied) from "not found" (127). Only a name containing '/'
 *          can be probed for existence on disk; a bare name that PATH
 *          search failed to resolve is reported as not found.
 * @param name The command name as typed.
 * @return 126 if the path exists but is not executable, 127 otherwise.
 */
int	report_command_error(const char *name)
{
	ft_putstr_fd("42sh: ", 2);
	ft_putstr_fd(name, 2);
	if (ft_strchr(name, '/') && access(name, F_OK) == 0)
	{
		ft_putendl_fd(": Permission denied", 2);
		return (126);
	}
	ft_putendl_fd(": command not found", 2);
	return (127);
}
