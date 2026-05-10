/**
 * @file expander.h
 * @brief Word expansion: quotes, parameter, tilde, field splitting.
 * @author pulgamecanica
 *
 * The expander is invoked by the executor right before a simple command
 * runs. It performs the POSIX expansion pipeline on the raw token strings
 * produced by the lexer/parser:
 *
 *   1. Tilde expansion        ~  ~user
 *   2. Parameter expansion    $VAR ${VAR} $? $$ $0
 *   3. Quote handling         ' " \ semantics
 *   4. Field splitting        on $IFS (only on unquoted expansion output)
 *   5. Quote removal          done in-line during the char-by-char loop
 *
 * Command substitution `$()`, arithmetic `$(())` and pathname (glob)
 * expansion are listed as modular features in plan/04_expander.md and
 * are not implemented in this module yet.
 */

#ifndef EXPANDER_H
#define EXPANDER_H

#include "ast.h"
#include <stddef.h>

struct s_shell;

/**
 * @brief Expansion buffer with a parallel "splittable" mask.
 * @details Phase 1 of expansion writes into a t_xbuf instead of a plain
 *          C string so that field splitting can later distinguish IFS
 *          characters that came from an unquoted expansion (splittable)
 *          from IFS characters that were literal-quoted in the source
 *          word (not splittable).
 *
 * @details For every byte stored in @c data there is a parallel byte in
 *          @c mask: 1 means "IFS-splittable", 0 means "literal - never a field boundary".
 * @note Both buffers are NUL-terminated so the
 * payload can be inspected with the regular string functions.
 */
typedef struct s_xbuf {
  char *data; /**< NUL-terminated expanded text. */
  char *mask; /**< Parallel mask: 1 = splittable, 0 = literal. */
  size_t len; /**< Number of bytes currently stored (excluding NUL). */
  size_t cap; /**< Allocated capacity of @c data and @c mask. */
} t_xbuf;

/**
 * @brief Expand a word to a single string (no field splitting, no globbing).
 * @details Used for assignment values and redirection targets.
 * @param shell The shell instance.
 * @param word The raw token string from the parser.
 * @return Newly-allocated string. Caller frees. NULL on allocation failure
 *         or when @p word is NULL.
 */
char *expand_word(struct s_shell *shell, const char *word);

/**
 * @brief Expand a word to multiple fields (argv words).
 * @details Performs all expansions plus field splitting on $IFS.
 * @param shell The shell instance.
 * @param word The raw token string from the parser.
 * @return Newly-allocated NULL-terminated array of strings. Caller frees
 *         each element and the array itself. May return a zero-element
 *         array (just a NULL terminator) when the word expanded to nothing
 *         splittable. NULL on allocation failure or when @p word is NULL.
 */
char **expand_word_to_fields(struct s_shell *shell, const char *word);

/**
 * @brief Expand a whole simple command in place: argv, assignments, redirs.
 * @details Replaces @c cmd->argv with a new array containing the expanded
 *          fields, updates @c cmd->argc, rewrites each "NAME=value" in
 *          @c cmd->assignments, and rewrites @c redir->target on every
 *          non-heredoc redirection.
 * @param shell The shell instance.
 * @param cmd The command node to expand.
 * @return 0 on success, -1 on allocation failure.
 */
int expand_command(struct s_shell *shell, t_cmd *cmd);

/** @brief Initialise an empty expansion buffer. Returns 0 / -1. */
int xbuf_init(t_xbuf *buf);

/** @brief Release a buffer's storage. Safe to call on a NULL/empty buf. */
void xbuf_free(t_xbuf *buf);

/**
 * @brief Append one byte plus its split-mask flag.
 * @param buf The buffer to append to.
 * @param c The character to append.
 * @param split 1 if @p c is subject to IFS splitting, 0 if literal.
 * @return 0 on success, -1 on allocation failure.
 */
int xbuf_putc(t_xbuf *buf, char c, char split);

/**
 * @brief Append a NUL-terminated string with a uniform split flag.
 * @param buf The buffer to append to.
 * @param s The string to append (may be NULL → no-op success).
 * @param split 1 if every byte of @p s is splittable, 0 if literal.
 * @return 0 on success, -1 on allocation failure.
 */
int xbuf_puts(t_xbuf *buf, const char *s, char split);

/**
 * @brief Run the char-by-char expansion loop on @p word into @p out.
 * @details Handles single quotes (no expansion), double quotes (parameter
 *          expansion only), tilde at word start, $-expansions, and
 *          backslash escapes per POSIX.
 * @return 0 on success, -1 on allocation failure.
 */
int expand_word_into(struct s_shell *shell, const char *word, t_xbuf *out);

/**
 * @brief Read a $... sequence beginning at @c input[*pos].
 * @details Supports $?, $$, $0, $NAME and ${NAME} (including ${?}, ${$},
 *          ${0}). Unknown variables expand to the empty string. A bare $
 *          followed by no recognised form is emitted literally.
 * @param shell The shell instance (for variable lookup and $?).
 * @param input The full word string.
 * @param pos In/out: byte index, advanced past the consumed sequence.
 * @param dq 1 when the $-sequence is inside double quotes (suppresses split).
 * @param out Buffer to append the expanded value to.
 * @return 0 on success, -1 on allocation failure.
 */
int expand_dollar(struct s_shell *shell, const char *input, size_t *pos, int dq,
                  t_xbuf *out);

/**
 * @brief Read a tilde sequence beginning at @c input[*pos].
 * @details Recognises a leading "~" or "~user" and replaces it with $HOME
 *          or the named user's home directory. If neither resolves, the
 *          original text is emitted literally.
 * @return 0 on success, -1 on allocation failure.
 */
int expand_tilde_at(struct s_shell *shell, const char *input, size_t *pos,
                    t_xbuf *out);

/**
 * @brief Field-split an already-expanded buffer on $IFS.
 * @details Honours the buffer's split mask: a character is a candidate
 *          delimiter only when both (a) it appears in $IFS and (b) its
 *          mask byte is 1.  Empty $IFS disables splitting entirely.
 * @return Newly-allocated NULL-terminated array of strings, or NULL on
 *         allocation failure.
 */
char **field_split(struct s_shell *shell, const t_xbuf *expanded);

#endif
