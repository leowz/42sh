 /**
 * @file builtin_cd.c
 * @brief Implementation of the cd builtin command for the 42sh shell.
 * @author jguillem
 */

#include "42sh.h"
#include "builtins.h"
#include "limits.h"

/**
 * @param old_path : the base path
 * @param relative : the relative path to append
 * @brief Join a base path with a relative path, ensuring proper '/' separation
 * @return The newly allocated combined path
 */
static char	*join_paths(char *old_path, char *relative)
{
	char	*tmp;
	char	*new_path;
	size_t	old_path_len;

	if (!old_path)
		return (strdup(relative));
	old_path_len = strlen(old_path);
	if (old_path[old_path_len - 1] == '/')
		tmp = strdup(old_path);
	else
	{
		tmp = malloc(sizeof(char) * (old_path_len + 2));
		snprintf(tmp, old_path_len + 2, "%s/", old_path);
	}
	new_path = malloc(sizeof(char) * (strlen(tmp) + strlen(relative) + 1));
	sprintf(new_path, "%s%s", tmp, relative);
	free(tmp);
	return (new_path);
}

/**
 * @param base : the base string
 * @param add : the string to append
 * @brief Concatenate two paths, inserting '/' when needed and freeing base
 * @return The resulting concatenated string
 */
static char	*concatenate(char *base, char *add)
{
	char	*concat;
	size_t	b = base ? strlen(base) : 0;
	size_t	a = add ? strlen(add) : 0;

	concat = malloc(sizeof(char) * (b + a + 2));
	if (!concat)
		return (NULL);
	if (base)
	{
		concat = strcpy(concat, base);
		free(base);
	}
	if (b > 1)
		concat = strcat(concat, "/");
	if (add)
		concat = strcat(concat, add);
	return (concat);
}

/**
 * @param path : pointer to the path string to resolve
 * @brief Normalize a path by resolving '.', '..' and redundant separators
 * @return 0 on success, 1 if an error occurs (e.g. path too long)
 */
static int	resolve_path(char **path)
{
	char	*resolve = *path;
	char	*token;
	t_list	*tokens; 
	t_list	*current;

	tokens = NULL;
	token = strtok(resolve, "/");
	while (token)
	{
		if (strlen(token) >= _POSIX_PATH_MAX)
		{
			ft_lstdel(&tokens, &free);
			free(*path);
			*path = NULL;
			return (1);
		}
		if (!strcmp(token, ".."))
			ft_lstdellast(&tokens, &free);
		else if (strcmp(token, "."))
			ft_lstappend(&tokens, ft_lstnew(strdup(token)));
		token = strtok(NULL, "/");
	}
	free(*path);
	resolve = strdup("/");
	current = tokens;
	while (current)
	{
		resolve = concatenate(resolve, (char *)current->content);
		current = current->next;
	}
	ft_lstdel(&tokens, &free);
	*path = resolve;
	return (0);
}

/**
 * @param directory : the directory argument provided by the user
 * @param path : the resolved path
 * @param oldpwd : the previous working directory
 * @param msg : the error message to display
 * @brief Print a cd error message and free allocated resources
 * @return 1 (failure)
 */
static int	access_failure(char *directory, char *path, char *oldpwd, char *msg)
{
	fprintf(stderr, "42sh: cd: %s: %s\n", directory, msg);
	free(path);
	free(oldpwd);
	return (1);
}

/**
 * @param directory : the directory argument provided by the user
 * @param path : the resolved path
 * @param oldpwd : the previous working directory
 * @brief Handle No such file or directory error
 * @return 1 (failure)
 */
static int	no_such_file_or_directory(char *directory, char *path, char *oldpwd)
{
	return (access_failure(directory, path, oldpwd, "No such file or directory"));
}

/**
 * @param directory : the directory argument provided by the user
 * @param path : the resolved path
 * @param oldpwd : the previous working directory
 * @brief Handle Not a directory error
 * @return 1 (failure)
 */
static int	not_a_directory(char *directory, char *path, char *oldpwd)
{
	return (access_failure(directory, path, oldpwd, "Not a directory"));
}

/**
 * @param directory : the directory argument provided by the user
 * @param path : the resolved path
 * @param oldpwd : the previous working directory
 * @brief Handle Permission denied error
 * @return 1 (failure)
 */
static int	permission_denied(char *directory, char *path, char *oldpwd)
{
	return (access_failure(directory, path, oldpwd, "Permission denied"));
}

/**
 * @param directory : the directory argument provided by the user
 * @param path : the resolved path
 * @param oldpwd : the previous working directory
 * @brief Handle File name too long error
 * @return 1 (failure)
 */
static int	filename_too_long(char *directory, char *path, char *oldpwd)
{
	return (access_failure(directory, path, oldpwd, "File name too long"));
}

/**
 * @param directory : the directory argument provided by the user
 * @param path : the resolved path
 * @param oldpwd : the previous working directory
 * @brief Validate the path (length, type, permissions)
 * @return 0 if valid, 1 if an error occurs
 */
static int	is_invalid_path(char *directory, char *path, char *oldpwd)
{
	struct stat	sb;

	if (strlen(path) >= PATH_MAX)
		return (filename_too_long(directory, path, oldpwd));
	if (stat(path, &sb) != 0)
		return (no_such_file_or_directory(directory, path, oldpwd));
	if (!S_ISDIR(sb.st_mode))
		return (not_a_directory(directory, path, oldpwd));
	if (access(path, X_OK) != 0)
		return (permission_denied(directory, path, oldpwd));
	return (0);
}

/**
 * @param target : the target directory argument
 * @param argc : number of remaining arguments
 * @param physical : flag indicating physical (-P) or logical (-L) resolution
 * @brief Resolve and change the current working directory, updating PWD/OLDPWD
 * @return 0 on success, 1 on failure
 */
static int	change_directory(char *target, int argc, int physical)
{
	char	*path = NULL;
	char	*directory;
	char	*oldpwd = NULL;
	char	*cwd;

	if (!target || argc == 0)
	{
		directory = getenv("HOME");
		if (!directory)
		{
			fprintf(stderr, "42sh: cd: HOME not set\n");
			return (1);
		}
	}
	else if (!strcmp(target, "-"))
	{
		oldpwd = getenv("OLDPWD");
		if (!oldpwd)
		{
			fprintf(stderr, "42sh: cd: OLDPWD not set\n");
			return (1);
		}
		directory = oldpwd;
		printf("%s\n", directory);
	}
	else
		directory = target;
	oldpwd = strdup(getenv("PWD") ? getenv("PWD") : "");
	if (physical)
	{
		path = realpath(directory, NULL);
		if (!path)
			return (no_such_file_or_directory(directory, NULL, oldpwd));
	}
	else
	{
		if (directory[0] == '/')
			path = strdup(directory);
		else
			path = join_paths(oldpwd, directory);
	}
	if (resolve_path(&path))
		return (filename_too_long(directory, path, oldpwd));
	if (is_invalid_path(directory, path, oldpwd))
		return (1);
	if (chdir(path) < 0)
		return (no_such_file_or_directory(directory, path, oldpwd));
	else
	{
		setenv("OLDPWD", oldpwd, 1);
		if (physical)
		{
			cwd = getcwd(NULL, 0);
			setenv("PWD", cwd, 1);
			free(cwd);
		}
		else
			setenv("PWD", path, 1);
		free(path);
		free(oldpwd);
		return (0);
	}
}

/**
 * @param option : a string from *argv
 * @brief detect if the string correspond to an option
 * @return 0 | 1
 */
static int	detect_option(char *option)
{
	int	physical = 0;
	int	i = 0;

	while (option[++i])
	{
		if (option[i] == 'P')
			physical = 1;
		else if (option[i] == 'L')
			physical = 0;
		else
		{
			fprintf(stderr, "42sh: cd: -%c: invalid option\n", option[i]);
			fprintf(stderr, "cd: usage: cd [-L|-P] [dir]\n");
			return (-1);
		}
	}
	return (physical);
}

/**
 * @param shell : a pointer on the s_shell struct
 * @param argc : the number of tokens of the command
 * @param argv : the tokens of the command
 * @brief This is the POSIX cd command
 * @return 0
 */
int	builtin_cd(struct s_shell *shell, int argc, char **argv)
{
	int	physical = 0;

	++argv;
	argc--;
	while (*argv && **argv == '-' && strlen(*argv) > 1)
	{
		if (!strcmp(*argv, "--"))
		{
			argc--;
			argv++;
			break;
		}
		if ((physical = detect_option(*argv)) == -1)
		{
			shell->last_exit_status = 2;
			return (shell->last_exit_status);
		}
		argc--;
		++argv;
	}
	if (argc > 1)
	{
		fprintf(stderr, "42sh: cd: too many arguments\n");
		shell->last_exit_status = 2;
		return (shell->last_exit_status);
	}
	shell->last_exit_status = change_directory(*argv, argc, physical);
	return (shell->last_exit_status);
}
