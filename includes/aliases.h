/**
 * @file aliases.h
 * @brief Alias table API: create, look up and remove shell aliases, plus
 *        the command-word alias-expansion pass.
 * @author zweng
 *
 * Aliases are stored on `t_shell.aliases` as a `t_list*` chain of
 * `t_alias*` content pointers (the `t_alias` node type lives in
 * `variables.h`). This header owns every mutator that touches that table.
 */

#ifndef ALIASES_H
#define ALIASES_H

#include "libft.h"
#include "variables.h"

/** Look up alias @p name; returns the `t_alias` node, or NULL if unset. */
t_alias	*alias_get(t_shell *shell, const char *name);

/** Replacement text of alias @p name, or NULL if @p name is not an alias. */
char	*alias_get_value(t_shell *shell, const char *name);

/** Define or update alias @p name to @p value. Returns 0 on success. */
int		alias_set(t_shell *shell, const char *name, const char *value);

/** Remove alias @p name. Returns 0 if removed, 1 if it did not exist. */
int		alias_unset(t_shell *shell, const char *name);

/** Remove every alias (used by `unalias -a` and shell shutdown). */
void	alias_clear(t_shell *shell);

/**
 * @brief Expand command-position aliases in @p tokens, in place.
 * @details A `TOK_WORD` that starts a simple command and is an unquoted
 *          match for an alias is replaced by the tokens of the alias
 *          value. Runs between `lexer_tokenize` and `parser_parse`.
 * @return 0 always.
 */
int		alias_expand_tokens(t_shell *shell, t_list **tokens);

#endif
