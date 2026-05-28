/**
 * @file expander.c
 * @brief Public expander API: expand_word, expand_word_to_fields,
 *        expand_command.
 * @author pulgamecanica
 *
 * Thin glue between the executor and the lower-level expansion
 * helpers.  All allocation/error contracts are documented in
 * expander.h; this file only implements them on top of the t_xbuf
 * and helpers from expand_word.c, expand_parameter.c, expand_tilde.c
 * and field_split.c.
 */

#include "42sh.h"
#include "expander.h"
#include "ast.h"
#include <stdlib.h>

char	*expand_word(t_shell *shell, const char *word)
{
	t_xbuf	buf;
	char	*result;

	if (!word)
		return (NULL);
	if (xbuf_init(&buf) == -1)
		return (NULL);
	if (expand_word_into(shell, word, &buf) == -1)
	{
		xbuf_free(&buf);
		return (NULL);
	}
	result = ft_strsub(buf.data, 0, buf.len);
	xbuf_free(&buf);
	return (result);
}

/**
 * @brief True if the raw token @p word contains at least one quote (` or " ),
 *        skipping past backslash-escapes so `\"` doesn't count.
 * @details Used to detect words like `""` or `"$X"` (X unset): their
 *          expansion is empty bytes-wise, yet POSIX 2.6.5 requires one
 *          empty argv field to be emitted (so `printf "[%s]" "$X" foo`
 *          prints `[][foo]`, not `[foo]`).
 */
static int	word_has_quote(const char *word)
{
	while (*word)
	{
		if (*word == '\\' && word[1])
		{
			word += 2;
			continue ;
		}
		if (*word == '\'' || *word == '"')
			return (1);
		word++;
	}
	return (0);
}

/**
 * @brief Produce an argv array of exactly one empty string (`{"", NULL}`).
 */
static char	**one_empty_field(void)
{
	char	**out;

	out = malloc(sizeof(char *) * 2);
	if (!out)
		return (NULL);
	out[0] = ft_strdup("");
	if (!out[0])
		return (free(out), NULL);
	out[1] = NULL;
	return (out);
}

char	**expand_word_to_fields(t_shell *shell, const char *word)
{
	t_xbuf	buf;
	char	**fields;

	if (!word)
		return (NULL);
	if (xbuf_init(&buf) == -1)
		return (NULL);
	if (expand_word_into(shell, word, &buf) == -1)
	{
		xbuf_free(&buf);
		return (NULL);
	}
	fields = field_split(shell, &buf);
	xbuf_free(&buf);
	if (fields && fields[0] == NULL && word_has_quote(word))
	{
		free(fields);
		return (one_empty_field());
	}
	return (fields);
}

/**
 * @brief Free a NULL-terminated argv array (each string + the array).
 */
static void	free_argv(char **argv)
{
	int	i;

	if (!argv)
		return ;
	i = 0;
	while (argv[i])
		free(argv[i++]);
	free(argv);
}

/**
 * @brief Append @p src (a NULL-terminated array) onto @p *dst, growing.
 * @details Takes ownership of @p src's elements; frees @p src itself.
 *          On failure both arrays are left in a recoverable state
 *          (@p src is freed, @p *dst keeps the elements collected so
 *          far so the caller can free it via free_argv).
 * @return New element count of @p *dst, or -1 on failure.
 */
static int	argv_append_all(char ***dst, int dst_count, char **src)
{
	int		src_count;
	int		i;
	char	**grown;

	src_count = 0;
	while (src && src[src_count])
		src_count++;
	grown = malloc(sizeof(char *) * (dst_count + src_count + 1));
	if (!grown)
	{
		i = 0;
		while (src && src[i])
			free(src[i++]);
		free(src);
		return (-1);
	}
	i = 0;
	while (i < dst_count)
	{
		grown[i] = (*dst)[i];
		i++;
	}
	i = 0;
	while (i < src_count)
	{
		grown[dst_count + i] = src[i];
		i++;
	}
	grown[dst_count + src_count] = NULL;
	free(*dst);
	free(src);
	*dst = grown;
	return (dst_count + src_count);
}

/**
 * @brief Replace cmd->argv with the field-split expansion of every word.
 */
static int	expand_argv(t_shell *shell, t_cmd *cmd)
{
	char	**new_argv;
	int		count;
	int		i;
	char	**fields;

	new_argv = malloc(sizeof(char *));
	if (!new_argv)
		return (-1);
	new_argv[0] = NULL;
	count = 0;
	i = 0;
	while (cmd->argv && cmd->argv[i])
	{
		fields = expand_word_to_fields(shell, cmd->argv[i]);
		if (!fields)
			return (free_argv(new_argv), -1);
		count = argv_append_all(&new_argv, count, fields);
		if (count == -1)
			return (free_argv(new_argv), -1);
		i++;
	}
	free_argv(cmd->argv);
	cmd->argv = new_argv;
	cmd->argc = count;
	return (0);
}

/**
 * @brief Rebuild "NAME=value" with @p value expanded as a single string.
 */
static char	*expand_assignment(t_shell *shell, const char *original)
{
	const char	*eq;
	char		*name;
	char		*expanded;
	char		*name_eq;
	char		*joined;

	eq = ft_strchr(original, '=');
	if (!eq)
		return (ft_strdup(original));
	name = ft_strsub(original, 0, eq - original);
	expanded = expand_word(shell, eq + 1);
	if (!name || !expanded)
		return (free(name), free(expanded), NULL);
	name_eq = ft_strjoin(name, "=");
	free(name);
	if (!name_eq)
		return (free(expanded), NULL);
	joined = ft_strjoin(name_eq, expanded);
	free(name_eq);
	free(expanded);
	return (joined);
}

/**
 * @brief Walk cmd->assignments and expand each value in place.
 */
static int	expand_assignments(t_shell *shell, t_cmd *cmd)
{
	t_list	*node;
	char	*rebuilt;

	node = cmd->assignments;
	while (node)
	{
		rebuilt = expand_assignment(shell, (const char *)node->content);
		if (!rebuilt)
			return (-1);
		free(node->content);
		node->content = rebuilt;
		node = node->next;
	}
	return (0);
}

/**
 * @brief True if @p type names a heredoc operator (whose target is the
 *        delimiter and must NOT be expanded as a filename).
 */
static int	is_heredoc(t_token_type type)
{
	return (type == TOK_HEREDOC || type == TOK_HEREDOC_STRIP);
}

/**
 * @brief Report "<target>: ambiguous redirect" to stderr.
 */
static void	report_ambiguous_redir(const char *target)
{
	ft_putstr_fd("42sh: ", 2);
	if (target)
		ft_putstr_fd(target, 2);
	ft_putendl_fd(": ambiguous redirect", 2);
}

/**
 * @brief Expand a redir target with field splitting and reject ambiguity.
 * @details After full expansion + field splitting, the target must
 *          collapse to exactly one field. Zero fields (e.g. an empty
 *          unquoted variable) and >1 fields are both rejected as
 *          ambiguous, matching bash behaviour.
 * @return Newly-allocated single-field string on success, or NULL on
 *         allocation failure / ambiguity (error already reported).
 */
static char	*expand_redir_target(t_shell *shell, const char *word)
{
	char	**fields;
	char	*result;
	int		count;

	fields = expand_word_to_fields(shell, word);
	if (!fields)
		return (NULL);
	count = 0;
	while (fields[count])
		count++;
	if (count != 1)
	{
		report_ambiguous_redir(word);
		count = 0;
		while (fields[count])
			free(fields[count++]);
		free(fields);
		return (NULL);
	}
	result = fields[0];
	free(fields);
	return (result);
}

/**
 * @brief Walk cmd->redirs and expand each non-heredoc target in place.
 */
static int	expand_redirs(t_shell *shell, t_cmd *cmd)
{
	t_list	*node;
	t_redir	*redir;
	char	*expanded;

	node = cmd->redirs;
	while (node)
	{
		redir = (t_redir *)node->content;
		if (redir && redir->target && !is_heredoc(redir->type))
		{
			expanded = expand_redir_target(shell, redir->target);
			if (!expanded)
				return (-1);
			free(redir->target);
			redir->target = expanded;
		}
		node = node->next;
	}
	return (0);
}

int	expand_command(t_shell *shell, t_cmd *cmd)
{
	if (!cmd)
		return (0);
	if (expand_argv(shell, cmd) == -1)
		return (-1);
	if (expand_assignments(shell, cmd) == -1)
		return (-1);
	if (expand_redirs(shell, cmd) == -1)
		return (-1);
	return (0);
}
