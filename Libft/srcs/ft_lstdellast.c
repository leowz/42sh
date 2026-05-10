/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_lstdellast.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jguillem <jguillem@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/04/28 15:13:48 by jguillem          #+#    #+#             */
/*   Updated: 2026/04/28 15:36:22 by jguillem         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

void	ft_lstdellast(t_list **alst, void (*del)(void *))
{
	t_list	*current;

	if (!alst || !*alst || !del)
		return ;
	if (!(*alst)->next)
	{
		ft_lstdelone(alst, del);
		return ;
	}
	current = *alst;
	while (current->next->next)
		current = current->next;
	ft_lstdelone(&(current->next), del);
	current->next = NULL;
}
