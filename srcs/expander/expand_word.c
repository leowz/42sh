/**
 * @file expand_word.c
 * @brief Quote-aware char-by-char expansion of one word into a t_xbuf.
 * @author pulgamecanica
 *
 * This is phase 1 of the expansion pipeline.  It walks the raw token
 * string a byte at a time, tracking whether we're inside single
 * quotes, double quotes, or unquoted, and dispatches to the right
 * helper for `$`, `~` and `\`.  All non-special bytes are copied
 * straight through with the appropriate split-mask flag.
 *
 * Quote semantics (POSIX):
 *   - Single quotes: everything literal until the next ' .  No
 *     expansion of any kind.  No backslash escape inside.
 *   - Double quotes: parameter expansion still runs, but field
 *     splitting does not (the mask byte stays 0 for everything inside).
 *     A backslash escapes only $, `, ", \ and newline; otherwise the
 *     backslash is preserved literally.
 *   - Unquoted: backslash escapes the next character (one-byte copy).
 *
 * Tilde expansion only triggers at byte 0 of the word and only when
 * unquoted.
 */

#include "42sh.h"
#include "expander.h"

/**
 * @brief Tracks the quote state across one expansion pass.
 */
typedef struct s_qstate
{
	int	in_single;
	int	in_double;
}	t_qstate;

/**
 * @brief True if @p c is one of the bytes a backslash escapes inside "".
 */
static int	is_dq_escapable(char c)
{
	return (c == '$' || c == '`' || c == '"' || c == '\\' || c == '\n');
}

/**
 * @brief Handle a backslash sitting at @c input[*pos] in unquoted state.
 * @details Copies the next byte literally (with split=0 so it never
 *          becomes a field boundary).  A trailing backslash is a no-op.
 */
static int	handle_bs_unquoted(const char *input, size_t *pos, t_xbuf *out)
{
	*pos += 1;
	if (input[*pos] == '\0')
		return (0);
	if (xbuf_putc(out, input[*pos], 0) == -1)
		return (-1);
	*pos += 1;
	return (0);
}

/**
 * @brief Handle a backslash inside double quotes.
 * @details Per POSIX 2.2.3 the backslash retains its special meaning
 *          only when followed by $, `, ", \ or newline; otherwise
 *          both the backslash and the next character are passed
 *          through literally.
 */
static int	handle_bs_dq(const char *input, size_t *pos, t_xbuf *out)
{
	char	next;

	next = input[*pos + 1];
	if (next != '\0' && is_dq_escapable(next))
	{
		if (xbuf_putc(out, next, 0) == -1)
			return (-1);
		*pos += 2;
		return (0);
	}
	if (xbuf_putc(out, '\\', 0) == -1)
		return (-1);
	*pos += 1;
	return (0);
}

/**
 * @brief Dispatch one non-quote, non-single-literal byte.
 * @details Called by the main loop after quote toggles and the
 *          single-quote passthrough have been handled.
 */
static int	dispatch_byte(t_shell *shell, const char *input,
		size_t *pos, t_xbuf *out)
{
	char	c;

	c = input[*pos];
	if (c == '\\')
		return (handle_bs_unquoted(input, pos, out));
	if (c == '$')
		return (expand_dollar(shell, input, pos, 0, out));
	if (c == '~' && *pos == 0)
		return (expand_tilde_at(shell, input, pos, out));
	if (xbuf_putc(out, c, 1) == -1)
		return (-1);
	*pos += 1;
	return (0);
}

/**
 * @brief Dispatch one byte while inside double quotes.
 */
static int	dispatch_in_dq(t_shell *shell, const char *input,
		size_t *pos, t_xbuf *out)
{
	char	c;

	c = input[*pos];
	if (c == '\\')
		return (handle_bs_dq(input, pos, out));
	if (c == '$')
		return (expand_dollar(shell, input, pos, 1, out));
	if (xbuf_putc(out, c, 0) == -1)
		return (-1);
	*pos += 1;
	return (0);
}

int	expand_word_into(t_shell *shell, const char *word, t_xbuf *out)
{
	t_qstate	q;
	size_t		pos;
	char		c;

	if (!word || !out)
		return (-1);
	q.in_single = 0;
	q.in_double = 0;
	pos = 0;
	while (word[pos])
	{
		c = word[pos];
		if (c == '\'' && !q.in_double)
		{
			q.in_single = !q.in_single;
			pos++;
		}
		else if (c == '"' && !q.in_single)
		{
			q.in_double = !q.in_double;
			pos++;
		}
		else if (q.in_single)
		{
			if (xbuf_putc(out, c, 0) == -1)
				return (-1);
			pos++;
		}
		else if (q.in_double)
		{
			if (dispatch_in_dq(shell, word, &pos, out) == -1)
				return (-1);
		}
		else if (dispatch_byte(shell, word, &pos, out) == -1)
			return (-1);
	}
	return (0);
}
