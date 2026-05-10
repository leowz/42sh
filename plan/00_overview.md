# 42sh Architecture Overview

## Philosophy

1. **Stability over features** - A shell that never crashes is better than one with many features that segfaults
2. **Modular design** - Each component should be independent and testable
3. **Clear interfaces** - Well-defined boundaries between modules enable parallel development
4. **Reference: bash** - When in doubt, follow bash behavior

## High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                            42sh Main Loop                           │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                          LINE EDITOR                                │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐                  │
│  │   Termcap   │  │   History   │  │  Input Buf  │                  │
│  └─────────────┘  └─────────────┘  └─────────────┘                  │
└─────────────────────────────────────────────────────────────────────┘
                                    │ char *line
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                             LEXER                                   │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐                  │
│  │ State Mach. │  │  Tokenizer  │  │ Quote Track │                  │
│  └─────────────┘  └─────────────┘  └─────────────┘                  │
└─────────────────────────────────────────────────────────────────────┘
                                    │ t_list *tokens   (each node: t_token*)
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                             PARSER                                  │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐                  │
│  │  Grammar    │  │ AST Builder │  │Syntax Check │                  │
│  └─────────────┘  └─────────────┘  └─────────────┘                  │
└─────────────────────────────────────────────────────────────────────┘
                                    │ t_ast *ast
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                       HEREDOC COLLECTION                            │
│  Walk AST, read heredoc content from input for each << redirect     │
└─────────────────────────────────────────────────────────────────────┘
                                    │ t_ast *ast (with heredoc content)
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                            EXECUTOR                                 │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌───────────┐   │
│  │  Commands   │  │   Pipes     │  │ Redirects   │  │  Builtins │   │
│  └─────────────┘  └─────────────┘  └─────────────┘  └───────────┘   │
│                                                                     │
│  ┌─────────────────────────────────────────────────────────────┐    │
│  │  EXPANDER (called per-command, not as a separate pass)      │    │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌───────────────┐   │    │
│  │  │ Var Exp. │ │ Tilde    │ │ Globbing │ │ Quote Removal │   │    │
│  │  └──────────┘ └──────────┘ └──────────┘ └───────────────┘   │    │
│  └─────────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                    ┌───────────────┼───────────────┐
                    ▼               ▼               ▼
            ┌─────────────┐ ┌─────────────┐ ┌─────────────┐
            │ Job Control │ │  Variables  │ │   Signals   │
            └─────────────┘ └─────────────┘ └─────────────┘
```

**Key design decision:** The expander is NOT a separate pipeline pass. It is a service called by the executor for each command's words right before that command runs. This is required because `$?` and other variables must reflect the state at execution time, not parse time.

## Data Flow

1. **Input**: Line editor reads user input character by character
2. **Lexing**: Raw string is converted to a linked list of tokens (quotes preserved in values)
3. **Parsing**: Tokens are organized into an Abstract Syntax Tree (AST)
4. **Heredoc collection**: Walk AST, read heredoc content for each `<<` redirect
5. **Execution**: AST is walked; each command's words are expanded immediately before that command runs
6. **Result**: Exit status is captured, shell state is updated

## Module Responsibilities

| Module | Input | Output | Responsibility |
|--------|-------|--------|----------------|
| Line Editor | keystrokes | `char *line` | Interactive input with editing |
| Lexer | `char *line` | `t_list *tokens` | Tokenization (preserves quotes in values; each node is `t_token *`) |
| Parser | `t_token *list` | `t_ast *tree` | Build syntax tree, detect assignments |
| Expander | `char *word` | `char **fields` | Variable/tilde/glob expansion (called by executor) |
| Executor | `t_ast *tree` | `int exit_status` | Walk AST, expand per-command, run commands |
| Builtins | `char **argv` | `int exit_status` | Built-in commands |
| Variables | get/set requests | values | State management |
| Job Control | process events | job status | Background jobs |
| Signals | OS signals | actions | Signal handling |

## Modes of Operation

The shell operates in two modes:

1. **Interactive mode** (`isatty(stdin)`): Show prompt, use line editor, handle signals for user
2. **Non-interactive mode** (`42sh -c "cmd"` or piped input): Read from string/stdin, no prompt, no line editor. Essential for testing.

## Shared State: t_shell

All modules access a shared shell state structure.

**List conventions** (see `01_data_structures.md`):
- `t_list *variables` - list of `t_var *` (use `LST_VAR(node)`)
- `t_list *jobs` - list of `t_job *` (use `LST_JOB(node)`)
- `t_list *aliases` - list of `t_alias *` (use `LST_ALIAS(node)`)
- No raw `*next` pointers in any of these structs.

```c
typedef struct s_shell
{
    // Variables (P3)
    t_list          *variables;         // list of t_var* (no raw *next in t_var)
    char            **env;              // Cached NULL-terminated array for execve
    int             env_dirty;          // 1 = rebuild env before next execve

    // State
    int             last_exit_status;   // $?
    int             interactive;        // isatty(STDIN_FILENO)
    int             running;            // main loop flag
    int             exit_confirmed;     // double-exit guard with stopped jobs

    // Job Control (P4)
    t_list          *jobs;              // list of t_job* (no raw *next in t_job)
    t_job           *current_job;       // direct pointer to most-recent job (%+/%%)
    pid_t           shell_pgid;
    int             terminal_fd;
    struct termios  original_termios;

    // History + Line Editor (P1 history, P4 line editor)
    t_history       history;            // embedded (not a pointer)
    t_line_editor   line_editor;        // embedded; wraps readline

    // Aliases (modular, P1)
    t_list          *aliases;           // list of t_alias* (NULL until implemented)
}   t_shell;
```

## File Organization

```
42sh/
├── makefile
├── includes/
│   ├── 42sh.h          # t_shell, LST_* macros, top-level function decls
│   ├── dlist.h         # t_dlist (doubly-linked, used by history)
│   ├── lexer.h         # t_token, lexer_tokenize → t_list*
│   ├── parser.h        # parser_parse, parser_collect_heredocs
│   ├── ast.h           # t_ast, t_cmd (t_list* redirs/assignments), t_redir
│   ├── expander.h      # expand_word, expand_command
│   ├── executor.h      # executor_execute
│   ├── builtins.h      # builtin registry, builtin_get
│   ├── variables.h     # t_var, t_alias (no *next)
│   ├── job_control.h   # t_process, t_job (t_list* processes; no *next)
│   ├── signals.h       # g_signal_received, setup functions
│   ├── history.h       # t_history_entry, t_history (uses t_dlist)
│   └── line_editor.h   # t_line_editor (wraps readline)
├── srcs/
│   ├── main.c
│   ├── dlist/          # ft_dlstnew, ft_dlstadd_back, ft_dlstclear, etc.
│   ├── lexer/
│   ├── parser/
│   ├── expander/
│   ├── executor/
│   ├── builtins/
│   ├── variables/
│   ├── history/        # P1-owned; t_history operations + file I/O
│   ├── job_control/    # P4-owned
│   ├── signals/
│   ├── line_editor/    # P4-owned; wraps readline
│   └── utils/
├── Libft/              # libft (t_list, ft_array, ft_printf, get_next_line)
├── tests/              # Unit tests (make test)
│   ├── minunit.h       # Header-only test framework
│   ├── test_runner.c   # main() for test binary
│   ├── test_dlist.c    # t_dlist tests
│   ├── test_list.c     # t_list (libft) usage tests
│   ├── test_array.c    # ft_array (libft) usage tests
│   └── test_history.c  # history module tests
└── plan/
```

## Error Handling Strategy

1. **Lexer errors**: Invalid tokens → print error, return NULL
2. **Parser errors**: Syntax errors → print error, return NULL
3. **Expansion errors**: Bad substitution → print error, set $? to 1
4. **Execution errors**: Command not found, permission denied → print error, set $?
5. **Memory errors**: malloc fails → clean up and exit gracefully

Never crash. Always free memory. Always reset terminal state.
