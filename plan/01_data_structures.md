# Core Data Structures

## Guiding Principles

1. **Use `t_list` (from libft) for all singly-linked collections.**
   No struct may have a raw `*next` pointer for list linkage.
   Traversal happens through `t_list *` fields on the owning struct.
   Access pattern: `(t_var *)node->content` (direct cast, see note below).

2. **Use `t_dlist` (project-defined in `includes/dlist.h`) for doubly-linked needs.**
   Currently used for history navigation only. Same content-as-pointer convention.

3. **Use `ft_array` (from libft) for dynamic arrays built incrementally.**
   Primary use: building `argv[]` during parsing (`ft_arrappend_raw`), glob results,
   and field-splitting output.
   Final result is converted to `char **` (NULL-terminated) for execve.

4. **Use plain `char **` where POSIX demands it.**
   `argv` in `t_cmd`, environment passed to execve.  These are the terminal
   form; `ft_array` is only the build step.

### t_list storage convention

`ft_lstnew(&ptr, sizeof(ptr))` copies the _pointer value_ into `node->content`
(an 8-byte allocation on 64-bit).  The correct read pattern is therefore:

```c
t_var *v = *(t_var **)node->content;
```

The LST_VAR / LST_JOB / LST_ALIAS / LST_PROC macros in `42sh.h` do this for
the common types.  For one-off casts, use `*(T **)node->content`.

---

## 1. Token (Lexer Output)

```c
typedef enum e_token_type
{
    TOK_WORD,           // Regular word/argument (quotes preserved in value)
    TOK_PIPE,           // |
    TOK_AND,            // &&
    TOK_OR,             // ||
    TOK_SEMICOLON,      // ;
    TOK_AMPERSAND,      // & (background)
    TOK_NEWLINE,        // \n
    TOK_REDIR_IN,       // <
    TOK_REDIR_OUT,      // >
    TOK_REDIR_APPEND,   // >>
    TOK_HEREDOC,        // <<
    TOK_REDIR_DUP_IN,   // <&
    TOK_REDIR_DUP_OUT,  // >&
    TOK_LPAREN,         // (
    TOK_RPAREN,         // )
    TOK_EOF,
    TOK_ERROR
}   t_token_type;

// Token data - NO *next pointer.
typedef struct s_token
{
    t_token_type    type;
    char            *value;         // Raw string (quotes preserved for expander)
    int             io_number;      // fd before redirect (-1 if none)
}   t_token;
```

**Lexer returns `t_list *`** (each node contains a `t_token *`).
Access: `TOK(node)` macro → `(t_token *)node->content`.
Free: `lexer_free_tokens(t_list *tokens)`.

---

## 2. AST Nodes (Parser Output)

```c
// Redirection data - NO *next pointer.
typedef struct s_redir
{
    t_token_type    type;
    int             fd;                 // Source fd (-1 = default)
    char            *target;            // Filename or fd string (unexpanded)
    char            *heredoc_delim;
    char            *heredoc_content;   // Filled by heredoc collection pass
    int             heredoc_quoted;     // 1 = no expansion inside heredoc
}   t_redir;

// Simple command.
typedef struct s_cmd
{
    char    **argv;         // NULL-terminated, unexpanded (for execve)
    int     argc;
    t_list  *assignments;   // list of char* "NAME=val" - NO char **
    t_list  *redirs;        // list of t_redir*
}   t_cmd;

// Binary node (pipe, &&, ||, ;)
typedef struct s_binary
{
    struct s_ast    *left;
    struct s_ast    *right;
}   t_binary;

// Group node (subshell, block, background)
typedef struct s_group
{
    struct s_ast    *child;
    t_list          *redirs;    // list of t_redir* for the whole group
}   t_group;

typedef struct s_ast
{
    t_node_type type;
    union {
        t_cmd       cmd;
        t_binary    binary;
        t_group     group;
    } data;
}   t_ast;
```

**Design decisions:**
- `argv` is a `char **` because that's what execve needs; build it from
  `ft_array` in the parser and call `ft_arrdel` when done building.
- `assignments` and `redirs` use `t_list *` (no raw next pointer).
- `REDIR(node)` macro → `(t_redir *)node->content`.

---

## 3. Variable Storage

```c
// Variable data - NO *next pointer.
typedef struct s_var
{
    char    *name;
    char    *value;
    int     exported;
    int     readonly;
}   t_var;
```

Stored as `t_list *variables` in `t_shell`.
Access: `LST_VAR(node)` → `*(t_var **)node->content`.

For O(1) lookup a hash table variant may be added later, but the linked list
is the baseline.

---

## 4. Job Control

```c
// Process in a pipeline - NO *next pointer.
typedef struct s_process
{
    pid_t   pid;
    char    *cmd;       // command string for display
    int     status;     // raw waitpid status
    int     completed;
    int     stopped;
}   t_process;

// Job - NO *next pointer.
typedef struct s_job
{
    int             id;
    pid_t           pgid;
    char            *cmd_line;
    t_list          *processes;     // list of t_process*
    t_job_status    status;
    int             notified;
    int             foreground;
}   t_job;
```

Stored as `t_list *jobs` in `t_shell`.
Access: `LST_JOB(node)` → `*(t_job **)node->content`.
`LST_PROC(node)` → `*(t_process **)node->content` (for `job->processes`).

---

## 5. History

```c
// History entry data - stored in t_dlist nodes (doubly-linked for navigation).
typedef struct s_history_entry
{
    int     number;     // monotonically increasing history number (1-based)
    char    *line;      // command line string (heap-allocated)
}   t_history_entry;

typedef struct s_history
{
    t_dlist *head;      // oldest entry  (t_dlist; content = t_history_entry*)
    t_dlist *tail;      // newest entry
    t_dlist *current;   // navigation cursor (NULL when not navigating)
    int     count;
    int     max_size;
    int     next_number;
    char    *file_path;
}   t_history;
```

**Why `t_dlist`?** Arrow key navigation requires both forward (Down) and
backward (Up) traversal.  A singly-linked `t_list` would require storing a
reverse pointer separately; `t_dlist` is the natural fit.

Access: `(t_history_entry *)node->content` (no double-deref needed here since
`ft_dlstnew(ptr)` stores the pointer directly).

---

## 6. Line Editor State

With readline handling raw mode, key reading, buffer, and display, the
`t_line_editor` struct is lean:

```c
typedef struct s_line_editor
{
    t_history   *history;       // pointer into t_shell.history
    char        *saved_line;    // buffer snapshot before history navigation
    int         editing_mode;   // LE_MODE_EMACS (0) or LE_MODE_VI (1)
}   t_line_editor;
```

All terminal state is managed by readline internally.  We configure readline
via `rl_variable_bind()`, readline callbacks, and `rl_attempted_completion_function`.

---

## 7. Alias

```c
// Alias data - NO *next pointer.
typedef struct s_alias
{
    char    *name;
    char    *value;
}   t_alias;
```

Stored as `t_list *aliases` in `t_shell`.
Access: `LST_ALIAS(node)` → `*(t_alias **)node->content`.

---

## 8. Hash Table for Command Cache (Modular Feature)

```c
typedef struct s_hash_entry
{
    char                *name;
    char                *path;
    int                 hits;
    struct s_hash_entry *next;  // internal chaining within a bucket (not a project-wide list)
}   t_hash_entry;

typedef struct s_hash_table
{
    t_hash_entry    **buckets;
    size_t          size;
}   t_hash_table;
```

The internal bucket chaining uses a raw `*next` because this is a hash-specific
structure (not a general project list) and the performance trade-off justifies it.

---

## Memory Management Guidelines

| Structure | When to free |
|-----------|-------------|
| Token list | After `parser_parse()` consumes it |
| AST | After `executor_execute()` completes |
| Variables | On `unset` or shell exit |
| Jobs | When job is done, reported, and removed from list |
| History entries | On eviction (max_size) or shell exit |
| argv `char **` | After executor no longer needs the command |
| ft_array used to build argv | Right after `char **` is extracted |

### Helper functions to implement:

```c
// t_list node helpers (in src/utils/list_utils.c)
t_list      *lst_new_ptr(void *ptr);            // malloc node, store ptr directly
void        lst_free_ptr(t_list **lst,          // free list; del frees content
                         void (*del)(void *));

// Token
t_token     *token_new(t_token_type type, char *value);
void        token_free(t_token *token);
void        lexer_free_tokens(t_list *head);

// AST
t_ast       *ast_new_command(t_cmd *cmd);
t_ast       *ast_new_binary(t_node_type type, t_ast *left, t_ast *right);
t_ast       *ast_new_group(t_node_type type, t_ast *child, t_list *redirs);
void        ast_free(t_ast *node);

// Redirection
t_redir     *redir_new(t_token_type type, int fd, char *target);
void        redir_free(t_redir *redir);

// Variables
t_var       *var_get(t_shell *shell, const char *name);
char        *var_get_value(t_shell *shell, const char *name);
int         var_set(t_shell *shell, const char *name, const char *value);
int         var_unset(t_shell *shell, const char *name);
int         var_export(t_shell *shell, const char *name);

// argv building during parsing
char        **argv_from_array(t_array *arr);    // NULL-terminate and extract
```
