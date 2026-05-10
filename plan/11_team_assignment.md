# Team Assignment

## Overview

Division of work for a **4-person team**. The key is clear module boundaries with well-defined interfaces so each person can develop and test independently.

## Workload Estimation

Rough effort per module (implementation + testing + debugging):

| Module | Effort | Notes |
|--------|--------|-------|
| Lexer | ~18h | State machine, quote-aware scanning |
| Parser | ~23h | Recursive descent, grammar rules |
| AST/data structures | ~8h | Node creation/deletion |
| Heredoc collection | ~7h | Walk AST, read content |
| Executor dispatch | ~13h | AST walker, node-type dispatch |
| Pipeline execution | ~25h | Pipes, PGID, no-double-fork (hardest in executor) |
| Redirections | ~18h | dup2, all types, fd numbers |
| PATH search | ~5h | Split PATH, check access |
| Expander core | ~30h | Quote-aware char-by-char expansion (hardest algorithmic module) |
| Field splitting | ~10h | IFS rules |
| Variable module | ~13h | Linked list, get/set/export/environ |
| Environment array | ~5h | Lazy rebuild |
| Line Editor | ~40h | Termcap, raw mode, keys, buffer, display (hardest standalone module) |
| Job Control | ~20h | Process groups, launch/wait/continue |
| Signals | ~10h | Three contexts, sigaction |
| History | ~13h | Data structure, file I/O, navigation API |
| 10 builtins | ~30h | ~3h average each |
| **Total** | **~288h** | **~72h per person** |

## Team Structure

| Person | Primary Responsibility | Modules | Est. |
|--------|----------------------|---------|------|
| **P1** | Parsing + History | Lexer, Parser, AST, Heredoc Collection, History, echo | ~72h |
| **P2** | Execution + Signals | Executor, Pipes, Redirections, Signals (exec+child ctx), exit, type | ~72h |
| **P3** | Variables + Expansion | Variables, Expander, Field Splitting, export, unset, set, cd | ~70h |
| **P4** | Terminal + Jobs | Line Editor, Job Control, Signals (interactive ctx), jobs, fg, bg | ~72h |

### Why This Split Works

1. **P1 gets History** - P1 finishes core parsing early (Phase 1-3). History is clean data-structure work (doubly linked list + file I/O) with a simple API. P1 can develop and test it independently. P4 integrates it into the line editor later via `history_add()`, `history_prev()`, `history_next()`.

2. **P1 gets echo** - echo is the simplest builtin. Building it helps P1 understand the builtin interface early, which helps them test their parser output against real execution.

3. **P2 gets signal setup for executing + child contexts** - P2 already handles fork/exec. Setting up signals in the child before execve (restore defaults) and in the parent during foreground wait (ignore SIGINT etc.) is naturally part of executor code.

4. **P4 keeps interactive signal context** - Interactive signals (Ctrl-C at prompt clears line) are tightly coupled with the line editor. P4 owns both.

5. **P4 keeps Job Control** - Job control is coupled with the line editor through terminal ownership (tcsetpgrp) and signals (SIGTSTP/SIGCHLD). P4 needs to own both for clean terminal management. The pipeline PGID setup code is in the executor (P2), but P4 provides the higher-level job API that P2 calls.

### Shared Infrastructure

| Component | Owner | Reason |
|-----------|-------|--------|
| `main.c` | **P2** | Main loop drives execution; P2 owns executor which is the core of the loop. Others contribute init/cleanup calls. |
| `builtins.c` (registry) | **P2** | Builtin lookup table and `builtin_get()`/`builtin_is_builtin()`. P2 owns the executor that calls these. Each person adds their own builtin entries. |

**Phase 0 note:** `main.c` skeleton, Makefile, and shared headers are set up collaboratively by all 4 members. After Phase 0, P2 maintains `main.c`.

### Builtin Ownership

| Builtin | Owner | Reason |
|---------|-------|--------|
| `echo` | **P1** | Simplest builtin, helps P1 test parser early |
| `exit` | **P2** | Controls main loop flow, tied to executor |
| `type` | **P2** | Needs PATH search (shared with executor) |
| `cd` | **P3** | Modifies PWD/OLDPWD variables |
| `export` | **P3** | Core variable operations |
| `unset` | **P3** | Core variable operations |
| `set` | **P3** | Lists variables |
| `jobs` | **P4** | Job control internals |
| `fg` | **P4** | Job control + terminal |
| `bg` | **P4** | Job control |

---

## Detailed Role Descriptions

### P1: Parsing + History

**Modules:** Lexer, Parser, AST, Heredoc Collection, History, echo builtin

**Responsibilities:**
- Define token types and implement tokenizer (quotes preserved in values)
- Define shell grammar and recursive descent parser
- Handle `&` and NEWLINE as list separators in grammar
- Handle `{`/`}` as reserved words (WORD tokens, not operators)
- Assignment detection (in parser, not lexer)
- Heredoc collection pass (walk AST after parsing, read heredoc content)
- History data structure (doubly linked list)
- History file persistence (load at startup, save at exit, `~/.42sh_history`)
- History navigation API (for P4 to use in line editor)
- echo builtin implementation

**Dependencies:**
- None (first in pipeline)

**Delivers to:**
- P2: AST with heredoc content filled in
- P4: History API (`history_add`, `history_prev`, `history_next`, `history_load`, `history_save`)
- P2: echo builtin function pointer

**Why P1 works well:**
P1 finishes the core lexer/parser by Phase 1-3. After that, history and echo fill the remaining time. History is clean data structure work (no fork/exec complexity), and echo is a simple string operation. Both can be developed and unit-tested independently.

**Files:**
```
include/lexer.h, include/parser.h, include/ast.h, include/history.h
src/lexer/*, src/parser/*, src/history/* (see 17_history.md), src/builtins/builtin_echo.c
```

---

### P2: Execution + Signals (exec/child)

**Modules:** Executor, Redirections, Signals (executing + child contexts), exit/type builtins, main.c, builtin registry

**Responsibilities:**
- Maintain `main.c` (main loop, shell init/cleanup, `-c` mode)
- Maintain builtin registry (`builtins.c`: lookup table, `builtin_get`, `builtin_is_builtin`)
- Walk AST and execute commands
- Call expander (P3's code) for each command before execution
- Implement pipe handling with PGID setup (setpgid in both parent+child)
- Implement redirections (>, >>, <, <<, >&, <&) - applied left-to-right
- Handle fork/exec and PATH search
- Handle `&&`, `||` short-circuit logic and `;` sequences
- Handle `&` (background) by calling P4's job_launch_background
- Signal setup for executing context (parent ignores SIGINT while waiting)
- Signal setup for child context (restore defaults before execve)
- exit and type builtins

**Dependencies:**
- P1: AST structure
- P3: Expander functions (expand_command), Variables (var_get_environ for execve)
- P4: Job control API (job_create, job_launch_foreground, job_launch_background)

**Delivers to:**
- Main loop (exit status)
- P4: Forked processes (via job_add_process)
- All: builtin_get/builtin_is_builtin lookup

**Why P2 works well:**
The executor is the most integration-heavy module, so P2 focuses entirely on it. Signal setup for executing/child contexts is naturally part of the fork/exec code. Owning main.c and the builtin registry is natural since the main loop drives the executor and the executor dispatches builtins.

**Files:**
```
src/main.c
include/executor.h, include/builtins.h, include/signals.h (executing + child parts)
src/executor/*, src/builtins/builtins.c (registry)
src/signals/signal_exec.c, src/signals/signal_child.c
src/builtins/builtin_exit.c, src/builtins/builtin_type.c
```

---

### P3: Variables + Expansion

**Modules:** Expander, Variables, Field Splitting, cd/export/unset/set builtins

**Responsibilities:**
- Variable storage: linked list with get/set/unset/export
- Environment array building (lazy rebuild with env_dirty flag)
- Expander as a service: expand_word, expand_word_to_fields, expand_command
- Quote-aware expansion (walk raw string char-by-char)
- Special parameters in braces: `${?}`, `${$}`, `${0}`
- Assignment expansion (split on `=`, expand only the value part)
- Field splitting on $IFS
- Builtins: cd, export, unset, set

**Dependencies:**
- P1: Token/AST structures (to know how quotes are stored in values)

**Delivers to:**
- P2: Expander functions (expand_command), variable lookup, environment array
- All: var_get_value, var_set, etc.

**Why P3 works well:**
Variables and expansion are deeply coupled (expansion needs var_get_value; builtins like export/unset modify the same data). The expander is algorithmically complex but self-contained - it doesn't need fork/exec or terminal knowledge, just string manipulation and variable lookup. P3 can unit-test expansion independently.

**Files:**
```
include/expander.h, include/variables.h
src/expander/*, src/variables/*
src/builtins/builtin_cd.c, builtin_export.c, builtin_unset.c, builtin_set.c
```

---

### P4: Terminal + Job Control

**Modules:** Line Editor, Job Control, Signals (interactive context), jobs/fg/bg builtins

**Responsibilities:**
- Raw terminal mode with termcap (enter/exit safely)
- Key reading (single chars, escape sequences)
- Buffer management (insert, delete, cursor tracking)
- Display refresh (using termcap capabilities)
- Integrate P1's history module (up/down arrows call history_prev/next)
- Process group management and job tracking
- Terminal ownership management (tcsetpgrp for fg/bg jobs)
- Signal setup for interactive context (Ctrl-C clears line, Ctrl-Z ignored by shell)
- SIGCHLD handling (update background job statuses)
- Builtins: jobs, fg, bg

**Dependencies:**
- P1: History API (history_prev, history_next, history_add)
- P2: Executor calls job_launch_foreground/background

**Delivers to:**
- Main loop (input line from line editor)
- P2: Job control API (job_create, job_launch_foreground, etc.)

**Why P4 works well:**
Line editor + job control are coupled through terminal ownership. When a foreground job runs, the terminal belongs to the job; when it finishes or stops, the terminal returns to the shell/line editor. P4 manages this entire lifecycle. History integration is minimal (just calling P1's API from key handlers).

**Files:**
```
include/line_editor.h, include/job_control.h, include/signals.h (interactive part)
src/line_editor/*, src/job_control/*, src/signals/signal_interactive.c
src/builtins/builtin_jobs.c, builtin_fg.c, builtin_bg.c
```

---

## Signal Module Split

Signals span three contexts, owned by two team members:

| Context | When | Owner | What |
|---------|------|-------|------|
| Interactive | Shell at prompt | **P4** | SIGINT → redisplay prompt. SIGTSTP, SIGQUIT → ignore. SIGCHLD → update jobs. |
| Executing | Foreground child running | **P2** | SIGINT → let it reach child. SIGTSTP → let child stop. Parent waits. |
| Child | Inside fork, before exec | **P2** | Restore all signals to SIG_DFL. |

Shared header: `include/signals.h` defines the setup functions. P2 and P4 each implement their own context setup.

```c
// P4 implements:
void    signals_setup_interactive(void);

// P2 implements:
void    signals_setup_executing(void);
void    signals_setup_child(void);
```

---

## Interface Contracts

These are the key function signatures that cross module boundaries.

### Lexer → Parser (P1 internal)

```c
t_token *lexer_tokenize(const char *input);
void    token_list_free(t_token *head);
```

### Parser → Executor (P1 → P2)

```c
t_ast   *parser_parse(t_token *tokens);
void    ast_free(t_ast *node);
int     parser_collect_heredocs(t_ast *ast, t_shell *shell);
```

### Expander → Executor (P3 → P2)

```c
int     expand_command(t_shell *shell, t_cmd *cmd);
char    *expand_word(t_shell *shell, const char *word);
char    **expand_word_to_fields(t_shell *shell, const char *word);
```

### Variables → Everyone (P3 → All)

```c
char    *var_get_value(t_shell *shell, const char *name);
int     var_set(t_shell *shell, const char *name, const char *value);
int     var_unset(t_shell *shell, const char *name);
int     var_export(t_shell *shell, const char *name);
char    **var_get_environ(t_shell *shell);
```

### History → Line Editor (P1 → P4)

```c
int     history_add(t_history *hist, const char *line);
char    *history_prev(t_history *hist);
char    *history_next(t_history *hist);
void    history_reset_cursor(t_history *hist);
int     history_load(t_history *hist, const char *path);
int     history_save(t_history *hist, const char *path);
```

### Line Editor → Main (P4 → Main)

```c
char    *line_editor_readline(t_line_editor *le, const char *prompt);
```

### Builtins → Executor (P1/P3/P4 → P2)

```c
t_builtin_fn builtin_get(const char *name);
int     builtin_is_builtin(const char *name);
```

### Job Control → Executor (P4 → P2)

```c
t_job   *job_create(t_shell *shell, const char *cmd_line);
int     job_launch_foreground(t_shell *shell, t_job *job);
int     job_launch_background(t_shell *shell, t_job *job);
void    job_add_process(t_job *job, pid_t pid);
```

### Signals → Everyone (P2+P4 → All)

```c
void    signals_setup_interactive(void);    // P4
void    signals_setup_executing(void);      // P2
void    signals_setup_child(void);          // P2
```

---

## Git Workflow

```
main
├── feature/lexer-parser      (P1)
├── feature/history           (P1, after parsing done)
├── feature/executor          (P2)
├── feature/signals-exec      (P2)
├── feature/expander-vars     (P3)
├── feature/line-editor       (P4)
├── feature/job-control       (P4)
└── feature/signals-interact  (P4)
```

---

## Milestones

### M1: Basic Pipeline (P1 + P2)
- Lexer tokenizes simple commands
- Parser builds AST for simple commands
- Executor runs simple commands via fork/exec
- Test: `ls`, `ls -la`, `/bin/echo hello`

### M2: Pipes and Redirections (P1 + P2)
- Lexer/Parser handle |, >, >>, <, &, newlines
- Executor implements pipes (with PGID), redirections
- Test: `ls | grep txt`, `echo hello > file`, `sleep 1 & echo done`

### M3: Variables and Expansion (P3)
- Variable storage working
- Basic $VAR and ${?} expansion working
- export/unset/set builtins working
- Test: `VAR=hello`, `echo $VAR`, `export VAR`, `echo ${?}`

### M4: Line Editing + History (P4 + P1)
- Raw mode, key reading, buffer management (P4)
- History module ready (P1) - P4 integrates into line editor
- Test: interactive editing and history navigation works

### M5: Integration (All)
- Executor calls expander before running commands
- Builtins integrated (cd, echo, exit, type)
- Signals working in all three contexts
- Test: `echo $HOME`, `cd /tmp && pwd`, Ctrl-C at prompt

### M6: Job Control (P4 + P2)
- Background jobs, fg/bg/jobs builtins
- Terminal ownership management
- Test: `sleep 10 &`, `jobs`, `fg %1`, Ctrl-C, Ctrl-Z

### M7: Complete Mandatory (All)
- Heredoc, fd duplication, logical operators
- `cmd1 & cmd2` works correctly
- All edge cases handled
- Memory leak and crash testing

### M8+: Modular Features (All)
- Pick and implement 6 features
- See distribution below

---

## Modular Feature Distribution

Each person takes features closest to their domain:

| Feature | Owner | Reason |
|---------|-------|--------|
| Inhibitors (quotes/backslash) | **P3** | Expander owns quote handling |
| Tilde expansion | **P3** | Part of expander |
| Globbing | **P3** | Part of expander |
| History expansions (!!,!n) | **P1** | Owns history module |
| Aliases | **P1** | Pre-tokenization step, P1 understands lexing |
| Subshells/groups or test or hash | **P2** or **P4** | Flexible - executor or standalone |

This gives each person 1-2 modular features on top of their mandatory work.
