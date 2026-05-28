/**
 * @file expand_cmdsub.c
 * @brief POSIX `$( ... )` command substitution.
 * @author zweng
 *
 * Run the inner script in a forked child whose stdout is a pipe to the
 * parent; the captured output (with trailing newlines stripped per POSIX
 * 2.6.3) is spliced into the surrounding word. Quote context matters:
 * unquoted output is subject to field splitting (split mask = 1),
 * double-quoted output is not (split mask = 0).
 *
 * Reuses the parent shell's lexer/parser/executor on the inner script
 * so variables, aliases, builtins and exported state all match what the
 * user would see typing the same line at the prompt.
 */

#include "42sh.h"
#include "expander.h"
#include "lexer.h"
#include "parser.h"
#include "executor.h"
#include <sys/wait.h>
#include <unistd.h>

/**
 * @brief Locate the matching `)` for an opening `$(` already consumed.
 * @details Tracks nested `(`/`)` and respects single/double quotes so
 *          `$(echo "(hello)")` is captured correctly. `start` is the
 *          index of the first byte INSIDE the `$(`. Returns the index of
 *          the matching `)`, or (size_t)-1 if unbalanced.
 */
static size_t	find_close(const char *input, size_t start)
{
	size_t	i;
	int		depth;
	int		in_sq;
	int		in_dq;

	i = start;
	depth = 1;
	in_sq = 0;
	in_dq = 0;
	while (input[i])
	{
		if (input[i] == '\\' && input[i + 1] && !in_sq)
		{
			i += 2;
			continue ;
		}
		if (input[i] == '\'' && !in_dq)
			in_sq = !in_sq;
		else if (input[i] == '"' && !in_sq)
			in_dq = !in_dq;
		else if (!in_sq && !in_dq)
		{
			if (input[i] == '(')
				depth++;
			else if (input[i] == ')' && --depth == 0)
				return (i);
		}
		i++;
	}
	return ((size_t)-1);
}

/**
 * @brief Strip trailing newlines from @p buf in place (POSIX 2.6.3).
 */
static void	strip_trailing_newlines(char *buf, size_t *len)
{
	while (*len > 0 && buf[*len - 1] == '\n')
		(*len)--;
	buf[*len] = '\0';
}

/**
 * @brief Drain the read end of @p pipefd into a freshly malloc'd buffer.
 * @return NULL on alloc failure; otherwise a NUL-terminated string whose
 *         length is written to @p out_len (trailing `\n` already stripped).
 */
static char	*read_all_from_pipe(int rfd, size_t *out_len)
{
	char	*buf;
	char	*grown;
	size_t	cap;
	size_t	len;
	ssize_t	n;

	cap = 4096;
	len = 0;
	buf = malloc(cap);
	if (!buf)
		return (NULL);
	while (1)
	{
		if (len + 1 >= cap)
		{
			cap *= 2;
			grown = realloc(buf, cap);
			if (!grown)
			{
				free(buf);
				return (NULL);
			}
			buf = grown;
		}
		n = read(rfd, buf + len, cap - len - 1);
		if (n <= 0)
			break ;
		len += (size_t)n;
	}
	buf[len] = '\0';
	strip_trailing_newlines(buf, &len);
	*out_len = len;
	return (buf);
}

/**
 * @brief Child half: redirect stdout to the pipe and run @p script through
 *        the parent shell's lexer/parser/executor.
 * @details Exits with the inner script's last exit status. On any
 *          allocation or parse failure, exits non-zero so the parent
 *          sees a sensible status if it cares (we don't, today, but it
 *          keeps the contract honest).
 */
static void	run_in_child(t_shell *shell, char *script, int wfd)
{
	t_list	*tokens;
	t_ast	*ast;

	if (dup2(wfd, STDOUT_FILENO) == -1)
		_exit(1);
	close(wfd);
	tokens = lexer_tokenize(script);
	if (!tokens)
		_exit(1);
	alias_expand_tokens(shell, &tokens);
	ast = parser_parse(tokens, shell);
	lexer_free_tokens(tokens);
	if (!ast)
		_exit(shell->last_exit_status ? shell->last_exit_status : 1);
	executor_execute(shell, ast);
	ast_free(ast);
	_exit(shell->last_exit_status);
}

/**
 * @brief Fork, run @p script in the child, capture its stdout, return it.
 * @return malloc'd captured string (trailing newlines stripped), or NULL.
 *         Length is written to *out_len.
 */
static char	*capture_substitution(t_shell *shell, char *script, size_t *out_len)
{
	int		pipefd[2];
	pid_t	pid;
	char	*captured;
	int		status;

	if (pipe(pipefd) == -1)
		return (NULL);
	pid = fork();
	if (pid == -1)
	{
		close(pipefd[0]);
		close(pipefd[1]);
		return (NULL);
	}
	if (pid == 0)
	{
		close(pipefd[0]);
		run_in_child(shell, script, pipefd[1]);
	}
	close(pipefd[1]);
	captured = read_all_from_pipe(pipefd[0], out_len);
	close(pipefd[0]);
	waitpid(pid, &status, 0);
	if (WIFEXITED(status))
		shell->last_exit_status = WEXITSTATUS(status);
	return (captured);
}

/**
 * @brief Handle a `$(...)` sitting at @c input[*pos] (the `$` itself).
 * @details Caller (expand_dollar) has already confirmed `input[*pos+1] ==
 *          '('` AND `input[*pos+2] != '('` (the arithmetic case is routed
 *          elsewhere). Advances *pos past the closing `)`. The captured
 *          output is pushed with split=!dq so an unquoted result still
 *          undergoes field splitting downstream.
 */
int	expand_cmdsub(t_shell *shell, const char *input,
		size_t *pos, int dq, t_xbuf *out)
{
	size_t	inner_start;
	size_t	inner_end;
	char	*inner;
	char	*captured;
	size_t	captured_len;
	int		rc;

	inner_start = *pos + 2;
	inner_end = find_close(input, inner_start);
	if (inner_end == (size_t)-1)
	{
		fprintf(stderr, "42sh: unmatched `$(`\n");
		shell->last_exit_status = 2;
		return (-1);
	}
	inner = ft_strsub(input, inner_start, inner_end - inner_start);
	if (!inner)
		return (-1);
	captured = capture_substitution(shell, inner, &captured_len);
	free(inner);
	if (!captured)
	{
		*pos = inner_end + 1;
		return (0);
	}
	rc = xbuf_puts(out, captured, !dq);
	free(captured);
	*pos = inner_end + 1;
	return (rc);
}
