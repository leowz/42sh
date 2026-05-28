/**
 * @file command_hash_ops.c
 * @brief Mutating operations for the command-path hash cache.
 * @author pulgamecanica
 */

#include "42sh.h"
#include "executor.h"

/**
 * @brief Build a fresh value carrying a duplicated `path` and zero hits.
 *        Returns NULL (without leaking) on any allocation failure.
 */
static t_cmd_hash_value	*value_new(const char *path)
{
	t_cmd_hash_value	*v;

	v = malloc(sizeof(t_cmd_hash_value));
	if (!v)
		return (NULL);
	v->path = ft_strdup(path);
	if (!v->path)
	{
		free(v);
		return (NULL);
	}
	v->hits = 0;
	return (v);
}

/**
 * @details If `name` is already cached, we keep its `hits` counter and
 *          only swap the path - this is what lets `hash -p` change the
 *          binding without resetting usage history.
 */
int	cmd_hash_set(t_shell *shell, const char *name, const char *path)
{
	t_cmd_hash_value	*existing;
	char				*dup;

	if (!shell || !name || !path)
		return (0);
	cmd_hash_init(shell);
	if (!shell->cmd_hash)
		return (0);
	existing = (t_cmd_hash_value *)ft_hash_get(shell->cmd_hash, name);
	if (existing)
	{
		dup = ft_strdup(path);
		if (!dup)
			return (0);
		free(existing->path);
		existing->path = dup;
		return (1);
	}
	existing = value_new(path);
	if (!existing)
		return (0);
	if (!ft_hash_set(shell->cmd_hash, name, existing, NULL))
	{
		free(existing->path);
		free(existing);
		return (0);
	}
	return (1);
}

int	cmd_hash_delete(t_shell *shell, const char *name)
{
	void	*old;

	if (!shell || !shell->cmd_hash || !name)
		return (0);
	if (!ft_hash_delete(shell->cmd_hash, name, &old))
		return (0);
	if (old)
	{
		free(((t_cmd_hash_value *)old)->path);
		free(old);
	}
	return (1);
}

/*
 * @brief Adapter from the generic libft callback to the typed one we
 *        expose, plus pass-through of the user's `userdata`.
 */
typedef struct s_cmd_hash_iter_ctx
{
	void	(*f)(const char *, t_cmd_hash_value *, void *);
	void	*userdata;
}	t_cmd_hash_iter_ctx;

static void	iter_adapter(const char *key, void *value, void *ud)
{
	t_cmd_hash_iter_ctx	*ctx;

	ctx = (t_cmd_hash_iter_ctx *)ud;
	ctx->f(key, (t_cmd_hash_value *)value, ctx->userdata);
}

void	cmd_hash_iter(t_shell *shell,
				void (*f)(const char *, t_cmd_hash_value *, void *),
				void *userdata)
{
	t_cmd_hash_iter_ctx	ctx;

	if (!shell || !shell->cmd_hash || !f)
		return ;
	ctx.f = f;
	ctx.userdata = userdata;
	ft_hash_iter(shell->cmd_hash, iter_adapter, &ctx);
}
