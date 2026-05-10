/**
 * @file test_list.c
 * @brief Test suite for libft's singly-linked list (t_list).
 * @author pulgamecanica (arosado-)
 *
 * Storage convention: ft_lstnew(ptr) stores the pointer directly in `content`.
 * Access pattern: `(T *)node->content` - wrapped here by the `LST_DUMMY()` macro.
 *
 * Covers ft_lstnew(), ft_lstsize(), forward traversal, ft_lstdel()
 * (pointer-and-node free), and ft_lstadd() (prepend to front).
 *
 * This suite is always compiled and always runs - it is not guarded by a
 * feature flag because `t_list` is a core Libft primitive.
 */

#include "minunit.h"
#include "../Libft/includes/libft.h"
#include <stdlib.h>
#include <string.h>

typedef struct s_dummy
{
	int		id;
	char	*name;
}	t_dummy;

static t_dummy	*dummy_new(int id, const char *name)
{
	t_dummy	*d;

	d = malloc(sizeof(t_dummy));
	d->id = id;
	d->name = strdup(name);
	return (d);
}

static void	del_dummy(void *content)
{
	t_dummy	*d;

	d = (t_dummy *)content;
	free(d->name);
	free(d);
}

# define LST_DUMMY(n)	((t_dummy *)((n)->content))

/* Append helper: walk to end and link node (no copy, no shared ownership). */
static void	lst_append(t_list **head, t_list *node)
{
	t_list	*cur;

	if (!*head)
	{
		*head = node;
		return ;
	}
	cur = *head;
	while (cur->next)
		cur = cur->next;
	cur->next = node;
}

/**
 * Run all singly-linked list assertions.
 *
 * Builds an append-order list `[alpha(1) -> beta(2) -> gamma(3)]`, verifies
 * ft_lstsize() and forward traversal via `->next`, then exercises
 * ft_lstdel() (freeing all nodes with del_dummy) and asserts the head
 * pointer is NULL afterwards.
 *
 * A second sub-test uses ft_lstadd() (prepend) to verify that two nodes
 * inserted as `[first, second]` are stored as `[second -> first]` in the list.
 *
 * > **Precondition:** `Libft/srcs/ft_stdlib/` must be compiled and linked.
 */
void	test_list_suite(void)
{
	t_list	*list;
	t_list	*node;
	t_dummy	*d;

	list = NULL;
	/* -- build: append three unique nodes -- */
	d = dummy_new(1, "alpha");
	lst_append(&list, ft_lstnew(d));
	d = dummy_new(2, "beta");
	lst_append(&list, ft_lstnew(d));
	d = dummy_new(3, "gamma");
	lst_append(&list, ft_lstnew(d));
	/* -- size -- */
	MU_ASSERT_INT(3, (int)ft_lstsize(list));
	/* -- access pattern: (T *)node->content -- */
	node = list;
	MU_ASSERT_INT(1, LST_DUMMY(node)->id);
	MU_ASSERT_STR("alpha via LST_DUMMY", "alpha", LST_DUMMY(node)->name);
	node = node->next;
	MU_ASSERT_INT(2, LST_DUMMY(node)->id);
	node = node->next;
	MU_ASSERT_INT(3, LST_DUMMY(node)->id);
	MU_ASSERT("tail->next NULL", node->next == NULL);
	/* -- ft_lstdel: calls del_dummy on each node, frees content + node -- */
	ft_lstdel(&list, del_dummy);
	MU_ASSERT("list NULL after ft_lstdel", list == NULL);
	/* -- prepend with ft_lstadd (adds to front) -- */
	list = NULL;
	d = dummy_new(10, "first");
	ft_lstadd(&list, ft_lstnew(d));
	d = dummy_new(20, "second");
	ft_lstadd(&list, ft_lstnew(d));
	MU_ASSERT_INT(2, (int)ft_lstsize(list));
	MU_ASSERT_INT(20, LST_DUMMY(list)->id);
	MU_ASSERT_INT(10, LST_DUMMY(list->next)->id);
	ft_lstdel(&list, del_dummy);
}
