/*
 * @file hash.c
 * @brief Generic string-keyed hash table backed by t_list chains.
 * @author pulgamecanica
 *
 * Separate-chaining hash table. Each bucket holds a t_list of t_hash_entry.
 * Keys are duplicated and owned by the table; values are stored by pointer
 * and remain caller-owned (use the `del` callback on destroy / clear to
 * free them through the table, or pass a non-NULL `old_out` to recover
 * the previous value on set / delete).
 */

#include "libft.h"

/*
 * @brief DJB2 string hash (Bernstein, k=33).
 * @details Cheap, well-mixed, and stable across runs. Returning
 *          unsigned long lets callers `% bucket_count` directly.
 */
unsigned long	ft_strhash(const char *s)
{
	unsigned long	h;
	int				c;

	h = 5381;
	if (!s)
		return (0);
	while ((c = (unsigned char)*s++))
		h = ((h << 5) + h) + (unsigned long)c;
	return (h);
}

/*
 * @brief Locate the node holding `key`, with its predecessor for unlink.
 * @details `*bucket_idx` is always set (when h and key are valid) so the
 *          caller can use it to insert at the head on a miss.
 */
t_list	*hash_lookup_node(const t_hash *h, const char *key,
		size_t *bucket_idx, t_list **prev_out)
{
	t_list			*cur;
	t_list			*prev;
	t_hash_entry	*e;

	if (!h || !key || !h->buckets || h->bucket_count == 0)
		return (NULL);
	*bucket_idx = ft_strhash(key) % h->bucket_count;
	cur = h->buckets[*bucket_idx];
	prev = NULL;
	while (cur)
	{
		e = (t_hash_entry *)cur->content;
		if (e && ft_strcmp(e->key, key) == 0)
		{
			if (prev_out)
				*prev_out = prev;
			return (cur);
		}
		prev = cur;
		cur = cur->next;
	}
	if (prev_out)
		*prev_out = prev;
	return (NULL);
}

t_hash	*ft_hash_new(size_t bucket_count)
{
	t_hash	*h;

	if (bucket_count == 0)
		return (NULL);
	h = malloc(sizeof(t_hash));
	if (!h)
		return (NULL);
	h->buckets = ft_memalloc(sizeof(t_list *) * bucket_count);
	if (!h->buckets)
	{
		free(h);
		return (NULL);
	}
	h->bucket_count = bucket_count;
	h->size = 0;
	return (h);
}

void	*ft_hash_get(const t_hash *h, const char *key)
{
	t_list	*node;
	size_t	idx;

	node = hash_lookup_node(h, key, &idx, NULL);
	if (!node)
		return (NULL);
	return (((t_hash_entry *)node->content)->value);
}

size_t	ft_hash_size(const t_hash *h)
{
	if (!h)
		return (0);
	return (h->size);
}
