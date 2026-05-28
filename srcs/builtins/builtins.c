/**
 * @file builtins.c
 * @brief Builtin function registry and implementations.
 * @author pulgamecanica
 */

#include "42sh.h"
#include "builtins.h"
#include <stddef.h>

typedef struct s_builtin_entry
{
	const char		*name;
	t_builtin_fn	fn;
}	t_builtin_entry;

/**
 * @brief Builtin lookup table.
 */
static const t_builtin_entry	g_builtins[] = {
	{"history", builtin_history},
	{"echo", builtin_echo},
	{"cd", builtin_cd},
	{"pwd", builtin_pwd},
	{"jobs", builtin_jobs},
	{"fg", builtin_fg},
	{"bg", builtin_bg},
	{"exit", builtin_exit},
	{"type", builtin_type},
	{"set", builtin_set},
	{"unset", builtin_unset},
	{"export", builtin_export},
	{"hash", builtin_hash},
	{"alias", builtin_alias},
	{"unalias", builtin_unalias},
	{"test", builtin_test},
	{NULL, NULL}
};

/**
 * @param name Name of the builtin function to retrieve
 * @return Pointer to the builtin function, or NULL if not found
 */
t_builtin_fn	builtin_get(const char *name)
{
	int	i;

	if (!name)
		return (NULL);
	i = 0;
	while (g_builtins[i].name)
	{
		if (ft_strcmp(name, g_builtins[i].name) == 0)
			return (g_builtins[i].fn);
		i++;
	}
	return (NULL);
}

int	builtin_is_builtin(const char *name)
{
	return (builtin_get(name) != NULL);
}
