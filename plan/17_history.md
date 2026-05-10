# History Module

## Purpose

Store and navigate command history.  History is **owned by P1** and provides a
clean API that the line editor (P4) and the main loop consume.

The module has no dependency on terminal I/O, signals, or the executor.

## Data Structure

Uses **`t_dlist`** (doubly-linked list) because history navigation requires both
backward (Up arrow → older) and forward (Down arrow → newer) traversal.
`t_list` (singly-linked) would not support efficient forward traversal.

```c
// History entry data.  Stored in t_dlist nodes: node->content = t_history_entry*
typedef struct s_history_entry
{
    int     number;     // 1-based monotonically increasing
    char    *line;      // heap-allocated command string
}   t_history_entry;

typedef struct s_history
{
    t_dlist *head;          // oldest entry
    t_dlist *tail;          // newest entry
    t_dlist *current;       // navigation cursor (NULL = not navigating)
    int     count;
    int     max_size;       // default: HISTORY_MAX_SIZE (500)
    int     next_number;
    char    *file_path;     // ~/.42sh_history (set by caller)
}   t_history;
```

`ft_dlstnew(entry_ptr)` stores the pointer directly (`node->content = entry_ptr`).
Access: `(t_history_entry *)node->content` - single cast, no double-deref needed
because `ft_dlstnew` does NOT copy content (unlike `ft_lstnew`).

## Interface

```c
// Lifecycle
void    history_init(t_history *hist, int max_size);
void    history_free(t_history *hist);

// Add after each accepted non-empty line
int     history_add(t_history *hist, const char *line);

// Navigation (called by line editor on up/down arrow)
char    *history_prev(t_history *hist);     // returns older line or NULL
char    *history_next(t_history *hist);     // returns newer line or NULL (= restore saved_line)

// Reset cursor before each new readline session
void    history_reset_cursor(t_history *hist);

// File I/O (0600 permissions, one command per line)
int     history_load(t_history *hist, const char *path);
int     history_save(t_history *hist, const char *path);
```

## Operations

### history_add

```
history_add(hist, line):
    if line is empty or all whitespace: return 0
    if hist->tail && tail->line == line: return 0  // skip duplicates

    entry = malloc(t_history_entry)
    entry->line = strdup(line)
    entry->number = hist->next_number++

    node = ft_dlstnew(entry)
    ft_dlstadd_back(&hist->head, node)
    hist->tail = node   // update tail pointer
    hist->count++

    while hist->count > hist->max_size:
        old_node = hist->head
        hist->head = hist->head->next
        if hist->head: hist->head->prev = NULL
        free (t_history_entry *)old_node->content ->line
        free (t_history_entry *)old_node->content
        free old_node
        hist->count--

    return 1
```

### history_prev (Up arrow)

```
history_prev(hist):
    if hist->count == 0: return NULL

    if hist->current == NULL:
        hist->current = hist->tail      // start from newest
    else if hist->current->prev != NULL:
        hist->current = hist->current->prev
    else:
        return NULL                     // already at oldest

    return ((t_history_entry *)hist->current->content)->line
```

### history_next (Down arrow)

```
history_next(hist):
    if hist->current == NULL: return NULL   // not navigating

    if hist->current->next != NULL:
        hist->current = hist->current->next
        return ((t_history_entry *)hist->current->content)->line
    else:
        hist->current = NULL
        return NULL                         // caller restores saved_line
```

### history_load / history_save

Same semantics as before.  File format: plain text, one command per line,
newest last.  File permissions: 0600.

## Integration with Line Editor (P4)

| Event | Line editor calls |
|-------|-------------------|
| Session starts | `history_reset_cursor(hist)` |
| Up arrow | `history_prev(hist)` |
| Down arrow | `history_next(hist)` |
| Enter | `history_add(hist, line)` + `add_history(line)` (readline) |
| Shell startup | `history_load(hist, path)` |
| Shell exit | `history_save(hist, path)` |

The line editor saves `le->saved_line` before starting navigation so it can
restore it when `history_next()` returns NULL.

## Modular: History Expansions (P1)

Pre-processing step on the raw input string **before tokenization**:

```
history_expand(hist, input):
    scan for !! / !n / !-n / !string
    if match found: print expanded form (so user sees what was substituted)
    return expanded string (malloc'd)
```

Called in main loop after `line_editor_readline()` returns, before
`lexer_tokenize()`.

## Files

```
includes/history.h              # types + function declarations
includes/dlist.h                # t_dlist (used by history)

src/history/
├── history.c                   # history_init, history_free, history_add
├── history_nav.c               # history_prev, history_next, history_reset_cursor
├── history_file.c              # history_load, history_save
└── history_expand.c            # Modular: !! !n expansions

src/dlist/
└── dlist.c                     # ft_dlstnew, ft_dlstadd_back, ft_dlstclear, etc.
```
