/**
 * @file expand_parameter.c
 * @brief Dollar-sign expansions: $NAME, ${NAME}, $?, $$, $0.
 * @author pulgamecanica
 *
 * Called from the char-by-char loop in expand_word.c whenever the
 * current byte is '$'.  Recognises the POSIX special parameters
 * required by the subject ($?, $$, $0, ${?}) and standard variable
 * names matching the identifier grammar [A-Za-z_][A-Za-z0-9_]*.
 *
 * Anything that doesn't match a recognised form (e.g. `$+`, a lone
 * trailing `$`) is emitted literally — this matches POSIX behaviour
 * and the way bash treats unrecognised dollar sequences.
 */

#include "42sh.h"
#include "expander.h"
#include <stdlib.h>
#include <unistd.h>

static int	is_name_start(char c)
{
	return (ft_isalpha((unsigned char)c) || c == '_');
}

static int	is_name_char(char c)
{
	return (ft_isalnum((unsigned char)c) || c == '_');
}

/**
 * @brief Push a value lookup result to @p out.
 * @details Resolves @p name in the shell variables. If @p name names a
 *          known variable, its value is appended (NULL value treated as
 *          empty). If unset, nothing is appended (POSIX default behaviour
 *          when the `nounset` option is off, which is the only mode we
 *          support today).
 */
static int	push_variable(t_shell *shell, const char *name, int dq, t_xbuf *out)
{
	const char	*value;

	value = var_get_value(shell, name);
	if (!value)
		return (0);
	return (xbuf_puts(out, value, !dq));
}

/**
 * @brief Push a special parameter recognised by a single character.
 * @return 1 if @p c was a special parameter (and one was emitted),
 *         0 if @p c is not a recognised special parameter,
 *         -1 on allocation failure.
 */
static int	push_special(t_shell *shell, char c, int dq, t_xbuf *out)
{
	char	*tmp;
	int		rc;

	if (c == '?')
	{
		tmp = ft_itoa(shell->last_exit_status);
		if (!tmp)
			return (-1);
		rc = xbuf_puts(out, tmp, !dq);
		free(tmp);
		return (rc == -1 ? -1 : 1);
	}
	if (c == '$')
	{
		tmp = ft_itoa((int)getpid());
		if (!tmp)
			return (-1);
		rc = xbuf_puts(out, tmp, !dq);
		free(tmp);
		return (rc == -1 ? -1 : 1);
	}
	if (c == '0')
		return (xbuf_puts(out, "42sh", !dq) == -1 ? -1 : 1);
	return (0);
}

/**
 * @brief Read ${...} starting at @c input[*pos] (pointing at '{').
 * @details Only the basic form ${NAME} and the special parameters
 *          ${?}, ${$}, ${0} are implemented here.  The richer modular
 *          formats (${VAR:-word}, ${#VAR}, ...) listed in
 *          plan/04_expander.md are not handled yet — they are deferred
 *          to a follow-up branch.  An unterminated brace is emitted
 *          literally so the user can see what they typed.
 */
static int	expand_braced(t_shell *shell, const char *input,
		size_t *pos, int dq, t_xbuf *out)
{
	size_t	start;
	size_t	end;
	char	*name;
	int		rc;

	start = *pos + 1;
	end = start;
	while (input[end] && input[end] != '}')
		end++;
	if (input[end] != '}')
		return (xbuf_puts(out, "${", 0));
	if (end == start + 1
		&& (input[start] == '?' || input[start] == '$' || input[start] == '0'))
	{
		rc = push_special(shell, input[start], dq, out);
		if (rc == -1)
			return (-1);
		*pos = end + 1;
		return (0);
	}
	name = ft_strsub(input, start, end - start);
	if (!name)
		return (-1);
	rc = push_variable(shell, name, dq, out);
	free(name);
	*pos = end + 1;
	return (rc);
}

/**
 * @brief Read a $-sequence whose unbraced first byte is a name char.
 * @details Consumes the longest valid identifier and looks it up. The
 *          caller has already verified that @c input[*pos] is a name
 *          start character.
 */
static int	expand_named(t_shell *shell, const char *input,
		size_t *pos, int dq, t_xbuf *out)
{
	size_t	start;
	size_t	end;
	char	*name;
	int		rc;

	start = *pos;
	end = start;
	while (input[end] && is_name_char(input[end]))
		end++;
	name = ft_strsub(input, start, end - start);
	if (!name)
		return (-1);
	rc = push_variable(shell, name, dq, out);
	free(name);
	*pos = end;
	return (rc);
}

int	expand_dollar(t_shell *shell, const char *input,
		size_t *pos, int dq, t_xbuf *out)
{
	char	c;
	int		rc;

	*pos += 1;
	c = input[*pos];
	if (c == '\0')
		return (xbuf_putc(out, '$', !dq));
	if (c == '{')
		return (expand_braced(shell, input, pos, dq, out));
	rc = push_special(shell, c, dq, out);
	if (rc == -1)
		return (-1);
	if (rc == 1)
	{
		*pos += 1;
		return (0);
	}
	if (is_name_start(c))
		return (expand_named(shell, input, pos, dq, out));
	return (xbuf_putc(out, '$', !dq));
}
