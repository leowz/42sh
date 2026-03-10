/**
 * @file test_btree.c
 * @brief Test suite for the binary tree (t_btree) implementation
 * @author jguillem
 *
 * Covers btree_insert_data(), btree_create_node(), btree_clear()
 */

#include "minunit.h"
#include "../Libft/includes/libft.h"
#include <stdlib.h>

static void	del_item(void *content)
{
	free(content);
}

static int	int_cmp(int *a, int *b)
{
	return (*a - *b);
}

void	test_btree_suite(void)
{
	t_btree	*root;
	int		*val;
	int		first = 34;

	root = NULL;
	while (first > 1)
	{
		val = malloc(sizeof(int));
		*val = first;
		btree_insert_data(&root, val, (int (*)(void *, void *))&int_cmp);
		if (first % 2)
			first = 3 * first + 1;
		else
			first /= 2;
	}
	MU_ASSERT_INT(34, *(int *)root->item);
	MU_ASSERT_INT(17, *(int *)root->left->item);
	MU_ASSERT_INT(52, *(int *)root->right->item);
	MU_ASSERT_INT(26, *(int *)root->left->right->item);
	MU_ASSERT_INT(13, *(int *)root->left->left->item);
	MU_ASSERT_INT(40, *(int *)root->right->left->item);
	MU_ASSERT_INT(20, *(int *)root->left->right->left->item);
	MU_ASSERT_INT(10, *(int *)root->left->left->left->item);
	MU_ASSERT_INT(5, *(int *)root->left->left->left->left->item);
	MU_ASSERT_INT(16, *(int *)root->left->left->right->item);
	MU_ASSERT_INT(8, *(int *)root->left->left->left->left->right->item);
	MU_ASSERT_INT(4, *(int *)root->left->left->left->left->left->item);
	MU_ASSERT_INT(2, *(int *)root->left->left->left->left->left->left->item);
	btree_clear(&root, del_item);
}
