/*
 * @file hash_set.c
 * @brief Mutating operations for t_hash: set / delete.
 * @author pulgamecanica
 */

#include "libft.h"

/*
 * @brief Allocate an entry owning a duplicated key and the given value.
 *        Returns NULL (without leaking) on any allocation failure.
 */
static t_hash_entry	*entry_new(const char *key, void *value)
{
	t_hash_entry	*e;

	e = malloc(sizeof(t_hash_entry));
	if (!e)
		return (NULL);
	e->key = ft_strdup(key);
	if (!e->key)
	{
		free(e);
		return (NULL);
	}
	e->value = value;
	return (e);
}

/*
 * @brief Insert (or replace) `key -> value`.
 * @param old_out If non-NULL, receives the previous value on replacement
 *                (or NULL on a fresh insert). Lets the caller free the
 *                old value without the table having to know how.
 * @return 1 on success, 0 on allocation failure (table unchanged).
 */
int	ft_hash_set(t_hash *h, const char *key, void *value, void **old_out)
{
	t_list			*node;
	t_hash_entry	*e;
	size_t			idx;

	if (!h || !key)
		return (0);
	node = hash_lookup_node(h, key, &idx, NULL);
	if (node)
	{
		e = (t_hash_entry *)node->content;
		if (old_out)
			*old_out = e->value;
		e->value = value;
		return (1);
	}
	if (old_out)
		*old_out = NULL;
	e = entry_new(key, value);
	if (!e)
		return (0);
	node = ft_lstnew(e);
	if (!node)
	{
		free(e->key);
		free(e);
		return (0);
	}
	node->next = h->buckets[idx];
	h->buckets[idx] = node;
	h->size++;
	return (1);
}

/*
 * @brief Unlink and free a node found by hash_lookup_node().
 *        Returns the entry's value so the caller (or `old_out`) can
 *        decide what to do with it.
 */
static void	*unlink_node(t_hash *h, size_t idx, t_list *prev, t_list *node)
{
	t_hash_entry	*e;
	void			*value;

	e = (t_hash_entry *)node->content;
	value = e->value;
	if (prev)
		prev->next = node->next;
	else
		h->buckets[idx] = node->next;
	free(e->key);
	free(e);
	free(node);
	h->size--;
	return (value);
}

/*
 * @brief Remove `key`. Returns 1 if an entry was removed, 0 otherwise.
 *        On removal, `*old_out` (if non-NULL) receives the stored value.
 */
int	ft_hash_delete(t_hash *h, const char *key, void **old_out)
{
	t_list	*node;
	t_list	*prev;
	size_t	idx;
	void	*value;

	if (!h || !key)
		return (0);
	prev = NULL;
	node = hash_lookup_node(h, key, &idx, &prev);
	if (!node)
	{
		if (old_out)
			*old_out = NULL;
		return (0);
	}
	value = unlink_node(h, idx, prev, node);
	if (old_out)
		*old_out = value;
	return (1);
}
