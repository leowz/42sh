/*
 * @file btree.c file
 * @brief functions to manage binary trees
 * @author jguillem
 */

#include <stdlib.h>
#include "libft.h"

/*
 * @brief insert a node in a binary tree
 * @details put on the left if smaller, on the right otherwise
 * @details recursive function
 * @return void
 * @param root struct s_btree
 * @param item void pointer
 * @cmpf comparison function
 */
void	btree_insert_data(t_btree **root, void *item,
			int (*cmpf)(void *, void *))
{
	if (!root)
		return ;
	if (!*root)
		*root = btree_create_node(item);
	else if (cmpf((*root)->item, item) > 0)
	{
		if ((*root)->left)
			btree_insert_data(&((*root)->left), item, cmpf);
		else
			(*root)->left = btree_create_node(item);
	}
	else
	{
		if ((*root)->right)
			btree_insert_data(&((*root)->right), item, cmpf);
		else
			(*root)->right = btree_create_node(item);
	}
}

/*
 * @brief allocate memory for a new binary tree node
 * @param item void pointer
 */
t_btree	*btree_create_node(void *item)
{
	t_btree	*btree;

	btree = malloc(sizeof(t_btree));
	if (!btree)
		return (NULL);
	btree->left = 0;
	btree->right = 0;
	btree->item = item;
	return (btree);
}

/*
 * @brief free memory of the tree
 * @details recursive function
 * @param root struct s_btree
 * @param del delete function, can be set to NULL
 */
void	btree_clear(t_btree **root, void (*del)(void *))
{
	if (!root)
		return ;
	if ((*root)->left)
		btree_clear(&((*root)->left), del);
	if ((*root)->right)
		btree_clear(&((*root)->right), del);
	if (*root)
	{
		if (del)
			del((*root)->item);
		free(*root);
	}
}
