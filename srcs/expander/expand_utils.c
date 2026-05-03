/**
 * @file expand_utils.c
 * @brief Growing buffer used during word expansion.
 * @author pulgamecanica
 *
 * The expander needs to build two parallel byte streams: the expanded
 * text itself and a "split mask" telling field_split which IFS bytes
 * came from an unquoted expansion.  t_xbuf encapsulates that pair so
 * the rest of the expander can stay readable.
 */

#include "42sh.h"
#include "expander.h"
#include <stdlib.h>

#define XBUF_INITIAL_CAP	16

static int	xbuf_grow(t_xbuf *buf, size_t need)
{
	size_t	new_cap;
	char	*new_data;
	char	*new_mask;

	new_cap = buf->cap ? buf->cap : XBUF_INITIAL_CAP;
	while (new_cap <= need)
		new_cap *= 2;
	new_data = malloc(new_cap);
	new_mask = malloc(new_cap);
	if (!new_data || !new_mask)
	{
		free(new_data);
		free(new_mask);
		return (-1);
	}
	if (buf->len)
	{
		ft_memcpy(new_data, buf->data, buf->len);
		ft_memcpy(new_mask, buf->mask, buf->len);
	}
	free(buf->data);
	free(buf->mask);
	buf->data = new_data;
	buf->mask = new_mask;
	buf->cap = new_cap;
	return (0);
}

int	xbuf_init(t_xbuf *buf)
{
	if (!buf)
		return (-1);
	buf->data = malloc(XBUF_INITIAL_CAP);
	buf->mask = malloc(XBUF_INITIAL_CAP);
	if (!buf->data || !buf->mask)
	{
		free(buf->data);
		free(buf->mask);
		buf->data = NULL;
		buf->mask = NULL;
		return (-1);
	}
	buf->len = 0;
	buf->cap = XBUF_INITIAL_CAP;
	buf->data[0] = '\0';
	buf->mask[0] = 0;
	return (0);
}

void	xbuf_free(t_xbuf *buf)
{
	if (!buf)
		return ;
	free(buf->data);
	free(buf->mask);
	buf->data = NULL;
	buf->mask = NULL;
	buf->len = 0;
	buf->cap = 0;
}

int	xbuf_putc(t_xbuf *buf, char c, char split)
{
	if (!buf)
		return (-1);
	if (buf->len + 1 >= buf->cap && xbuf_grow(buf, buf->len + 1) == -1)
		return (-1);
	buf->data[buf->len] = c;
	buf->mask[buf->len] = split;
	buf->len++;
	buf->data[buf->len] = '\0';
	buf->mask[buf->len] = 0;
	return (0);
}

int	xbuf_puts(t_xbuf *buf, const char *s, char split)
{
	size_t	i;

	if (!buf)
		return (-1);
	if (!s)
		return (0);
	i = 0;
	while (s[i])
	{
		if (xbuf_putc(buf, s[i], split) == -1)
			return (-1);
		i++;
	}
	return (0);
}
