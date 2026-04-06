// /* ************************************************************************** */
// /*                                                                            */
// /*                                                        :::      ::::::::   */
// /*   builtins.c                                       :+:      :+:    :+:   */
// /*                                                    +:+ +:+         +:+     */
// /*   By: wengzhang <marvin@42.fr>                   +#+  +:+       +#+        */
// /*                                                +#+#+#+#+#+   +#+           */
// /*   Created: 2026/04/06 00:00:00 by wengzhang         #+#    #+#             */
// /*   Updated: 2026/04/06 00:00:00 by wengzhang        ###   ########.fr       */
// /*                                                                            */
// /* ************************************************************************** */

// #include "42sh.h"
// #include "builtins.h"
// #include <stddef.h>

// typedef struct s_builtin_entry
// {
// 	const char		*name;
// 	t_builtin_fn	fn;
// }	t_builtin_entry;

// /*
// ** Builtin lookup table.
// ** Add new builtins here as they are implemented.
// */
// static const t_builtin_entry	g_builtins[] = {
// 	{"history", builtin_history},
// 	{NULL, NULL}
// };

// /*
// ** Look up a builtin by name.  Returns the function pointer or NULL.
// */
// t_builtin_fn	builtin_get(const char *name)
// {
// 	int	i;

// 	if (!name)
// 		return (NULL);
// 	i = 0;
// 	while (g_builtins[i].name)
// 	{
// 		if (ft_strcmp(name, g_builtins[i].name) == 0)
// 			return (g_builtins[i].fn);
// 		i++;
// 	}
// 	return (NULL);
// }

// /*
// ** Return 1 if name is a builtin, 0 otherwise.
// */
// int	builtin_is_builtin(const char *name)
// {
// 	return (builtin_get(name) != NULL);
// }
