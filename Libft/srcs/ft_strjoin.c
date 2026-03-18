/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_strjoin.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zweng <marvin@42.fr>                       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2017/11/11 12:57:24 by zweng             #+#    #+#             */
/*   Updated: 2017/11/11 13:09:08 by zweng            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "libft.h"

char *ft_strjoin(char *buf, char *add) {
	char	*newbuf;
	int		len;

	if (buf == NULL)
		len = 0;
	else
		len = ft_strlen(buf);
	newbuf = calloc((len + ft_strlen(add) + 1), sizeof(*newbuf));
	if (newbuf == NULL)
		return (NULL);
	if (buf != NULL)
		ft_strlcat(newbuf, buf, len + 1);
	ft_strlcat(newbuf, add, len + ft_strlen(add) + 1);
	return (newbuf);
}
