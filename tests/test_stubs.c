/**
 * @file test_stubs.c
 * @brief Functional stubs for modules the executor depends on.
 *
 * Provides minimal but working implementations of variables, builtins,
 * expander, and signals so that executor tests can link and run.
 * These stubs are only compiled into the test binary (42sh_test).
 */

#include "../includes/42sh.h"
#include "../includes/builtins.h"
#include "../includes/expander.h"
#include "../includes/executor.h"
#include <string.h>

/* ===== Global signal flag (extern in signals.h) ===== */

volatile sig_atomic_t	g_signal_received = 0;

/* ===== Variable system stubs =====
 * Uses shell->variables (t_list of t_var*) as a simple linked list.
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

/* ===== Signal stubs ===== */

void	signals_setup_interactive(void)
{
}

void	signals_setup_executing(void)
{
}

void	signals_setup_child(void)
{
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
}

void	signals_check(t_shell *shell)
{
	(void)shell;
}

/* ===== Expander stubs =====
 * expand_command is a no-op: leaves argv/assignments as-is.
 * This is fine for testing since test data is already "expanded".
 */

char	*expand_word(t_shell *shell, const char *word)
{
	(void)shell;
	return (ft_strdup(word));
}

char	**expand_word_to_fields(t_shell *shell, const char *word)
{
	char	**fields;

	(void)shell;
	fields = malloc(sizeof(char *) * 2);
	if (!fields)
		return (NULL);
	fields[0] = ft_strdup(word);
	fields[1] = NULL;
	return (fields);
}

int	expand_command(t_shell *shell, t_cmd *cmd)
{
	(void)shell;
	(void)cmd;
	return (0);
}

/* ===== Builtin stubs =====
 * A configurable test builtin: stub_builtin_fn is set by tests.
 * builtin_get returns it when name matches stub_builtin_name.
 */

static t_builtin_fn	g_stub_builtin_fn = NULL;
static const char	*g_stub_builtin_name = NULL;

void	stub_set_builtin(const char *name, t_builtin_fn fn)
{
	g_stub_builtin_name = name;
	g_stub_builtin_fn = fn;
}

t_builtin_fn	builtin_get(const char *name)
{
	if (g_stub_builtin_name && name && strcmp(name, g_stub_builtin_name) == 0)
		return (g_stub_builtin_fn);
	return (NULL);
}

int	builtin_is_builtin(const char *name)
{
	return (builtin_get(name) != NULL);
}

/* ===== Shell init/cleanup helpers for tests ===== */

void	stub_shell_init(t_shell *shell)
{
	memset(shell, 0, sizeof(t_shell));
	shell->running = 1;
	shell->interactive = 0;
	shell->last_exit_status = 0;
	shell->env_dirty = 1;
}

void	stub_shell_cleanup(t_shell *shell)
{
	t_list	*cur;
	t_list	*next;
	t_var	*v;

	cur = shell->variables;
	while (cur)
	{
		next = cur->next;
		v = LST_VAR(cur);
		free(v->name);
		free(v->value);
		free(v);
		free(cur);
		cur = next;
	}
	shell->variables = NULL;
	if (shell->env)
	{
		int i = 0;
		while (shell->env[i])
			free(shell->env[i++]);
		free(shell->env);
		shell->env = NULL;
	}
}
