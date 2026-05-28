/**
 * @file builtin_hash.c
 * @brief Implementation of the `hash` builtin command for 42sh.
 * @author pulgamecanica
 *
 * `hash` manages the shell's PATH-lookup cache (see command_hash.c).
 * Usage matches the common bash subset:
 *
 * | Form                 | Effect                                          |
 * |----------------------|-------------------------------------------------|
 * | `hash`               | Print cached entries (hits + path).             |
 * | `hash name [name..]` | Resolve each name now and cache it.             |
 * | `hash -r`            | Forget every cached entry.                      |
 * | `hash -d name`       | Forget a single entry.                          |
 * | `hash -p path name`  | Cache `name -> path` without consulting PATH.   |
 * | `hash -t name [..]`  | Print the cached path for each name.            |
 */

#include "42sh.h"
#include "builtins.h"
#include "executor.h"

/**
 * @brief Options accumulated from the leading flags. Mutually exclusive
 *        modes are tracked here so the dispatcher can pick one path.
 */
typedef struct s_hash_opts
{
	int	clear;
	int	delete_mode;
	int	preset_mode;
	int	type_mode;
	int	first_arg;
}	t_hash_opts;

/*
 * @brief Print a single (hits, path) line. Used as the per-entry
 *        callback for the no-arg listing.
 */
static void	print_entry(const char *key, t_cmd_hash_value *v, void *ud)
{
	(void)ud;
	if (!v || !v->path)
		return ;
	printf("%4zu\t%s\n", v->hits, v->path);
	(void)key;
}

/*
 * @brief Walk argv[start..end) and run `hash name` semantics on each.
 *        Failure to resolve any one name yields a non-zero overall exit.
 */
static int	hash_resolve_names(t_shell *shell, int argc, char **argv, int start)
{
	int		i;
	int		status;
	char	*path;

	status = 0;
	i = start;
	while (i < argc)
	{
		path = find_command(shell, argv[i]);
		if (!path)
		{
			ft_putstr_fd("42sh: hash: ", 2);
			ft_putstr_fd(argv[i], 2);
			ft_putendl_fd(": not found", 2);
			status = 1;
		}
		else
			free(path);
		i++;
	}
	return (status);
}

/*
 * @brief Handle `hash -d name [name ...]`.
 */
static int	hash_delete_names(t_shell *shell, int argc, char **argv, int start)
{
	int	i;
	int	status;

	if (start >= argc)
	{
		ft_putendl_fd("42sh: hash: -d: option requires an argument", 2);
		return (2);
	}
	status = 0;
	i = start;
	while (i < argc)
	{
		if (!cmd_hash_delete(shell, argv[i]))
		{
			ft_putstr_fd("42sh: hash: ", 2);
			ft_putstr_fd(argv[i], 2);
			ft_putendl_fd(": not found", 2);
			status = 1;
		}
		i++;
	}
	return (status);
}

/*
 * @brief Handle `hash -t name [name ...]`: print stored paths.
 */
static int	hash_print_paths(t_shell *shell, int argc, char **argv, int start)
{
	int					i;
	int					status;
	int					multi;
	t_cmd_hash_value	*v;

	if (start >= argc)
	{
		ft_putendl_fd("42sh: hash: -t: option requires an argument", 2);
		return (2);
	}
	status = 0;
	multi = (argc - start > 1);
	i = start;
	while (i < argc)
	{
		v = cmd_hash_get(shell, argv[i]);
		if (!v || !v->path)
		{
			ft_putstr_fd("42sh: hash: ", 2);
			ft_putstr_fd(argv[i], 2);
			ft_putendl_fd(": not found", 2);
			status = 1;
		}
		else if (multi)
			printf("%s\t%s\n", argv[i], v->path);
		else
			printf("%s\n", v->path);
		i++;
	}
	return (status);
}

/*
 * @brief Handle `hash -p path name`: insert without PATH lookup.
 */
static int	hash_preset(t_shell *shell, int argc, char **argv, int start)
{
	if (start + 2 != argc)
	{
		ft_putendl_fd("42sh: hash: -p: expects exactly 'path name'", 2);
		return (2);
	}
	if (!cmd_hash_set(shell, argv[start + 1], argv[start]))
	{
		ft_putendl_fd("42sh: hash: allocation failure", 2);
		return (1);
	}
	return (0);
}

/*
 * @brief Print the cached table (or a friendly note if empty).
 */
static int	hash_list(t_shell *shell)
{
	if (!shell->cmd_hash || ft_hash_size(shell->cmd_hash) == 0)
	{
		printf("hash: hash table empty\n");
		return (0);
	}
	printf("hits\tcommand\n");
	cmd_hash_iter(shell, print_entry, NULL);
	return (0);
}

/*
 * @brief Accept one short-flag character; bail with a diagnostic if it
 *        belongs to a different builtin's option set.
 */
static int	apply_flag(t_hash_opts *o, char c)
{
	if (c == 'r')
		o->clear = 1;
	else if (c == 'd')
		o->delete_mode = 1;
	else if (c == 'p')
		o->preset_mode = 1;
	else if (c == 't')
		o->type_mode = 1;
	else
	{
		ft_putstr_fd("42sh: hash: -", 2);
		ft_putchar_fd(c, 2);
		ft_putendl_fd(": invalid option", 2);
		ft_putendl_fd("hash: usage: hash [-r] [-p path] [-dt] [name ...]", 2);
		return (-1);
	}
	return (0);
}

/*
 * @brief Parse leading -x flags into `opts`, leaving `opts->first_arg`
 *        pointing at the first non-flag arg (or argc on no args).
 *        Stops at the first `--` or non-option token. Returns -1 on
 *        unknown flag (diagnostic already printed).
 */
static int	parse_opts(int argc, char **argv, t_hash_opts *opts)
{
	int	i;
	int	j;

	ft_bzero(opts, sizeof(*opts));
	i = 1;
	while (i < argc && argv[i] && argv[i][0] == '-' && argv[i][1])
	{
		if (ft_strcmp(argv[i], "--") == 0)
		{
			opts->first_arg = i + 1;
			return (0);
		}
		j = 1;
		while (argv[i][j])
		{
			if (apply_flag(opts, argv[i][j]) < 0)
				return (-1);
			j++;
		}
		i++;
	}
	opts->first_arg = i;
	return (0);
}

/*
 * @brief Reject combinations that don't fit any of the supported modes.
 *        Each mode-flag (-r, -d, -p, -t) is mutually exclusive with the
 *        others; everything else is fine.
 */
static int	check_mode_combo(const t_hash_opts *o)
{
	int	modes;

	modes = o->clear + o->delete_mode + o->preset_mode + o->type_mode;
	if (modes > 1)
	{
		ft_putendl_fd("42sh: hash: -r/-d/-p/-t are mutually exclusive", 2);
		return (-1);
	}
	return (0);
}

int	builtin_hash(t_shell *shell, int argc, char **argv)
{
	t_hash_opts	opts;
	int			status;

	if (parse_opts(argc, argv, &opts) < 0 || check_mode_combo(&opts) < 0)
	{
		shell->last_exit_status = 2;
		return (2);
	}
	if (opts.clear)
	{
		cmd_hash_clear(shell);
		if (opts.first_arg < argc)
			status = hash_resolve_names(shell, argc, argv, opts.first_arg);
		else
			status = 0;
	}
	else if (opts.delete_mode)
		status = hash_delete_names(shell, argc, argv, opts.first_arg);
	else if (opts.preset_mode)
		status = hash_preset(shell, argc, argv, opts.first_arg);
	else if (opts.type_mode)
		status = hash_print_paths(shell, argc, argv, opts.first_arg);
	else if (opts.first_arg < argc)
		status = hash_resolve_names(shell, argc, argv, opts.first_arg);
	else
		status = hash_list(shell);
	shell->last_exit_status = status;
	return (status);
}
