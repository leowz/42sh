/**
 * @file builtin.c
 * @brief Built-in command handling for 42sh.
 * @author pulgamecanica
 */

#include "42sh.h"
#include "builtins.h"

// !IMPORTANT
// THIS MUST BE CHANGED, only STUBS for testing purposes, later will change the tests

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