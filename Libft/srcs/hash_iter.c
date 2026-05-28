/*
 * @file hash_iter.c
 * @brief Iteration and teardown for t_hash.
 * @author pulgamecanica
 */

#include "libft.h"

/*
 * @brief Walk every entry, calling `f(key, value, userdata)`.
 * @details Iteration order is bucket-major: not stable across rehashes,
 *          and not sorted. Callers that need a sorted listing should
 *          collect into an array and sort there.
 */
void	ft_hash_iter(const t_hash *h,
				void (*f)(const char *, void *, void *),
				void *userdata)
{
	size_t			i;
	t_list			*cur;
	t_hash_entry	*e;

	if (!h || !f || !h->buckets)
		return ;
	i = 0;
	while (i < h->bucket_count)
	{
		cur = h->buckets[i];
		while (cur)
		{
			e = (t_hash_entry *)cur->content;
			if (e)
				f(e->key, e->value, userdata);
			cur = cur->next;
		}
		i++;
	}
}

/*
 * @brief Free every entry but keep the bucket array intact.
 * @param del If non-NULL, called on each stored value before its entry
 *            is freed - the right hook to release caller-owned values.
 */
void	ft_hash_clear(t_hash *h, void (*del)(void *))
{
	size_t			i;
	t_list			*cur;
	t_list			*nxt;
	t_hash_entry	*e;

	if (!h || !h->buckets)
		return ;
	i = 0;
	while (i < h->bucket_count)
	{
		cur = h->buckets[i];
		while (cur)
		{
			nxt = cur->next;
			e = (t_hash_entry *)cur->content;
			if (e && del && e->value)
				del(e->value);
			if (e)
				free(e->key);
			free(e);
			free(cur);
			cur = nxt;
		}
		h->buckets[i++] = NULL;
	}
	h->size = 0;
}

void	ft_hash_destroy(t_hash *h, void (*del)(void *))
{
	if (!h)
		return ;
	ft_hash_clear(h, del);
	free(h->buckets);
	free(h);
}
