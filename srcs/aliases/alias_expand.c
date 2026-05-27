/**
 * @file alias_expand.c
 * @brief Command-word alias expansion, applied to the token stream
 *        between lexing and parsing.
 * @author zweng
 *
 * A TOK_WORD in command position (the start of a simple command) whose
 * raw, unquoted text matches an alias is replaced by the tokens of the
 * alias value. The replacement's own first word is re-examined so alias
 * chains (a -> b -> cmd) resolve, while a per-command-word "seen" set
 * stops self-referential aliases (alias ls='ls -la') and a global budget
 * caps pathological recursion through aliases whose value holds operators.
 */
#include "42sh.h"
#include "aliases.h"
#include "lexer.h"
#include <stdlib.h>

#define ALIAS_SEEN_MAX 64
#define ALIAS_BUDGET 256

/** Mutable state threaded through the token scan. */
typedef struct s_alias_scan
{
	t_list	*cur;        /**< Token node currently under examination. */
	char	**seen;      /**< Alias names expanded for this command word. */
	int		seen_n;      /**< Number of entries in `seen`. */
	int		cmd_pos;     /**< 1 when the next word starts a command. */
	int		skip_target; /**< 1 when the next word is a redirect target. */
	int		budget;      /**< Total expansions done (global safety cap). */
}	t_alias_scan;

/** True for redirect operators - the token right after them is a target. */
static int	is_redir_tok(t_token_type t)
{
	return (t == TOK_REDIR_IN || t == TOK_REDIR_OUT || t == TOK_REDIR_APPEND
		|| t == TOK_HEREDOC || t == TOK_HEREDOC_STRIP
		|| t == TOK_REDIR_DUP_IN || t == TOK_REDIR_DUP_OUT);
}

/** True for operators after which the next word starts a new command. */
static int	starts_command(t_token_type t)
{
	return (t == TOK_PIPE || t == TOK_AND || t == TOK_OR
		|| t == TOK_SEMICOLON || t == TOK_AMPERSAND
		|| t == TOK_NEWLINE || t == TOK_LPAREN);
}

/** True if @p v is non-empty and carries no quote/backslash byte - i.e.
 *  an unquoted word, the only kind eligible for alias expansion. */
static int	is_plain_word(const char *v)
{
	int	i;

	if (!v || !*v)
		return (0);
	i = 0;
	while (v[i])
	{
		if (v[i] == '\'' || v[i] == '"' || v[i] == '\\')
			return (0);
		i++;
	}
	return (1);
}

/**
 * @brief Drop and free the trailing TOK_EOF node of a tokenized list.
 * @return The list head, or NULL if it held only the EOF token.
 */
static t_list	*strip_eof(t_list *sub)
{
	t_list	*prev;
	t_list	*node;

	prev = NULL;
	node = sub;
	while (node && TOK(node)->type != TOK_EOF)
	{
		prev = node;
		node = node->next;
	}
	if (!node)
		return (sub);
	if (prev)
		prev->next = NULL;
	else
		sub = NULL;
	token_free(TOK(node));
	free(node);
	return (sub);
}

/**
 * @brief Replace node @p cur with the tokens of @p value. @p prev is the
 *        node before @p cur (NULL when @p cur is the list head).
 * @return The node to resume scanning from: the first spliced node, or
 *         the tail when the alias value expanded to nothing.
 */
static t_list	*splice_alias(t_list **tokens, t_list *prev,
		t_list *cur, const char *value)
{
	t_list	*tail;
	t_list	*sub;
	t_list	*last;

	tail = cur->next;
	token_free(TOK(cur));
	free(cur);
	sub = strip_eof(lexer_tokenize(value));
	if (!sub)
	{
		if (prev)
			return (prev->next = tail, tail);
		return (*tokens = tail, tail);
	}
	last = sub;
	while (last->next)
		last = last->next;
	last->next = tail;
	if (prev)
		prev->next = sub;
	else
		*tokens = sub;
	return (sub);
}

/** Linear search of the per-command-word expansion history. */
static int	seen_has(char **seen, int n, const char *name)
{
	while (n-- > 0)
		if (ft_strequ(seen[n], name))
			return (1);
	return (0);
}

/** Free and reset the expansion-history set. */
static void	seen_reset(char **seen, int *n)
{
	while (*n > 0)
		free(seen[--(*n)]);
}

/**
 * @brief One pass of the scan loop over node @p cur. Updates the scan
 *        state and returns the node to continue from (after a splice the
 *        same node is revisited so alias chains resolve).
 */
static t_list	*alias_scan_step(t_shell *shell, t_list **tokens,
		t_list *prev, t_alias_scan *s)
{
	char	*value;
	t_list	*cur;

	cur = s->cur;
	if (TOK(cur)->type == TOK_WORD && s->skip_target)
		s->skip_target = 0;
	else if (TOK(cur)->type == TOK_WORD && s->cmd_pos)
	{
		value = NULL;
		if (s->budget < ALIAS_BUDGET && s->seen_n < ALIAS_SEEN_MAX
			&& is_plain_word(TOK(cur)->value)
			&& !seen_has(s->seen, s->seen_n, TOK(cur)->value))
			value = alias_get_value(shell, TOK(cur)->value);
		if (value)
		{
			s->seen[s->seen_n++] = ft_strdup(TOK(cur)->value);
			s->budget++;
			return (splice_alias(tokens, prev, cur, value));
		}
		s->cmd_pos = 0;
		seen_reset(s->seen, &s->seen_n);
	}
	else if (is_redir_tok(TOK(cur)->type))
		s->skip_target = 1;
	else if (starts_command(TOK(cur)->type))
	{
		s->cmd_pos = 1;
		seen_reset(s->seen, &s->seen_n);
	}
	else if (TOK(cur)->type == TOK_RPAREN)
		s->cmd_pos = 0;
	return (NULL);
}

int	alias_expand_tokens(t_shell *shell, t_list **tokens)
{
	char			*seen[ALIAS_SEEN_MAX];
	t_alias_scan	s;
	t_list			*prev;
	t_list			*spliced;

	if (!shell || !tokens)
		return (0);
	s.cmd_pos = 1;
	s.skip_target = 0;
	s.seen_n = 0;
	s.budget = 0;
	s.seen = seen;
	s.cur = *tokens;
	prev = NULL;
	while (s.cur)
	{
		spliced = alias_scan_step(shell, tokens, prev, &s);
		if (spliced)
		{
			s.cur = spliced;
			continue ;
		}
		prev = s.cur;
		s.cur = s.cur->next;
	}
	seen_reset(seen, &s.seen_n);
	return (0);
}
