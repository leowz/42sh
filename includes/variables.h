/**
 * @file variables.h
 * @brief Shell variable and alias data structures, plus the variable management
 *        API (set, unset, export, environ rebuild) used by the rest of the shell.
 * @author wengzhang, pulgamecanica
 *
 * Variables and aliases are stored on the central `t_shell` struct as
 * `t_list*` chains of `t_var*` / `t_alias*` content pointers. This header
 * owns those node types, the casting macros used to walk them, and every
 * mutator that touches the variable table (create, set, unset, export,
 * environ rebuild). The full `t_shell` definition lives in `42sh.h`; this
 * header forward-declares it so callers that only need the variable API
 * can avoid pulling the full shell state header.
 */

#ifndef VARIABLES_H
#define VARIABLES_H

/** Forward declaration to avoid circular dependency with 42sh.h. */
typedef struct s_shell t_shell;

/**
 * @brief Shell variable data node.
 * @details One instance per named variable. Stored in
 *          `t_shell.variables` as the `content` of a `t_list` node;
 *          traversal is done via that wrapper, which is why this struct
 *          carries no `next` pointer of its own. Both `name` and
 *          `value` are heap-owned by this module.
 */
typedef struct s_var {
  char *name;     /**< Variable name (heap-owned, never NULL once set). */
  char *value;    /**< Variable value, or NULL for "exported but unset". */
  int  exported; /**< 1 when this name should appear in `execve`'s envp. */
  int  readonly; /**< 1 when set/unset should refuse writes. */
} t_var;

/**
 * @brief Alias data node.
 * @details Stored in `t_shell.aliases` as the `content` of a `t_list`
 *          node. Like `t_var`, the linked-list plumbing is provided by
 *          the wrapping `t_list`, not by this struct.
 */
typedef struct s_alias {
  char *name;  /**< Alias name (heap-owned). */
  char *value; /**< Replacement text (heap-owned). */
} t_alias;

/** Casts a `t_list` node's content to `t_var *`. */
#define LST_VAR(n) ((t_var *)(n)->content)

/** Casts a `t_list` node's content to `t_alias *`. */
#define LST_ALIAS(n) ((t_alias *)(n)->content)

/** Get the value string of variable `name`, or NULL if unset. */
char *var_get_value(t_shell *shell, const char *name);

/** Get the `t_var` node for `name`, or NULL if unset. */
t_var *var_get(t_shell *shell, const char *name);

/** Set (or create) variable `name` to `value`. Returns 0 on success. */
int var_set(t_shell *shell, const char *name, const char *value);

/** Remove variable `name`. Returns 0 on success. */
int var_unset(t_shell *shell, const char *name);

/** Mark variable `name` for export to child processes. */
int var_export(t_shell *shell, const char *name);

/** Return (and cache) the `NULL`-terminated `envp` array for execve. */
char **var_get_environ(t_shell *shell);

/** Populate `shell->variables` from the process's initial `envp`. */
void var_init_from_environ(t_shell *shell, char **envp);

#endif
