/**
 * @file builtin_cd.c
 * @brief Implementation of the cd builtin command for the 42sh shell.
 * @author jguillem
 */

#include "42sh.h"
#include "builtins.h"
#include "limits.h"

char	*readlink_malloc (char *filename)
{
	ssize_t	size;
	ssize_t	bytes;
	char	*buffer;

	size = 100;
	while (1)
	{
		buffer = malloc(sizeof(char) * size);
		bytes = readlink (filename, buffer, size);
		if (bytes < 0)
			return (NULL);
		if (bytes < size)
			return (buffer);
		free (buffer);
		if (size < 16384)
			size *= 2;
		else
			return (NULL);
	}
}

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
		tmp = malloc(sizeof(char) * old_path_len + 2);
		sprintf(tmp, "%s/", old_path);
	}
	new_path = malloc(sizeof(char) * (strlen(tmp) + strlen(relative) + 1));
	sprintf(new_path, "%s%s", tmp, relative);
	free(tmp);
	return (new_path);
}

static char	*concatenate(char *base, char *add)
{
	char	*concat;
	size_t	b = base ? strlen(base) : 0;
	size_t	a = add ? strlen(add) : 0;

	concat = malloc(sizeof(char) * (b + a + 2));
	concat = strcpy(concat, base);
	free(base);
	if (b > 1)
		concat = strcat(concat, "/");
	concat = strcat(concat, add);
	return (concat);
}

static void	resolve_path(char **path)
{
	char	*resolve = *path;
	char	*token;
	t_list	**tokens = malloc(sizeof(t_list *));
	t_list	*current;

	*tokens = NULL;
	token = strtok(resolve, "/");
	while (token)
	{
		if (!strcmp(token, ".."))
			ft_lstdellast(tokens, &free);
		else if (strcmp(token, "."))
			ft_lstappend(tokens, ft_lstnew(strdup(token)));
		token = strtok(NULL, "/");
	}
	free(*path);
	resolve = strdup("/");
	current = *tokens;
	while (current)
	{
		resolve = concatenate(resolve, (char *)current->content);
		current = current->next;
	}
	ft_lstdel(tokens, &free);
	free(tokens);
	*path = resolve;
}

static int	change_directory(char *target, int physical)
{
	char	*path = NULL;
	char	*directory;
	char	*oldpwd = NULL;
	char	*cwd;

	if (!target)
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
	}
	else
		directory = target;
	oldpwd = strdup(getenv("PWD") ? getenv("PWD") : "");
	if (physical)
		path = realpath(directory, NULL);
	else
	{
		if (directory[0] == '/')
			path = strdup(directory);
		else
			path = join_paths(oldpwd, directory);
	}
	resolve_path(&path);
	if (chdir(path) < 0)
	{
		fprintf(stderr, "-42sh: cd: %s: No such file or directory\n", path);
		free(path);
		free(oldpwd);
		return (1);
	}
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
			fprintf(stderr, "-42sh: cd: -%c: invalid option\n", option[i]);
			fprintf(stderr, "cd: usage: cd [-L|[-P [-e]]] [-@] [dir]\n");
			return (-1);
		}
	}
	return (physical);
}

int	builtin_cd(struct s_shell *shell, int argc, char **argv)
{
	(void)shell;
	(void)argc;
	int	physical = 0;

	++argv;
	if (*argv && **argv == '-' && strlen(*argv) > 1)
	{
		if ((physical = detect_option(*argv)) == -1)
		{
			shell->last_exit_status = 2;
			return (shell->last_exit_status);
		}
		argc--;
		++argv;
	}
	if (argc > 2)
	{
		fprintf(stderr, "-42sh: cd: too many arguments\n");
		shell->last_exit_status = 2;
		return (shell->last_exit_status);
	}
	shell->last_exit_status = change_directory(*argv, physical);
	return (shell->last_exit_status);
}
