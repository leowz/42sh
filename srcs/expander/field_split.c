/**
 * @file field_split.c
 * @brief Split an expanded buffer on $IFS, honouring the split mask.
 * @author pulgamecanica
 *
 * Field splitting (POSIX 2.6.5) only applies to bytes that came from
 * an unquoted expansion — exactly the bytes whose mask byte is 1 in
 * t_xbuf.  Literal bytes (including spaces inside "..." or '...') are
 * preserved unconditionally.
 *
 * Splitting rules implemented (matching dash / bash):
 *   - Default IFS is " \t\n".
 *   - Leading IFS whitespace is dropped; a run of IFS whitespace
 *     between two values is one delimiter (no empty field generated).
 *   - Each non-whitespace IFS byte is a delimiter. IFS whitespace
 *     adjacent to it is consumed with it.
 *   - Adjacent non-whitespace IFS delimiters produce an empty field
 *     between them.
 *   - A trailing delimiter does NOT produce a trailing empty field.
 *   - IFS = "" disables splitting entirely (single field).
 *   - An empty buffer yields zero fields.
 */

#include "42sh.h"
#include "expander.h"
#include <stdlib.h>

/**
 * @brief Field record (offset, length) into the expansion buffer.
 * @details We collect these first and only allocate the t_strdup-style
 *          char* array once we know the final count.
 */
typedef struct s_field
{
	size_t	start;
	size_t	len;
}	t_field;

/**
 * @brief True if @c buf->data[i] is in @p ifs and the mask allows splitting.
 */
static int	is_ifs_split(const t_xbuf *buf, size_t i, const char *ifs)
{
	if (!buf->mask[i])
		return (0);
	return (ft_strchr(ifs, buf->data[i]) != NULL);
}

/**
 * @brief True if @p c is one of POSIX's three IFS-whitespace bytes
 *        AND it appears in @p ifs.
 */
static int	is_ifs_ws(const char *ifs, char c)
{
	if (c != ' ' && c != '\t' && c != '\n')
		return (0);
	return (ft_strchr(ifs, c) != NULL);
}

/**
 * @brief Append a field record to a growing array.
 * @return New capacity, or 0 on allocation failure (caller frees).
 */
static size_t	push_field(t_field **fields, size_t *count, size_t cap,
		t_field f)
{
	t_field	*grown;
	size_t	new_cap;
	size_t	i;

	if (*count + 1 > cap)
	{
		new_cap = cap ? cap * 2 : 4;
		grown = malloc(sizeof(t_field) * new_cap);
		if (!grown)
			return (0);
		i = 0;
		while (i < *count)
		{
			grown[i] = (*fields)[i];
			i++;
		}
		free(*fields);
		*fields = grown;
		cap = new_cap;
	}
	(*fields)[*count] = f;
	*count += 1;
	return (cap);
}

/**
 * @brief Skip a run of IFS whitespace starting at *i.
 */
static void	skip_ifs_ws(const t_xbuf *buf, const char *ifs, size_t *i)
{
	while (*i < buf->len && is_ifs_split(buf, *i, ifs)
		&& is_ifs_ws(ifs, buf->data[*i]))
		*i += 1;
}

/**
 * @brief Consume one delimiter at *i: optional non-ws byte plus
 *        any surrounding IFS whitespace.
 */
static void	consume_delimiter(const t_xbuf *buf, const char *ifs, size_t *i)
{
	skip_ifs_ws(buf, ifs, i);
	if (*i < buf->len && is_ifs_split(buf, *i, ifs)
		&& !is_ifs_ws(ifs, buf->data[*i]))
	{
		*i += 1;
		skip_ifs_ws(buf, ifs, i);
	}
}

/**
 * @brief Walk the buffer and record each field.
 * @return Field array (caller frees) or NULL on allocation failure.
 *         When the buffer expands to zero fields, *out_count is 0 and
 *         the returned pointer is NULL — that is not an error.
 */
static t_field	*collect_fields(const t_xbuf *buf, const char *ifs,
		size_t *out_count)
{
	t_field	*fields;
	size_t	cap;
	size_t	i;
	t_field	cur;

	fields = NULL;
	cap = 0;
	*out_count = 0;
	i = 0;
	skip_ifs_ws(buf, ifs, &i);
	while (i < buf->len)
	{
		cur.start = i;
		while (i < buf->len && !is_ifs_split(buf, i, ifs))
			i++;
		cur.len = i - cur.start;
		cap = push_field(&fields, out_count, cap, cur);
		if (!cap)
			return (free(fields), NULL);
		if (i >= buf->len)
			break ;
		consume_delimiter(buf, ifs, &i);
	}
	return (fields);
}

/**
 * @brief Convert collected (start, len) records to a NULL-terminated array.
 */
static char	**materialise(const t_xbuf *buf, t_field *fields, size_t count)
{
	char	**out;
	size_t	i;

	out = malloc(sizeof(char *) * (count + 1));
	if (!out)
		return (NULL);
	i = 0;
	while (i < count)
	{
		out[i] = ft_strsub(buf->data, fields[i].start, fields[i].len);
		if (!out[i])
		{
			while (i > 0)
				free(out[--i]);
			free(out);
			return (NULL);
		}
		i++;
	}
	out[count] = NULL;
	return (out);
}

/**
 * @brief Build a single-field array holding the buffer verbatim.
 */
static char	**single_field(const t_xbuf *buf)
{
	char	**out;

	out = malloc(sizeof(char *) * 2);
	if (!out)
		return (NULL);
	out[0] = ft_strsub(buf->data, 0, buf->len);
	if (!out[0])
		return (free(out), NULL);
	out[1] = NULL;
	return (out);
}

/**
 * @brief Build a zero-field result (single NULL pointer).
 */
static char	**empty_array(void)
{
	char	**out;

	out = malloc(sizeof(char *) * 1);
	if (!out)
		return (NULL);
	out[0] = NULL;
	return (out);
}

char	**field_split(t_shell *shell, const t_xbuf *expanded)
{
	const char	*ifs;
	t_field		*fields;
	size_t		count;
	char		**out;

	if (!expanded || !expanded->data)
		return (empty_array());
	ifs = var_get_value(shell, "IFS");
	if (!ifs)
		ifs = " \t\n";
	if (ifs[0] == '\0')
		return (single_field(expanded));
	if (expanded->len == 0)
		return (empty_array());
	fields = collect_fields(expanded, ifs, &count);
	if (!fields && count > 0)
		return (NULL);
	out = materialise(expanded, fields, count);
	free(fields);
	return (out);
}
