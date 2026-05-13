/**
 * @file expand_tilde.c
 * @brief Tilde expansion: leading "~" and "~user" forms.
 * @author pulgamecanica
 *
 * Tilde expansion only fires at the very beginning of a word (or after
 * a ':' inside an assignment value, which the executor handles by
 * splitting the value first - not this module's concern).  When the
 * substitution can't be resolved (e.g. no $HOME, unknown user) the
 * original text is emitted literally so that `ls ~unknownuser` still
 * passes a sensible argument to `ls` instead of failing silently.
 */

#include "42sh.h"
#include "expander.h"
#include <pwd.h>
#include <stdlib.h>
#include <unistd.h>

static int	is_tilde_terminator(char c)
{
	return (c == '\0' || c == '/');
}

/**
 * @brief Append the original "~" or "~name" run literally.
 * @details Used as a fallback when expansion can't resolve.  The mask
 *          for these bytes is 0 (literal) so they don't trigger field
 *          splitting later.
 */
static int	push_literal_tilde(const char *input, size_t start, size_t end,
		t_xbuf *out)
{
	size_t	i;

	i = start;
	while (i < end)
	{
		if (xbuf_putc(out, input[i], 0) == -1)
			return (-1);
		i++;
	}
	return (0);
}

static int	expand_home(t_shell *shell, const char *input,
		size_t *pos, t_xbuf *out)
{
	const char		*home;
	struct passwd	*pw;
	int				rc;

	home = var_get_value(shell, "HOME");
	if (home)
		rc = xbuf_puts(out, home, 0);
	else
	{
		pw = getpwuid(getuid());
		if (pw && pw->pw_dir)
			rc = xbuf_puts(out, pw->pw_dir, 0);
		else
			rc = push_literal_tilde(input, *pos, *pos + 1, out);
	}
	*pos += 1;
	return (rc);
}

static int	expand_user(const char *input, size_t *pos, t_xbuf *out)
{
	size_t			name_end;
	char			*name;
	struct passwd	*pw;
	int				rc;

	name_end = *pos + 1;
	while (input[name_end] && !is_tilde_terminator(input[name_end]))
		name_end++;
	name = ft_strsub(input, *pos + 1, name_end - (*pos + 1));
	if (!name)
		return (-1);
	pw = getpwnam(name);
	free(name);
	if (!pw || !pw->pw_dir)
	{
		rc = push_literal_tilde(input, *pos, name_end, out);
		*pos = name_end;
		return (rc);
	}
	if (xbuf_puts(out, pw->pw_dir, 0) == -1)
		return (-1);
	*pos = name_end;
	return (0);
}

int	expand_tilde_at(t_shell *shell, const char *input,
		size_t *pos, t_xbuf *out)
{
	if (input[*pos] != '~')
		return (0);
	if (is_tilde_terminator(input[*pos + 1]))
		return (expand_home(shell, input, pos, out));
	return (expand_user(input, pos, out));
}
