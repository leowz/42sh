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
#include "../includes/signals.h"
#include <string.h>

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
