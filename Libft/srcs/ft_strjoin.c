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

char	*ft_strjoin(char const *s1, char const *s2)
{
	char	*newbuf;
	size_t	len1;
	size_t	len2;

	if (!s2)
		return (NULL);
	if (!s1)
		len1 = 0;
	else
		len1 = ft_strlen(s1);
	len2 = ft_strlen(s2);
	newbuf = (char *)malloc(sizeof(char) * (len1 + len2 + 1));
	if (!newbuf)
		return (NULL);
	if (s1)
		ft_strcpy(newbuf, s1);
	else
		newbuf[0] = '\0';
	ft_strcat(newbuf, s2);
	return (newbuf);
}
