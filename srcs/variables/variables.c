/**
 * @file variables.c
 * @brief Shell variable storage and lookup.
 * @author wengzhang, pulgamecanica
 *
 * Variables live in @c shell->variables as a singly-linked t_list of
 * t_var pointers (one node per name).  The cached @c shell->env array
 * is rebuilt lazily by @c var_get_environ when @c shell->env_dirty
 * flips on after a write.
 *
 * This module owns both: every t_var name/value pair is freshly
 * allocated, and every entry of @c shell->env is heap-owned too.
 * @c var_init_from_environ copies from the inherited envp so the
 * caller can keep the original char** untouched.
 */

#include "42sh.h"
#include "variables.h"
#include <stdlib.h>

static int	is_valid_identifier(const char *name)
{
	size_t	i;

	if (!name || !*name)
		return (0);
	if (!ft_isalpha((unsigned char)name[0]) && name[0] != '_')
		return (0);
	i = 1;
	while (name[i])
	{
		if (!ft_isalnum((unsigned char)name[i]) && name[i] != '_')
			return (0);
		i++;
	}
	return (1);
}

static void	var_destroy(t_var *var)
{
	if (!var)
		return ;
	free(var->name);
	free(var->value);
	free(var);
}

t_var	*var_get(t_shell *shell, const char *name)
{
	t_list	*node;
	t_var	*var;

	if (!shell || !name)
		return (NULL);
	node = shell->variables;
	while (node)
	{
		var = LST_VAR(node);
		if (var && var->name && ft_strequ(var->name, name))
			return (var);
		node = node->next;
	}
	return (NULL);
}

char	*var_get_value(t_shell *shell, const char *name)
{
	t_var	*var;

	var = var_get(shell, name);
	if (!var)
		return (NULL);
	return (var->value);
}

/**
 * @brief Allocate a fresh t_var with copies of name and (optional) value.
 * @return New t_var or NULL on allocation failure.
 */
static t_var	*var_new(const char *name, const char *value)
{
	t_var	*var;

	var = malloc(sizeof(t_var));
	if (!var)
		return (NULL);
	var->name = ft_strdup(name);
	if (!var->name)
		return (free(var), NULL);
	var->value = NULL;
	if (value)
	{
		var->value = ft_strdup(value);
		if (!var->value)
			return (free(var->name), free(var), NULL);
	}
	var->exported = 0;
	var->readonly = 0;
	return (var);
}

int	var_set(t_shell *shell, const char *name, const char *value)
{
	t_var	*existing;
	t_var	*created;
	t_list	*node;
	char	*new_value;

	if (!shell || !is_valid_identifier(name))
		return (1);
	existing = var_get(shell, name);
	if (existing)
	{
		if (existing->readonly)
			return (1);
		new_value = NULL;
		if (value)
			new_value = ft_strdup(value);
		if (value && !new_value)
			return (1);
		free(existing->value);
		existing->value = new_value;
		shell->env_dirty = 1;
		return (0);
	}
	created = var_new(name, value);
	if (!created)
		return (1);
	node = ft_lstnew(created);
	if (!node)
		return (var_destroy(created), 1);
	ft_lstadd(&shell->variables, node);
	shell->env_dirty = 1;
	return (0);
}

int	var_unset(t_shell *shell, const char *name)
{
	t_list	*node;
	t_list	*prev;
	t_var	*var;

	if (!shell || !name)
		return (1);
	node = shell->variables;
	prev = NULL;
	while (node)
	{
		var = LST_VAR(node);
		if (var && var->name && ft_strequ(var->name, name))
		{
			if (var->readonly)
				return (1);
			if (prev)
				prev->next = node->next;
			else
				shell->variables = node->next;
			var_destroy(var);
			free(node);
			shell->env_dirty = 1;
			return (0);
		}
		prev = node;
		node = node->next;
	}
	return (0);
}

int	var_export(t_shell *shell, const char *name)
{
	t_var	*var;

	if (!shell || !is_valid_identifier(name))
		return (1);
	var = var_get(shell, name);
	if (!var)
	{
		if (var_set(shell, name, NULL) != 0)
			return (1);
		var = var_get(shell, name);
		if (!var)
			return (1);
	}
	var->exported = 1;
	shell->env_dirty = 1;
	return (0);
}

/**
 * @brief Free a NULL-terminated env array (each string + the array).
 */
static void	free_env_array(char **env)
{
	int	i;

	if (!env)
		return ;
	i = 0;
	while (env[i])
		free(env[i++]);
	free(env);
}

/**
 * @brief Build a single "NAME=VALUE" string for execve.
 */
static char	*env_entry(const t_var *var)
{
	char	*name_eq;
	char	*entry;

	name_eq = ft_strjoin(var->name, "=");
	if (!name_eq)
		return (NULL);
	entry = ft_strjoin(name_eq, var->value ? var->value : "");
	free(name_eq);
	return (entry);
}

/**
 * @brief Count exported variables that are eligible for the env array.
 * @details A NULL value still counts (POSIX exports remember unset state),
 *          but we render those as "NAME=" so execve sees a real string.
 */
static size_t	count_exported(t_shell *shell)
{
	t_list	*node;
	t_var	*var;
	size_t	count;

	count = 0;
	node = shell->variables;
	while (node)
	{
		var = LST_VAR(node);
		if (var && var->exported)
			count++;
		node = node->next;
	}
	return (count);
}

char	**var_get_environ(t_shell *shell)
{
	char	**env;
	size_t	count;
	size_t	i;
	t_list	*node;
	t_var	*var;

	if (!shell)
		return (NULL);
	if (!shell->env_dirty && shell->env)
		return (shell->env);
	free_env_array(shell->env);
	count = count_exported(shell);
	env = malloc(sizeof(char *) * (count + 1));
	if (!env)
		return (shell->env = NULL);
	i = 0;
	node = shell->variables;
	while (node && i < count)
	{
		var = LST_VAR(node);
		if (var && var->exported)
		{
			env[i] = env_entry(var);
			if (!env[i])
				return (free_env_array(env), shell->env = NULL);
			i++;
		}
		node = node->next;
	}
	env[i] = NULL;
	shell->env = env;
	shell->env_dirty = 0;
	return (env);
}

void	var_init_from_environ(t_shell *shell, char **envp)
{
	char	*eq;
	char	*name;
	int		i;

	if (!shell || !envp)
		return ;
	i = 0;
	while (envp[i])
	{
		eq = ft_strchr(envp[i], '=');
		if (eq && eq != envp[i])
		{
			name = ft_strsub(envp[i], 0, eq - envp[i]);
			if (name)
			{
				if (var_set(shell, name, eq + 1) == 0)
					var_export(shell, name);
				free(name);
			}
		}
		i++;
	}
}
