/**
 * @file test_dlist.c
 * @brief Test suite for the doubly-linked list (t_dlist) implementation.
 * @author pulgamecanica (arosado-)
 *
 * Covers ft_dlstnew(), ft_dlstadd_back(), ft_dlstsize().
 * 
 * Forward and backward traversal via `->next` / `->prev`, and ft_dlstclear().
 *
 * This suite is always compiled and always runs - it is not guarded by a
 * feature flag because `t_list` is a core Libft primitive.
 */

#include "minunit.h"
#include "../Libft/includes/libft.h"
#include <stdlib.h>

static void	del_int(void *content)
{
	free(content);
}

/**
 * Run all doubly-linked list assertions.
 *
 * Builds a three-node list `[1 <-> 2 <-> 3]`, verifies size, forward and
 * backward links, sentinel NULL values at both ends, then calls
 * ft_dlstclear() and asserts the head pointer is NULL afterwards.
 */
void	test_dlist_suite(void)
{
	t_dlist	*head;
	t_dlist	*node;
	int		*val;

	head = NULL;
	val = malloc(sizeof(int));
	*val = 1;
	ft_dlstadd_back(&head, ft_dlstnew(val));
	val = malloc(sizeof(int));
	*val = 2;
	ft_dlstadd_back(&head, ft_dlstnew(val));
	val = malloc(sizeof(int));
	*val = 3;
	ft_dlstadd_back(&head, ft_dlstnew(val));
	MU_ASSERT_INT(3, (int)ft_dlstsize(head));
	node = head;
	MU_ASSERT_INT(1, *(int *)node->content);
	node = node->next;
	MU_ASSERT_INT(2, *(int *)node->content);
	node = node->next;
	MU_ASSERT_INT(3, *(int *)node->content);
	MU_ASSERT("tail->next is NULL", node->next == NULL);
	MU_ASSERT_INT(2, *(int *)node->prev->content);
	MU_ASSERT_INT(1, *(int *)node->prev->prev->content);
	MU_ASSERT("head->prev is NULL", head->prev == NULL);
	ft_dlstclear(&head, del_int);
	MU_ASSERT("head is NULL after clear", head == NULL);
}