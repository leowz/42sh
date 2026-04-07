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
 * @details Add new builtins here as they are implemented.
 */
static const t_builtin_entry	g_builtins[] = {
	{"history", builtin_history},
	{"echo", builtin_echo},
	{NULL, NULL}
};

/**
 * @brief Look up a builtin by name.
 * @param name The name of the builtin to look up.
 * @return The function pointer or NULL if not found.
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

/**
 * @brief Check if a name is a builtin.
 * @param name The name to check.
 * @return 1 if the name is a builtin, 0 otherwise.
 */
int	builtin_is_builtin(const char *name)
{
	return (builtin_get(name) != NULL);
}
