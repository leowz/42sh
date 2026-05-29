/**
 * @file input.c
 * @brief Logical-line input reader plus heredoc pre-collection.
 * @author wengzhang
 *
 * `shell_read_line` is the single physical-line reader (readline interactively,
 * getline on shared stdin otherwise).
 *
 * `shell_read_logical_line` is the REPL's wrapper. It spans physical lines on
 * four kinds of incompleteness:
 *
 *   1. Open single/double quote — continues with the newline kept inside the
 *      quoted string (`> ` prompt).
 *   2. Trailing unescaped `\` — continues with the `\<NL>` pair consumed
 *      (`> ` prompt).
 *   3. Unbalanced `(` (subshell or arithmetic-looking group) — continues with
 *      a newline between lines.
 *   4. Pending `<<DELIM` / `<<-DELIM` heredocs — IMMEDIATELY consumes the
 *      heredoc body lines from stdin (NOT joining them into the command line)
 *      and pushes each body onto `shell->heredoc_body_queue`.
 *
 * The heredoc collector in `parser_heredoc.c` pops bodies from that queue
 * before falling back to its line-by-line reader. Net effect: constructs
 * like `(cat <<EOF ... EOF) | sort` (correction-PDF §21sh test #20) parse
 * and execute correctly.
 */
#include "42sh.h"
#include <stdlib.h>
#include <string.h>

typedef struct s_pending_hd
{
	char	*delim;
	int		strip;
}	t_pending_hd;

typedef struct s_lcont_ctx
{
	int		in_sq;
	int		in_dq;
	int		paren_depth;
	int		trailing_bs;
	t_list	*pending_hd;
}	t_lcont_ctx;

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
 * @brief Skip ASCII blanks at @p *cur in place.
 */
static void	skip_blanks(const char **cur)
{
	while (**cur == ' ' || **cur == '\t')
		(*cur)++;
}

/**
 * @brief Read a heredoc delimiter word starting at @p *cur, handling
 *        an optional single/double-quoted form. Advances @p *cur past
 *        the closing quote (if any) or past the unquoted word.
 * @return Newly-allocated unquoted delim string, or NULL on no word.
 */
static char	*read_hd_delim(const char **cur)
{
	const char	*start;
	char		quote;

	skip_blanks(cur);
	if (**cur == '\'' || **cur == '"')
	{
		quote = **cur;
		(*cur)++;
		start = *cur;
		while (**cur && **cur != quote)
			(*cur)++;
		if (**cur == quote)
		{
			char *delim = strndup(start, *cur - start);
			(*cur)++;
			return (delim);
		}
		return (strndup(start, *cur - start));
	}
	start = *cur;
	while (**cur && **cur != ' ' && **cur != '\t'
		&& **cur != ';' && **cur != '|' && **cur != '&'
		&& **cur != '(' && **cur != ')' && **cur != '<' && **cur != '>')
	{
		/* A backslash at end-of-line is a line-continuation marker, not
		 * part of the delim word. Leave it for the outer scanner to
		 * detect as trailing_bs so the next physical line can complete
		 * the delim (`cat << EO\<NL>F` -> delim "EOF"). */
		if (**cur == '\\' && (*cur)[1] == '\0')
			break ;
		(*cur)++;
	}
	if (*cur == start)
		return (NULL);
	return (strndup(start, *cur - start));
}

/**
 * @brief Free a t_pending_hd*.
 */
static void	pending_hd_free(void *p)
{
	t_pending_hd	*h;

	h = (t_pending_hd *)p;
	if (!h)
		return ;
	free(h->delim);
	free(h);
}

/**
 * @brief Detect `<<` or `<<-`, read the delim word, push a t_pending_hd
 *        onto @p ctx->pending_hd. Advances @p *p past the delim.
 *        Called with @p *p pointing at the first `<`.
 */
static void	scan_heredoc_op(const char **p, t_lcont_ctx *ctx)
{
	int				strip;
	char			*delim;
	t_pending_hd	*h;

	*p += 2;
	strip = 0;
	if (**p == '-')
	{
		strip = 1;
		(*p)++;
	}
	delim = read_hd_delim(p);
	if (!delim)
		return ;
	h = (t_pending_hd *)malloc(sizeof(*h));
	if (!h)
	{
		free(delim);
		return ;
	}
	h->delim = delim;
	h->strip = strip;
	ft_lstappend(&ctx->pending_hd, ft_lstnew(h));
}

/**
 * @brief One step of the line scanner: handle quotes, escapes, parens,
 *        heredoc operators, and detect a trailing unescaped backslash.
 * @return 1 if scanning consumed something and the caller should re-check
 *         the trailing-backslash flag; 0 to keep scanning.
 */
static void	scan_line_into(const char *line, t_lcont_ctx *ctx)
{
	const char	*p;

	p = line;
	ctx->trailing_bs = 0;
	while (*p)
	{
		if (!ctx->in_sq && *p == '\\')
		{
			if (p[1] == '\0')
			{
				ctx->trailing_bs = 1;
				return ;
			}
			p += 2;
			continue ;
		}
		if (!ctx->in_dq && *p == '\'')
			ctx->in_sq = !ctx->in_sq;
		else if (!ctx->in_sq && *p == '"')
			ctx->in_dq = !ctx->in_dq;
		else if (!ctx->in_sq && !ctx->in_dq)
		{
			if (*p == '#')
				return ;
			if (*p == '(')
				ctx->paren_depth++;
			else if (*p == ')' && ctx->paren_depth > 0)
				ctx->paren_depth--;
			else if (*p == '<' && p[1] == '<')
			{
				scan_heredoc_op(&p, ctx);
				continue ;
			}
		}
		p++;
	}
}

/**
 * @brief Read heredoc body lines from stdin via @c shell_read_line until
 *        a line equals @p delim (after optional tab-stripping for `<<-`).
 *        Returns the joined body as a single newline-terminated string,
 *        or "" if no body lines. NULL on allocation failure.
 */
static char	*collect_hd_body(t_shell *shell, t_pending_hd *h)
{
	char	*body;
	char	*line;
	char	*cmp;
	char	*grown;
	size_t	blen;
	size_t	llen;

	body = strdup("");
	if (!body)
		return (NULL);
	while (1)
	{
		line = shell_read_line(shell, "> ");
		if (!line)
		{
			fprintf(stderr,
				"\nwarning: here-document delimited by end-of-file (wanted `%s')\n",
				h->delim);
			return (body);
		}
		cmp = line;
		if (h->strip)
			while (*cmp == '\t')
				cmp++;
		if (strcmp(cmp, h->delim) == 0)
		{
			free(line);
			return (body);
		}
		blen = strlen(body);
		llen = strlen(cmp);
		grown = (char *)malloc(blen + llen + 2);
		if (!grown)
		{
			free(body);
			free(line);
			return (NULL);
		}
		memcpy(grown, body, blen);
		memcpy(grown + blen, cmp, llen);
		grown[blen + llen] = '\n';
		grown[blen + llen + 1] = '\0';
		free(body);
		free(line);
		body = grown;
	}
}

/**
 * @brief Drain every pending heredoc captured during line scanning. Each
 *        body is read from stdin and pushed onto @c shell->heredoc_body_queue
 *        in declaration order (matches the parser's AST walk order).
 * @return 0 on success, -1 on allocation failure.
 */
static int	drain_pending_heredocs(t_shell *shell, t_lcont_ctx *ctx)
{
	t_list			*node;
	t_pending_hd	*h;
	char			*body;

	node = ctx->pending_hd;
	while (node)
	{
		h = (t_pending_hd *)node->content;
		body = collect_hd_body(shell, h);
		if (!body)
			return (-1);
		ft_lstappend(&shell->heredoc_body_queue, ft_lstnew(body));
		node = node->next;
	}
	ft_lstdel(&ctx->pending_hd, pending_hd_free);
	ctx->pending_hd = NULL;
	return (0);
}

/**
 * @brief True if any incompleteness state is set: open quote, trailing
 *        backslash, pending heredoc, or unbalanced paren.
 */
static int	needs_continuation(const t_lcont_ctx *ctx)
{
	return (ctx->in_sq || ctx->in_dq || ctx->trailing_bs
		|| ctx->paren_depth > 0 || ctx->pending_hd != NULL);
}

/**
 * @brief Strip the final `\` from @p line (the line-continuation marker).
 */
static void	strip_trailing_bs(char *line)
{
	size_t	len;

	len = ft_strlen(line);
	if (len > 0 && line[len - 1] == '\\')
		line[len - 1] = '\0';
}

/**
 * @brief Replace @p *head with `*head + sep + tail`; takes ownership of
 *        @p tail and frees the old head. Returns 0 on success.
 */
static int	join_in_place(char **head, char *tail, const char *sep)
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
 * @brief Read one continuation physical line, choose the right join
 *        separator, and update scanning state.
 * @return 0 on success, -1 on alloc failure, 1 on EOF mid-continuation
 *         (caller should return @p *line as-is).
 */
static int	read_and_join_one(t_shell *shell, char **line, t_lcont_ctx *ctx)
{
	char		*cont;
	const char	*sep;

	if (ctx->trailing_bs)
	{
		strip_trailing_bs(*line);
		sep = "";
	}
	else
		sep = "\n";
	cont = shell_read_line(shell, "> ");
	if (!cont)
		return (1);
	scan_line_into(cont, ctx);
	if (join_in_place(line, cont, sep) == -1)
		return (-1);
	return (0);
}

/**
 * @brief Handle trailing-backslash continuation by reading another
 *        physical line, joining without separator, and re-scanning the
 *        result FROM SCRATCH. The re-scan is necessary because a `<<`
 *        delim word can be split across the backslash-newline (e.g.
 *        `cat << EO\` + `F`); any pending heredoc captured from the
 *        partial first line had an incomplete delim and must be rebuilt
 *        from the joined buffer.
 * @return 0 on success, -1 on alloc failure, 1 on EOF mid-continuation.
 */
static int	handle_trailing_bs(t_shell *shell, char **line, t_lcont_ctx *ctx)
{
	char	*cont;

	ft_lstdel(&ctx->pending_hd, pending_hd_free);
	ctx->pending_hd = NULL;
	strip_trailing_bs(*line);
	cont = shell_read_line(shell, "> ");
	if (!cont)
		return (1);
	if (join_in_place(line, cont, "") == -1)
		return (-1);
	ft_bzero(ctx, sizeof(*ctx));
	scan_line_into(*line, ctx);
	return (0);
}

char	*shell_read_logical_line(t_shell *shell, const char *primary)
{
	char			*line;
	t_lcont_ctx		ctx;
	int				rc;

	line = shell_read_line(shell, primary);
	if (!line)
		return (NULL);
	ft_bzero(&ctx, sizeof(ctx));
	scan_line_into(line, &ctx);
	while (needs_continuation(&ctx))
	{
		if (ctx.trailing_bs)
		{
			rc = handle_trailing_bs(shell, &line, &ctx);
			if (rc == 1)
				return (line);
			if (rc == -1)
				return (NULL);
			continue ;
		}
		if (ctx.pending_hd)
		{
			if (drain_pending_heredocs(shell, &ctx) == -1)
				return (free(line), (char *)NULL);
			continue ;
		}
		rc = read_and_join_one(shell, &line, &ctx);
		if (rc == 1)
			return (line);
		if (rc == -1)
			return (NULL);
	}
	return (line);
}
