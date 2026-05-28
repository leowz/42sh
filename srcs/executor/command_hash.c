/**
 * @file command_hash.c
 * @brief PATH-lookup cache backing both `find_command` and the `hash` builtin.
 * @author pulgamecanica
 *
 * Wraps a generic `t_hash` from libft so the rest of the shell only sees
 * `t_cmd_hash_value` (path + hits). The libft table owns the keys; this
 * file owns each value's `path` allocation and the value struct itself.
 */

#include "42sh.h"
#include "executor.h"

/**
 * @brief Free a value previously allocated by cmd_hash_set().
 * @details Used as the `del` callback for ft_hash_clear / ft_hash_destroy
 *          so the table can release values without knowing their shape.
 */
static void	free_value(void *p)
{
	t_cmd_hash_value	*v;

	v = (t_cmd_hash_value *)p;
	if (!v)
		return ;
	free(v->path);
	free(v);
}

void	cmd_hash_init(t_shell *shell)
{
	if (!shell || shell->cmd_hash)
		return ;
	shell->cmd_hash = ft_hash_new(CMD_HASH_BUCKETS);
}

void	cmd_hash_clear(t_shell *shell)
{
	if (!shell || !shell->cmd_hash)
		return ;
	ft_hash_clear(shell->cmd_hash, free_value);
}

void	cmd_hash_destroy(t_shell *shell)
{
	if (!shell || !shell->cmd_hash)
		return ;
	ft_hash_destroy(shell->cmd_hash, free_value);
	shell->cmd_hash = NULL;
}

t_cmd_hash_value	*cmd_hash_get(t_shell *shell, const char *name)
{
	if (!shell || !shell->cmd_hash)
		return (NULL);
	return ((t_cmd_hash_value *)ft_hash_get(shell->cmd_hash, name));
}
