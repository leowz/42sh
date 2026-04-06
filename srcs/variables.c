#include "42sh.h"

/* ===== Variable system stubs =====
 * Uses shell->variables (t_list of t_var*) as a simple linked list.
 * But we could probably implement the b_tree
 */

t_var	*var_get(t_shell *shell, const char *name)
{
	t_list	*node;

	node = shell->variables;
	while (node)
	{
		if (strcmp(LST_VAR(node)->name, name) == 0)
			return (LST_VAR(node));
		node = node->next;
	}
	return (NULL);
}

char	*var_get_value(t_shell *shell, const char *name)
{
	t_var	*v;

	v = var_get(shell, name);
	if (v)
		return (v->value);
	return (NULL);
}

int	var_set(t_shell *shell, const char *name, const char *value)
{
	t_var	*v;
	t_var	*new_var;

	v = var_get(shell, name);
	if (v)
	{
		free(v->value);
		v->value = strdup(value);
		return (0);
	}
	new_var = malloc(sizeof(t_var));
	if (!new_var)
		return (-1);
	new_var->name = strdup(name);
	new_var->value = strdup(value);
	new_var->exported = 0;
	new_var->readonly = 0;
	ft_lstappend(&shell->variables, ft_lstnew(new_var));
	shell->env_dirty = 1;
	return (0);
}

int	var_unset(t_shell *shell, const char *name)
{
	t_list	*prev;
	t_list	*cur;
	t_var	*v;

	prev = NULL;
	cur = shell->variables;
	while (cur)
	{
		v = LST_VAR(cur);
		if (strcmp(v->name, name) == 0)
		{
			if (prev)
				prev->next = cur->next;
			else
				shell->variables = cur->next;
			free(v->name);
			free(v->value);
			free(v);
			free(cur);
			shell->env_dirty = 1;
			return (0);
		}
		prev = cur;
		cur = cur->next;
	}
	return (-1);
}

int	var_export(t_shell *shell, const char *name)
{
	t_var	*v;

	v = var_get(shell, name);
	if (v)
	{
		v->exported = 1;
		shell->env_dirty = 1;
		return (0);
	}
	return (-1);
}

/*
** Build a NULL-terminated envp array from exported variables.
** Simple implementation: allocates fresh each call (no caching).
*/
char	**var_get_environ(t_shell *shell)
{
	t_list	*node;
	int		count;
	char	**env;
	int		i;
	t_var	*v;
	char	*tmp;

	count = 0;
	node = shell->variables;
	while (node)
	{
		if (LST_VAR(node)->exported)
			count++;
		node = node->next;
	}
	env = malloc(sizeof(char *) * (count + 1));
	if (!env)
		return (NULL);
	i = 0;
	node = shell->variables;
	while (node)
	{
		v = LST_VAR(node);
		if (v->exported)
		{
			tmp = ft_strjoin(v->name, "=");
			env[i] = ft_strjoin(tmp, v->value);
			free(tmp);
			i++;
		}
		node = node->next;
	}
	env[i] = NULL;
	if (shell->env)
		free(shell->env);
	shell->env = env;
	shell->env_dirty = 0;
	return (env);
}

void	var_init_from_environ(t_shell *shell, char **envp)
{
	(void)shell;
	(void)envp;
}