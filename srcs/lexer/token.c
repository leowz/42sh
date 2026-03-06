/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   token.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jguillem <jguillem@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/24 15:39:46 by jguillem          #+#    #+#             */
/*   Updated: 2026/02/27 18:56:40 by jguillem         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <stdlib.h>
#include "lexer.h"

t_list	*token_new(t_token_type type, char *value, int io_number)
{
	t_token	*token;

	token = malloc(sizeof(t_token));
	if (!token)
		return (NULL);
	token->type = type;
	token->value = value;
	token->io_number = io_number;
	return (ft_lstnew(token));
}

void	token_free(t_token *token)
{
	free(token->value);
	free(token);
}

void	lexer_free_tokens(t_list *tokens)
{
	ft_lstdel(&tokens, (void *)&token_free);
}
