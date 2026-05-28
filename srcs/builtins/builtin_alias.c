/**
 * @file builtin_alias.c
 * @brief Implementation of the `alias` builtin command.
 * @author zweng
 *
 * Defines aliases (`name=value`), queries a single alias (`name`), and
 * lists every alias sorted by name (no arguments).
 */
#include "42sh.h"
#include "builtins.h"
#include "aliases.h"
#include <stdlib.h>

/**
 * @brief Print one alias as `alias NAME='VALUE'`, single-quoting VALUE
 *        and rendering each embedded `'` as the POSIX `'\''` sequence.
 */
static void	print_one_alias(const char *name, const char *value)
{
	ft_putstr_fd("alias ", 1);
	ft_putstr_fd((char *)name, 1);
	ft_putstr_fd("='", 1);
	while (*value)
	{
		if (*value == '\'')
			ft_putstr_fd("'\\''", 1);
		else
			ft_putchar_fd(*value, 1);
		value++;
	}
	ft_putendl_fd("'", 1);
}

/**
 * @brief qsort comparator: order two `t_alias *` by their name.
 */
static int	cmp_alias(const void *a, const void *b)
{
	return (ft_strcmp((*(t_alias **)a)->name, (*(t_alias **)b)->name));
}

/**
 * @brief List every alias, sorted by name (matches bash `alias`).
 * @return 0 on success, 1 on allocation failure.
 */
static int	print_all_aliases(t_shell *shell)
{
	t_list	*node;
	t_alias	**arr;
	int		n;
	int		i;

	n = ft_lstsize(shell->aliases);
	if (n == 0)
		return (0);
	arr = malloc(sizeof(t_alias *) * n);
	if (!arr)
		return (1);
	node = shell->aliases;
	i = 0;
	while (node)
	{
		arr[i++] = LST_ALIAS(node);
		node = node->next;
	}
	qsort(arr, n, sizeof(t_alias *), cmp_alias);
	i = -1;
	while (++i < n)
		print_one_alias(arr[i]->name, arr[i]->value);
	free(arr);
	return (0);
}

/**
 * @brief True if @p name is a valid alias name.
 * @details Accepts non-empty names whose first character is a letter or
 *          `_`, and whose remaining characters are letters, digits, `_`,
 *          or one of the POSIX-allowed punctuation marks (`!`, `%`, `,`,
 *          `@`). Rejects empty, leading-digit, and anything containing
 *          `/`, `-`, `=`, whitespace or other shell metacharacters --
 *          which is exactly the set the correction PDF flags as invalid
 *          (`=`, `-`, `/`).
 */
static int	is_valid_alias_name(const char *name)
{
	size_t	i;

	if (!name || !*name)
		return (0);
	if (!ft_isalpha((unsigned char)name[0]) && name[0] != '_')
		return (0);
	i = 1;
	while (name[i])
	{
		if (ft_isalnum((unsigned char)name[i]) || name[i] == '_')
		{
			i++;
			continue ;
		}
		if (name[i] == '!' || name[i] == '%'
			|| name[i] == ',' || name[i] == '@')
		{
			i++;
			continue ;
		}
		return (0);
	}
	return (1);
}

/**
 * @brief Print `42sh: alias: NAME: invalid alias name` on stderr.
 */
static void	report_invalid_name(const char *name)
{
	ft_putstr_fd("42sh: alias: `", 2);
	ft_putstr_fd((char *)name, 2);
	ft_putendl_fd("': invalid alias name", 2);
}

/**
 * @brief Handle one `alias` argument: a NAME=VALUE definition or a NAME
 *        query. The split is at the first `=`; a leading `=` is treated
 *        as a (failing) query, not an empty-name definition.
 * @return 0 on success, 1 if a queried name is not an alias.
 */
static int	alias_arg(t_shell *shell, char *arg)
{
	char	*eq;
	char	*name;
	char	*value;
	int		rc;

	eq = ft_strchr(arg, '=');
	if (eq && eq != arg)
	{
		name = ft_strsub(arg, 0, eq - arg);
		if (!name)
			return (1);
		if (!is_valid_alias_name(name))
		{
			report_invalid_name(name);
			free(name);
			return (1);
		}
		rc = alias_set(shell, name, eq + 1);
		free(name);
		return (rc);
	}
	if (!is_valid_alias_name(arg))
	{
		report_invalid_name(arg);
		return (1);
	}
	value = alias_get_value(shell, arg);
	if (value)
		return (print_one_alias(arg, value), 0);
	ft_putstr_fd("42sh: alias: ", 2);
	ft_putstr_fd(arg, 2);
	ft_putendl_fd(": not found", 2);
	return (1);
}

int	builtin_alias(struct s_shell *shell, int argc, char **argv)
{
	int	i;
	int	result;

	if (!shell || !argv)
		return (1);
	result = 0;
	if (argc == 1)
		result = print_all_aliases(shell);
	i = 1;
	while (i < argc)
	{
		if (alias_arg(shell, argv[i]) != 0)
			result = 1;
		i++;
	}
	shell->last_exit_status = result;
	return (result);
}
