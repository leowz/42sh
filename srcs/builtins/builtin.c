/**
 * @file builtin.c
 * @brief main file of the builtin module
 * @author jguillem
 */

#include "42sh.h"
#include "builtins.h"

static const struct { const char *name; t_builtin_fn fn; } g_builtins[] = {
	{"cd",	builtin_echo},
	{"echo",	builtin_echo},
	{NULL, NULL}
};

t_builtin_fn	builtin_get(const char *name)
{
	int	i = 0;
	while (g_builtins[i].name && strcmp(name, g_builtins[i].name) != 0)
		i++;
	return (g_builtins[i].fn);
}
