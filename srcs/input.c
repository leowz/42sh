/**
 * @file input.c
 * @brief The shell's single stdin reader plus logical-line assembly.
 * @author wengzhang
 *
 * `shell_read_line` is the one place the shell consumes a *physical* line
 * of input. The REPL and the heredoc collector both call it, so command
 * lines and heredoc bodies travel through a single stdin cursor and can
 * never desynchronise.
 *
 * `shell_read_logical_line` is the REPL's wrapper: it calls
 * `shell_read_line` repeatedly to assemble a *logical* line that spans
 * physical lines across unclosed quotes and trailing-backslash line
 * continuation. The heredoc collector does NOT use it (heredoc bodies are
 * read verbatim, line by line).
 */
#include "42sh.h"
#include <stdlib.h>

typedef enum e_lcont_state
{
	LCONT_DONE,
	LCONT_SQUOTE,
	LCONT_DQUOTE,
	LCONT_BACKSLASH,
}	t_lcont_state;

char	*shell_read_line(t_shell *shell, const char *prompt)
{
	char	*line;
	size_t	len;
	ssize_t	n;

	if (shell->interactive)
		return (readline(prompt));
	line = NULL;
	len = 0;
	n = getline(&line, &len, stdin);
	if (n <= 0)
	{
		free(line);
		return (NULL);
	}
	if (line[n - 1] == '\n')
		line[n - 1] = '\0';
	return (line);
}

/**
 * @brief Scan @p line for an open quote or trailing backslash that would
 *        require another physical line to complete.
 * @details Inside single quotes, backslash is literal (no escaping); inside
 *          double quotes, backslash escapes the next byte; outside both,
 *          a backslash at end-of-line is the line-continuation marker.
 *          Quote state at end-of-line is what selects SQUOTE/DQUOTE.
 */
static t_lcont_state	scan_lcont(const char *line)
{
	int		in_sq;
	int		in_dq;
	size_t	i;

	in_sq = 0;
	in_dq = 0;
	i = 0;
	while (line[i])
	{
		if (!in_sq && line[i] == '\\')
		{
			if (line[i + 1] == '\0')
				return (LCONT_BACKSLASH);
			i += 2;
			continue ;
		}
		if (!in_dq && line[i] == '\'')
			in_sq = !in_sq;
		else if (!in_sq && line[i] == '"')
			in_dq = !in_dq;
		i++;
	}
	if (in_sq)
		return (LCONT_SQUOTE);
	if (in_dq)
		return (LCONT_DQUOTE);
	return (LCONT_DONE);
}

/**
 * @brief Strip a single trailing `\` from @p line (the line-continuation
 *        marker is consumed and discarded along with its newline).
 */
static void	strip_trailing_backslash(char *line)
{
	size_t	len;

	len = ft_strlen(line);
	if (len > 0 && line[len - 1] == '\\')
		line[len - 1] = '\0';
}

/**
 * @brief Replace @p *head with `*head + sep + tail`; frees both inputs.
 * @return 0 on success, -1 on allocation failure (head/tail are freed).
 */
static int	lcont_append(char **head, char *tail, const char *sep)
{
	char	*mid;
	char	*out;

	mid = ft_strjoin(*head, sep);
	if (!mid)
		return (free(*head), free(tail), *head = NULL, -1);
	out = ft_strjoin(mid, tail);
	free(mid);
	free(tail);
	if (!out)
		return (free(*head), *head = NULL, -1);
	free(*head);
	*head = out;
	return (0);
}

/**
 * @brief Read a logical line, joining physical lines across unclosed
 *        quotes and trailing-backslash continuation.
 * @details Implements the Inhibitors-module continuation prompt:
 *
 *              $> echo foo\<ENTER>
 *              > bar<ENTER>
 *
 *          returns one buffer "echo foobar" -- the `\<NL>` pair is
 *          consumed. For an open quote (`'a<ENTER>` or `"a<ENTER>`),
 *          the newline IS kept inside the string and `> ` is reprinted
 *          until the matching quote is typed. Returns NULL on EOF before
 *          the first physical line; on EOF mid-continuation returns the
 *          partial line so the lexer reports the unterminated state.
 */
char	*shell_read_logical_line(t_shell *shell, const char *primary)
{
	char			*line;
	char			*cont;
	t_lcont_state	state;

	line = shell_read_line(shell, primary);
	if (!line)
		return (NULL);
	state = scan_lcont(line);
	while (state != LCONT_DONE)
	{
		cont = shell_read_line(shell, "> ");
		if (!cont)
		{
			if (state == LCONT_BACKSLASH)
				strip_trailing_backslash(line);
			return (line);
		}
		if (state == LCONT_BACKSLASH)
			strip_trailing_backslash(line);
		if (lcont_append(&line, cont, (state == LCONT_BACKSLASH) ? "" : "\n")
			== -1)
			return (NULL);
		state = scan_lcont(line);
	}
	return (line);
}
