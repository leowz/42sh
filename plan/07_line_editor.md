# Line Editor Module (readline)

## Purpose

Provide interactive line editing using the **GNU readline library** (`-lreadline`).
Readline handles everything that would otherwise require a hand-rolled termcap
implementation:

- Raw terminal mode management
- Character insertion/deletion and cursor movement
- History navigation (Up/Down arrows) - integrated with our `t_history`
- Ctrl-A/E (home/end), Ctrl-K/U/W (kill operations), Ctrl-L (clear screen)
- Vi and Emacs editing modes (modular feature)

P4 wraps readline to:
1. Configure it at startup (history, signals, completion hook).
2. Integrate it with our `t_history` module (file persistence + expansions).
3. Handle multi-line continuation (unclosed quotes / trailing `\`).
4. Switch editing modes at runtime for the `set -o vi / -o emacs` modular feature.

## Interface

```c
// Initialize readline and hook in our history.
int     line_editor_init(t_line_editor *le, t_history *history);

// Save history file, free saved_line, rl_clear_history().
void    line_editor_cleanup(t_line_editor *le);

// Read one complete line (handles multi-line continuation internally).
// Returns malloc'd string, or NULL on EOF.
// Calls history_add() + add_history() after each accepted non-empty line.
char    *line_editor_readline(t_line_editor *le, const char *prompt);

// Switch between emacs (LE_MODE_EMACS) and vi (LE_MODE_VI) at runtime.
void    line_editor_set_mode(t_line_editor *le, int mode);
```

## Readline Configuration

```c
line_editor_init(le, history):
    le->history = history
    le->saved_line = NULL
    le->editing_mode = LE_MODE_EMACS  // default

    // Configure readline
    rl_readline_name = "42sh"
    rl_attempted_completion_function = completion_hook  // NULL until completion feature
    using_history()                                      // activate history
    rl_variable_bind("editing-mode", "emacs")            // default emacs mode

    return 0
```

## Reading a Line

```c
line_editor_readline(le, prompt):
    history_reset_cursor(le->history)     // each session starts fresh

    line = readline(prompt)
    if line == NULL:
        return NULL    // EOF (Ctrl-D on empty line)

    // Multi-line continuation: unclosed quotes or trailing backslash
    while lexer_check_quotes(line, &quote) != 0 or line ends with '\':
        continuation = readline("> ")
        if continuation == NULL:
            free(line)
            return NULL
        new_line = ft_strjoin(ft_strjoin(line, "\n"), continuation)
        free(line)
        free(continuation)
        line = new_line

    return line          // caller calls history_add() and add_history()
```

## History Integration

We maintain two history systems in parallel:

| System | Purpose |
|--------|---------|
| readline's internal history (`add_history()`) | Up/Down arrow key navigation |
| our `t_history` (P1's module) | File persistence, `!!`/`!n` expansions, `fc` builtin |

After each accepted non-empty line, the **caller** (main loop) calls:
```c
add_history(line);                       // readline internal
history_add(&shell->history, line);      // our module
```

At startup:  `history_load(&shell->history, "~/.42sh_history")`
At exit:     `history_save(&shell->history, "~/.42sh_history")`

## Editing Modes (Modular Feature)

`set -o emacs` → calls `line_editor_set_mode(le, LE_MODE_EMACS)`:
```c
rl_variable_bind("editing-mode", "emacs")
le->editing_mode = LE_MODE_EMACS
```

`set -o vi` → calls `line_editor_set_mode(le, LE_MODE_VI)`:
```c
rl_variable_bind("editing-mode", "vi")
le->editing_mode = LE_MODE_VI
```

Readline's built-in vi mode provides all the required vi shortcuts (h, j, k,
l, w, b, ^, $, 0, i, I, a, A, x, X, d, D, y, Y, p, P, u, U, r, R, c, C, s, S,
/, ?, n, N, #, v, ~) out of the box.

Readline's Emacs (default) mode provides: C-b, C-f, C-p, C-n, C-_, C-t, A-t.

## Signal Handling During Input

readline's `read()` is interrupted by signals.  The signals_setup_interactive
handler sets `g_signal_received = SIGINT` and writes `\n` to stdout.
readline detects the interrupted read and the next call to `readline()` shows a
fresh prompt with an empty line.

Ctrl-D on a non-empty line: readline returns the current buffer (not NULL).
Ctrl-D on an empty line: readline returns NULL → main loop exits.

## Files

```
src/line_editor/
├── line_editor.c       # init, cleanup, readline wrapper
└── line_editor_mode.c  # set_mode, completion hook (stub for now)
```

## Why Not a Custom Termcap Editor?

The subject explicitly allows `readline` (`-lreadline`).  Using readline:
- Gives us full POSIX-quality line editing on day 1 (freeing P4 to focus on job control).
- Provides Vi/Emacs modes with a single `rl_variable_bind()` call.
- Lets us add contextual completion later via `rl_attempted_completion_function`.
- Keeps the terminal always in a consistent state (readline resets on cleanup).

The raw-mode / termcap knowledge documented in background resources is still
worth understanding - it's exactly what readline implements under the hood - but
there is no reason to reimplement it from scratch.
