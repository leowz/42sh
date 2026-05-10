/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dlist.c                                           :+:      :+:    :+:    */
/*                                                    +:+ +:+         +:+     */
/*   By: pulgamecanica <pulgamecanica@student.42.fr> +#+  +:+       +#+       */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/24 00:00:00 by pulgamecanica     #+#    #+#             */
/*   Updated: 2026/02/24 00:00:00 by pulgamecanica    ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

/*
** ft_dlstnew: allocate a new doubly-linked node.
** content is stored as a pointer (not copied - caller owns the allocation).
** Returns NULL on malloc failure.
*/
t_dlist	*ft_dlstnew(void *content)
{
	t_dlist	*node;

	node = malloc(sizeof(t_dlist));
	if (!node)
		return (NULL);
	node->content = content;
	node->prev = NULL;
	node->next = NULL;
	return (node);
}

/*
** ft_dlstadd_front: insert node at the head of the list.
** Updates head's prev and the new node's next.
*/
void	ft_dlstadd_front(t_dlist **head, t_dlist *node)
{
	if (!head || !node)
		return ;
	node->next = *head;
	node->prev = NULL;
	if (*head)
		(*head)->prev = node;
	*head = node;
}

/*
** ft_dlstadd_back: append node at the tail of the list.
** Returns the appended node.
*/
t_dlist	*ft_dlstadd_back(t_dlist **head, t_dlist *node)
{
	t_dlist	*last;

	if (!head || !node)
		return (NULL);
	if (!*head)
	{
		*head = node;
		return (node);
	}
	last = *head;
	while (last->next)
		last = last->next;
	last->next = node;
	node->prev = last;
	node->next = NULL;
	return (node);
}

/*
** ft_dlstdelone: unlink node from its neighbours and free it.
** del(content) is called to free the content before freeing the node itself.
** The neighbours are re-linked - caller must update any head/tail pointers.
*/
void	ft_dlstdelone(t_dlist *node, void (*del)(void *))
{
	if (!node)
		return ;
	if (node->prev)
		node->prev->next = node->next;
	if (node->next)
		node->next->prev = node->prev;
	if (del && node->content)
		del(node->content);
	free(node);
}

/*
** ft_dlstclear: free every node in the list and set *head to NULL.
*/
void	ft_dlstclear(t_dlist **head, void (*del)(void *))
{
	t_dlist	*cur;
	t_dlist	*nxt;

	if (!head)
		return ;
	cur = *head;
	while (cur)
	{
		nxt = cur->next;
		if (del && cur->content)
			del(cur->content);
		free(cur);
		cur = nxt;
	}
	*head = NULL;
}

/*
** ft_dlstsize: count nodes in the list.
*/
size_t	ft_dlstsize(t_dlist *head)
{
	size_t	n;

	n = 0;
	while (head)
	{
		n++;
		head = head->next;
	}
	return (n);
}
