/**
 * @file aliases.c
 * @brief Alias table storage: create, look up and remove shell aliases.
 * @author zweng
 *
 * Aliases live on `t_shell.aliases` as a `t_list*` chain of `t_alias*`,
 * mirroring the variables module. This file owns every mutator.
 */
#include "42sh.h"
#include "aliases.h"
#include <stdlib.h>

/**
 * @brief Free a `t_alias` and both of its heap-owned strings.
 */
static void	alias_destroy(t_alias *alias)
{
	if (!alias)
		return ;
	free(alias->name);
	free(alias->value);
	free(alias);
}

t_alias	*alias_get(t_shell *shell, const char *name)
{
	t_list	*node;
	t_alias	*alias;

	if (!shell || !name)
		return (NULL);
	node = shell->aliases;
	while (node)
	{
		alias = LST_ALIAS(node);
		if (alias && alias->name && ft_strequ(alias->name, name))
			return (alias);
		node = node->next;
	}
	return (NULL);
}

char	*alias_get_value(t_shell *shell, const char *name)
{
	t_alias	*alias;

	alias = alias_get(shell, name);
	if (!alias)
		return (NULL);
	return (alias->value);
}

/**
 * @brief Allocate a fresh `t_alias` with heap copies of name and value.
 * @return New node, or NULL on allocation failure.
 */
static t_alias	*alias_new(const char *name, const char *value)
{
	t_alias	*alias;

	alias = malloc(sizeof(t_alias));
	if (!alias)
		return (NULL);
	alias->name = ft_strdup(name);
	alias->value = ft_strdup(value);
	if (!alias->name || !alias->value)
	{
		free(alias->name);
		free(alias->value);
		free(alias);
		return (NULL);
	}
	return (alias);
}

int	alias_set(t_shell *shell, const char *name, const char *value)
{
	t_alias	*existing;
	t_alias	*created;
	t_list	*node;
	char	*dup;

	if (!shell || !name || !value)
		return (1);
	existing = alias_get(shell, name);
	if (existing)
	{
		dup = ft_strdup(value);
		if (!dup)
			return (1);
		free(existing->value);
		existing->value = dup;
		return (0);
	}
	created = alias_new(name, value);
	if (!created)
		return (1);
	node = ft_lstnew(created);
	if (!node)
		return (alias_destroy(created), 1);
	ft_lstadd(&shell->aliases, node);
	return (0);
}

int	alias_unset(t_shell *shell, const char *name)
{
	t_list	*node;
	t_list	*prev;
	t_alias	*alias;

	if (!shell || !name)
		return (1);
	node = shell->aliases;
	prev = NULL;
	while (node)
	{
		alias = LST_ALIAS(node);
		if (alias && alias->name && ft_strequ(alias->name, name))
		{
			if (prev)
				prev->next = node->next;
			else
				shell->aliases = node->next;
			alias_destroy(alias);
			free(node);
			return (0);
		}
		prev = node;
		node = node->next;
	}
	return (1);
}

void	alias_clear(t_shell *shell)
{
	t_list	*node;
	t_list	*next;

	if (!shell)
		return ;
	node = shell->aliases;
	while (node)
	{
		next = node->next;
		alias_destroy(LST_ALIAS(node));
		free(node);
		node = next;
	}
	shell->aliases = NULL;
}
