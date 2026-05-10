# Background Knowledge & Resources

## Must-Read

### 1. "Advanced Programming in the UNIX Environment" (APUE) - Stevens & Rago

THE reference for everything this project needs: `fork`, `exec`, `pipe`, `dup2`, `signal`/`sigaction`, process groups, sessions, terminal control, `tcsetpgrp`.

Most relevant chapters:
- Ch 8: Process Control (fork/exec/wait)
- Ch 9: Process Relationships (sessions, process groups, job control)
- Ch 10: Signals
- Ch 15: IPC (pipes)
- Ch 18: Terminal I/O (termios, raw mode)

Use as a reference when implementing each module. No need to read cover-to-cover.

### 2. "The Linux Programming Interface" (TLPI) - Michael Kerrisk

Same topics as APUE but more modern and Linux-specific (our target platform). Longer but clearer explanations. Same chapters are relevant. Pick whichever of APUE or TLPI you prefer - you don't need both.

## Very Helpful

### 3. Bash Reference Manual (GNU)

Free online. This is the behavior reference - when unsure how something should work, test it in bash and read this manual.

- https://www.gnu.org/software/bash/manual/bash.html
- Sections on quoting, expansion, redirections, job control are directly applicable

### 4. "Writing a Unix Shell" tutorial series - Nelson Elhage

Short blog series that walks through building a shell step by step. Covers the exact concepts needed: fork/exec, pipes, signals, job control. Good for getting the big picture before diving into APUE/TLPI details.

### 5. POSIX Shell Command Language spec

The actual standard. Dry reading, but it's the definitive answer when bash behavior is ambiguous:

- https://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html

## For Specific Modules

| Module | Best Resource |
|--------|--------------|
| **Lexer/Parser/AST** | "Crafting Interpreters" ch4-6 for state machines, recursive descent, and tree nodes. POSIX §2.10 for shell grammar. |
| **Executor/Pipes** | GLIBC "Implementing a Shell" for fork/exec/wait patterns. APUE ch8-9 for process groups and pipeline PGID. |
| **Expander/Variables** | Bash manual §3.5 (Shell Expansions) and POSIX §2.6 (Word Expansions). Quote-aware char-by-char walking is the core skill. |
| **Termcap/Line Editor** | `man termios` for raw mode. `man termcap` for cursor control. GNU Readline source for architecture ideas. |
| **Job Control** | GLIBC "Implementing a Shell" - covers process groups, fg/bg, tcsetpgrp with real C code. |
| **Signals** | APUE ch10 or TLPI ch20-22. Three contexts (interactive/executing/child) need different setups. Read before coding. |

## Per Team Member

| Person | Priority Reading |
|--------|-----------------|
| **P1** (Parser + History) | "Crafting Interpreters" ch4-6, POSIX shell grammar §2.10, Bash `parse.y` source. Learn state machines, recursive descent, union-based AST nodes. |
| **P2** (Executor + Signals) | GLIBC "Implementing a Shell", APUE ch8-9 (fork/exec/waitpid, process groups). Learn pipeline PGID setup, dup2 wiring, signal restore in children. |
| **P3** (Expander + Variables) | Bash manual §3.5, POSIX §2.6. Learn quote-aware character walking, field splitting on IFS, and the expansion order (tilde → parameter → field split → quote removal). |
| **P4** (Line Editor + Jobs) | APUE ch18 (termios, raw mode), GLIBC "Implementing a Shell" (job control). Learn tcgetattr/tcsetattr, termcap capabilities, tcsetpgrp for terminal ownership. |

---

## P1 Deep Dive: Lexer, Parser, and AST

P1 builds the front-end of the shell - turning raw text into a tree structure that the executor can walk. This requires understanding three closely related concepts.

### What to Learn

**Lexer (tokenizer):**
- How to split a stream of characters into meaningful tokens
- State machines: tracking whether you're inside quotes, a word, or whitespace
- Longest-match rule: `>>` must be recognized before two `>` tokens

**Parser (syntax analysis):**
- Recursive descent parsing: one function per grammar rule
- Operator precedence: how `a ; b && c | d` groups correctly
- Left-to-right associativity: `a | b | c` = `(a | b) | c`
- Error recovery: printing a useful message and stopping cleanly

**AST (abstract syntax tree):**
- Tree representation of the command structure
- Union-based nodes in C (one struct, different data per node type)
- Tree walking (the executor traverses what P1 builds)

### Recommended Resources (in order)

**1. "Crafting Interpreters" by Robert Nystrom - Chapters 4-6 (free online)**
- https://craftinginterpreters.com
- Ch 4 "Scanning" = lexer. Teaches state-machine tokenization with clear code.
- Ch 5 "Representing Code" = AST. Shows how to define tree nodes with a union/visitor pattern.
- Ch 6 "Parsing Expressions" = recursive descent parser with operator precedence.
- Written in Java but concepts translate directly to C. The best intro to this topic.

**2. "Compilers: Principles, Techniques, and Tools" (Dragon Book) - Ch 2-3**
- Ch 2 "A Simple Syntax-Directed Translator" walks through lexing → parsing → tree building
- Ch 3 "Lexical Analysis" covers state machines and token recognition in depth
- Heavy academic book - use as reference, not cover-to-cover reading

**3. POSIX Shell Grammar**
- https://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html#tag_18_10
- Section 2.10 defines the actual shell grammar in BNF notation
- Our `03_parser.md` is a simplified version of this. Refer to the original when unclear.

**4. Bash source code - `parse.y`**
- Bash uses a yacc/bison grammar file. Reading it helps understand what tokens exist and how grammar rules combine.
- You won't copy it (we use recursive descent, not yacc), but it shows how a real shell handles edge cases.
- https://git.savannah.gnu.org/cgit/bash.git/tree/parse.y

**5. "Let's Build a Simple Interpreter" - Ruslan Spivak (blog series)**
- https://ruslanspivak.com/lsbasi-part1/
- Step-by-step blog building a recursive descent parser from scratch
- Very hands-on, good for someone who learns by doing

### Key Concepts for P1

**State machine for the lexer:**
```
The lexer tracks a state as it reads each character:

  DEFAULT → sees ' → IN_SINGLE_QUOTE (everything literal until next ')
  DEFAULT → sees " → IN_DOUBLE_QUOTE ($ still special, until next ")
  DEFAULT → sees | → emit TOK_PIPE, check for || next
  DEFAULT → sees letter → IN_WORD (keep reading until whitespace/operator)
  IN_WORD → sees ' → IN_SINGLE_QUOTE (quote is PART of the word)
  IN_WORD → sees whitespace → emit TOK_WORD, back to DEFAULT
```

**Recursive descent pattern:**
```
Each grammar rule becomes a function. The function:
  1. Calls the higher-precedence function to get the left operand
  2. Checks if the current token is the operator it handles
  3. If yes: consume the operator, call higher-precedence again for right operand
  4. Build a tree node, repeat

Example for && / ||:
  parse_and_or():
      left = parse_pipeline()          # higher precedence
      while current is && or ||:
          op = current
          advance
          right = parse_pipeline()
          left = new_node(op, left, right)
      return left
```

**AST node design in C (union-based):**
```c
// One struct with a type tag and a union for the data:
typedef struct s_ast {
    t_node_type type;       // NODE_COMMAND, NODE_PIPE, NODE_AND, ...
    union {
        t_cmd    cmd;       // for NODE_COMMAND
        t_binary binary;    // for NODE_PIPE, NODE_AND, NODE_OR, NODE_SEQUENCE
        t_group  group;     // for NODE_SUBSHELL, NODE_BLOCK, NODE_BACKGROUND
    } data;
} t_ast;

// The executor switches on type and accesses the right union member:
switch (ast->type) {
    case NODE_PIPE: use ast->data.binary.left / right
    case NODE_COMMAND: use ast->data.cmd.argv
}
```

**Common P1 mistakes to avoid:**
- Don't strip quotes in the lexer - preserve them for the expander
- Don't detect assignments in the lexer - only the parser knows if a word is in prefix position
- Don't use a separate token type for `{`/`}` - they're reserved words (WORD tokens)
- Don't forget that `&` is a list separator like `;`, not just a trailing flag

---

## P2 Deep Dive: Executor, Pipelines, and Signals

P2 is the engine of the shell - taking the AST that P1 built and actually running commands. This means understanding how UNIX creates processes, wires them together, and manages their lifecycle.

### What to Learn

**Process creation (fork/exec/wait):**
- `fork()` duplicates the current process; child gets PID 0 return, parent gets child's PID
- `execve()` replaces the child's memory with a new program - it never returns on success
- `waitpid()` blocks until a child changes state (exits, stops, continues)
- The fork+exec pattern: fork first, set up the child's environment, then exec

**Pipelines and file descriptor plumbing:**
- `pipe()` creates two connected fds: write to one end, read from the other
- `dup2(old, new)` makes fd `new` point to the same file/pipe as fd `old`
- Each pipeline stage: redirect stdin from previous pipe, stdout to next pipe
- Close ALL pipe fds in every process after dup2 - leaked fds cause hangs

**Process groups (PGID):**
- All processes in a pipeline share one process group
- First child's PID becomes the PGID for the whole pipeline
- `setpgid()` must be called in BOTH parent AND child (race condition otherwise)
- `tcsetpgrp()` gives terminal control to a process group (foreground)

**Signals in executor context:**
- Parent ignores SIGINT/SIGTSTP while a foreground child runs
- Child restores all signals to SIG_DFL before calling execve
- SIGCHLD tells the parent a child changed state

### Recommended Resources (in order)

**1. GLIBC "Implementing a Shell" (free online, ~30 min)**
- https://www.gnu.org/software/libc/manual/html_node/Implementing-a-Shell.html
- Walks through job launch, foreground/background, process groups, and waiting - with real C code
- THE single best resource for P2. Read this first, read it twice.

**2. APUE Ch 8 "Process Control" or TLPI Ch 24-26**
- Covers fork/exec/wait in depth with all the edge cases
- Explains exit status encoding (WIFEXITED, WIFSIGNALED, 128+N convention)
- Read after the GLIBC tutorial to fill in the details

**3. APUE Ch 9 "Process Relationships" or TLPI Ch 34**
- Process groups, sessions, controlling terminal
- Explains why `setpgid` needs to happen in both parent and child
- Critical for understanding pipeline PGID setup

**4. APUE Ch 10 "Signals" or TLPI Ch 20-22**
- `sigaction()` vs `signal()` (always use sigaction)
- Signal masks, async-signal-safe functions
- Read the sections on SIGINT, SIGTSTP, SIGCHLD specifically

**5. "Writing a Unix Shell" - Nelson Elhage (blog series)**
- Step-by-step tutorial building a shell with pipes and job control
- Shorter and more practical than APUE - good first pass before the textbooks

### Key Concepts for P2

**The fork/exec lifecycle:**
```
Parent shell
    │
    fork() ──────────► Child process (copy of parent)
    │                       │
    │                       ├── setpgid (join pipeline group)
    │                       ├── dup2 (wire pipes/redirections)
    │                       ├── close (unused pipe fds)
    │                       ├── signals_setup_child (SIG_DFL)
    │                       └── execve (replace with new program)
    │
    setpgid (race avoidance)
    waitpid (block until child done)
    collect exit status
```

**Pipeline fd wiring (3 commands: A | B | C):**
```
    pipe0        pipe1
  [r0, w0]     [r1, w1]

  A: stdout → w0          (dup2(w0, 1))
  B: stdin  ← r0, stdout → w1   (dup2(r0, 0), dup2(w1, 1))
  C: stdin  ← r1          (dup2(r1, 0))

  Every process closes ALL of: r0, w0, r1, w1 after dup2.
  Parent also closes all pipe fds after forking all children.
```

**Exit status decoding:**
```
waitpid(pid, &wstatus, 0):
  WIFEXITED(wstatus)   → normal exit   → WEXITSTATUS gives 0-255
  WIFSIGNALED(wstatus) → killed by signal → 128 + WTERMSIG
  WIFSTOPPED(wstatus)  → stopped (Ctrl-Z) → WSTOPSIG gives signal number
```

**Common P2 mistakes to avoid:**
- Don't forget to close pipe fds - a leaked write end keeps the reader blocking forever
- Don't call `setpgid` only in the child - parent must also call it (child might exec before parent runs)
- Don't ignore `execve` return - if it returns, it failed (print error, `exit(126)` or `exit(127)`)
- Don't double-fork in pipelines - each child either calls execve or runs a builtin and exits directly

---

## P3 Deep Dive: Expander and Variables

P3 handles the most algorithmically complex module - turning raw strings with `$VAR`, quotes, and special syntax into the final expanded words. It's pure string manipulation, no fork/exec knowledge needed.

### What to Learn

**Variable storage:**
- Linked list of name/value pairs, each with an `exported` flag
- `var_get_value(name)` returns the value (or NULL if unset)
- `var_set(name, value)` creates or updates a variable
- `var_export(name)` marks a variable for inclusion in child environments
- `var_get_environ()` builds the `char **envp` array for execve (lazy rebuild with dirty flag)

**Expansion order (POSIX §2.6):**
- Tilde expansion (`~` → home directory)
- Parameter expansion (`$VAR`, `${VAR}`, `${VAR:-default}`)
- Command substitution (`$(cmd)`) - modular feature
- Arithmetic expansion (`$((expr))`) - modular feature
- Field splitting (split unquoted results on `$IFS`)
- Pathname expansion (globbing) - modular feature
- Quote removal (final step - strip the quote characters)

**Quote-aware character walking:**
- The expander reads the raw string character by character
- Single quotes: everything literal, no expansion, until closing `'`
- Double quotes: `$` expansion happens, but no field splitting or globbing on the result
- Unquoted: full expansion + field splitting + globbing
- The key insight: you track a "quoted context" as you walk, and it determines what operations apply

**Field splitting:**
- After parameter expansion, unquoted results are split on characters in `$IFS`
- Default IFS is space, tab, newline
- IFS whitespace is treated specially (leading/trailing ignored, runs collapsed)
- Quoted expansions are NOT split - `"$VAR"` stays as one field even if VAR contains spaces

### Recommended Resources (in order)

**1. Bash Reference Manual §3.5 "Shell Expansions" (free online)**
- https://www.gnu.org/software/bash/manual/bash.html#Shell-Expansions
- Clear explanation of each expansion type with examples
- This is your behavior reference - test in bash, then read this to confirm

**2. POSIX §2.6 "Word Expansions"**
- https://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html#tag_18_06
- The formal specification. Drier than the bash manual but definitive.
- Sections 2.6.1 (tilde), 2.6.2 (parameter), 2.6.5 (field splitting), 2.6.7 (quote removal)

**3. Bash Reference Manual §3.1.2 "Quoting"**
- https://www.gnu.org/software/bash/manual/bash.html#Quoting
- How single quotes, double quotes, and backslash work
- Critical for understanding what the expander must do vs. what the lexer preserved

**4. POSIX §2.2 "Quoting"**
- https://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html#tag_18_02
- Formal rules for which characters are special in which contexts
- Use when bash manual is ambiguous

**5. Bash source code - `subst.c`**
- https://git.savannah.gnu.org/cgit/bash.git/tree/subst.c
- This is where bash does all expansion. It's ~6000 lines and complex, but searching for specific function names (like `parameter_brace_expand`) shows how edge cases are handled.
- Don't try to read it all - use it as a reference for specific questions

### Key Concepts for P3

**Character-by-character expansion loop:**
```
expand_word(shell, raw_string):
    result = empty buffer
    i = 0
    while raw_string[i]:
        if raw_string[i] == '\'':
            # Single quote: copy everything literally until closing '
            i++  (skip opening quote)
            while raw_string[i] != '\'':
                append raw_string[i] to result
                i++
            i++  (skip closing quote)

        else if raw_string[i] == '"':
            # Double quote: expand $ but mark result as "quoted"
            i++  (skip opening quote)
            while raw_string[i] != '"':
                if raw_string[i] == '$':
                    expand variable, append result (marked quoted)
                else:
                    append raw_string[i] to result
                i++
            i++  (skip closing quote)

        else if raw_string[i] == '$':
            # Unquoted $: expand variable
            expanded = expand_variable(shell, raw_string, &i)
            append expanded to result (marked unquoted - eligible for splitting)

        else:
            append raw_string[i] to result
            i++
    return result
```

**Field splitting concept:**
```
Input:  VAR="hello   world"

"$VAR"  → 1 field: "hello   world"     (quoted - no split)
$VAR    → 2 fields: "hello", "world"   (unquoted - split on IFS)

The expander must track which parts of the result came from
quoted vs unquoted expansions, so field splitting knows what to split.
```

**Environment array lazy rebuild:**
```
var_get_environ(shell):
    if not env_dirty:
        return cached_environ

    count exported variables
    allocate char** array
    for each variable with exported flag:
        array[i] = "NAME=VALUE"
    env_dirty = false
    return array

Any var_set/var_unset/var_export call sets env_dirty = true.
```

**Common P3 mistakes to avoid:**
- Don't expand inside single quotes - `'$VAR'` is literal, period
- Don't field-split inside double quotes - `"$VAR"` is always one field
- Don't forget quote removal is the LAST step - after all other expansions
- Don't expand assignments the same as words - split on first `=`, only expand the value part
- Don't rebuild the environ array on every execve - use the dirty flag pattern

---

## P4 Deep Dive: Line Editor, Job Control, and Terminal

P4 owns everything related to the terminal - reading input character by character, displaying edits, and managing which process group controls the terminal. These two modules (line editor + job control) are coupled through terminal ownership.

### What to Learn

**Terminal raw mode (termios):**
- By default, terminals are in "canonical mode": input is line-buffered, the kernel handles backspace/Ctrl-C
- For a line editor, you need "raw mode": every keypress is delivered immediately to your program
- `tcgetattr()` saves current settings, you modify the `termios` struct, `tcsetattr()` applies it
- Always restore original settings on exit (or the user's terminal is broken)

**Termcap capabilities:**
- Termcap is a database of terminal capabilities (cursor movement, clear screen, etc.)
- `tgetent()` loads the terminal's capabilities
- `tgetstr()` retrieves a specific capability string (e.g., "le" = cursor left)
- `tputs()` outputs a capability string (handles padding/timing)
- Key capabilities: cursor left/right, delete char, clear to end of line, cursor position

**Escape sequence reading:**
- Arrow keys, Home, End, Delete send multi-byte escape sequences (e.g., `\x1b[A` = up arrow)
- You read one byte at a time; if it's `\x1b`, use a short timeout to check if more bytes follow
- If more bytes come: it's an escape sequence (parse it)
- If no more bytes: it's the actual Escape key

**Job control (process groups + terminal):**
- Each job (pipeline) gets its own process group
- Only one process group can be "foreground" (receives terminal input)
- `tcsetpgrp(fd, pgid)` transfers terminal ownership to a process group
- When a foreground job finishes or stops (Ctrl-Z), the shell takes the terminal back
- Background jobs run without terminal access - they get SIGTTIN/SIGTTOU if they try to read/write the terminal

**SIGCHLD handling:**
- When any child exits or stops, the parent receives SIGCHLD
- The handler should call `waitpid(-1, &status, WNOHANG | WUNTRACED)` in a loop
- Update the job's status based on what happened (exited, stopped, signaled)
- Report completed/stopped background jobs at the next prompt

### Recommended Resources (in order)

**1. GLIBC "Implementing a Shell" (free online, ~30 min)**
- https://www.gnu.org/software/libc/manual/html_node/Implementing-a-Shell.html
- Covers job launch, foreground/background, process groups, terminal ownership
- Has complete C code for job control - closest thing to a tutorial for this

**2. APUE Ch 18 "Terminal I/O" or TLPI Ch 62**
- Deep dive into termios: canonical vs raw mode, all the flags, tcgetattr/tcsetattr
- Explains VMIN/VTIME for controlling read behavior in raw mode
- Essential for the line editor - read before coding

**3. `man termcap` and `man termios`**
- The actual API reference you'll use daily while coding
- termcap: tgetent, tgetstr, tgetnum, tputs
- termios: struct termios, tcgetattr, tcsetattr, all the c_lflag/c_iflag/c_oflag bits

**4. APUE Ch 9 "Process Relationships" or TLPI Ch 34**
- Sessions, process groups, controlling terminal
- Explains the foreground/background process group model
- Why `tcsetpgrp` is needed and when to call it

**5. GNU Readline source code**
- https://git.savannah.gnu.org/cgit/readline.git
- You can't use readline (using termcap instead), but reading its architecture helps
- Look at `readline.c` (main loop), `input.c` (key reading), `display.c` (screen update)
- Don't copy the code - understand the patterns (read key → update buffer → redisplay)

**6. "Build Your Own Text Editor" - antirez (Kilo editor tutorial)**
- https://viewsourcecode.org/snaptoken/kilo/
- Step-by-step tutorial for building a terminal text editor in C using raw mode
- Chapters 2-4 cover exactly what you need: raw mode, keypress reading, screen refresh
- Not a shell tutorial, but the terminal handling patterns are identical to a line editor

### Key Concepts for P4

**Entering and exiting raw mode:**
```
enter_raw_mode():
    tcgetattr(STDIN, &original_termios)     # save for later
    raw = original_termios
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN)
        # ECHO: don't echo input        (we do it ourselves)
        # ICANON: disable line buffering (read byte-by-byte)
        # ISIG: disable Ctrl-C/Ctrl-Z   (we handle them ourselves)
        # IEXTEN: disable Ctrl-V/Ctrl-O
    raw.c_iflag &= ~(IXON | ICRNL)
        # IXON: disable Ctrl-S/Ctrl-Q flow control
        # ICRNL: don't translate CR to NL (we want raw input)
    raw.c_oflag |= OPOST
        # OPOST: KEEP output processing  (\n still maps to \r\n)
    raw.c_cc[VMIN] = 1      # read returns after 1 byte
    raw.c_cc[VTIME] = 0     # no timeout
    tcsetattr(STDIN, TCSAFLUSH, &raw)

exit_raw_mode():
    tcsetattr(STDIN, TCSAFLUSH, &original_termios)
```

**Key reading with escape sequences:**
```
read_key():
    read 1 byte → c
    if c != '\x1b':
        return c   (normal character)

    # Might be escape sequence - try to read more
    read with short timeout → seq[0]
    if timeout (nothing came):
        return ESCAPE_KEY

    read with short timeout → seq[1]
    if seq[0] == '[':
        if seq[1] == 'A': return ARROW_UP
        if seq[1] == 'B': return ARROW_DOWN
        if seq[1] == 'C': return ARROW_RIGHT
        if seq[1] == 'D': return ARROW_LEFT
        if seq[1] == 'H': return HOME
        if seq[1] == 'F': return END
        if seq[1] == '3':
            read → seq[2]
            if seq[2] == '~': return DELETE
    return ESCAPE_KEY
```

**Line editor buffer management:**
```
The buffer tracks:
    - char *buf: the current line content
    - int len: number of characters in the buffer
    - int cursor: cursor position (0 to len)
    - int capacity: allocated size

Insert character at cursor:
    shift buf[cursor..len] right by 1
    buf[cursor] = c
    len++, cursor++

Delete character at cursor (backspace):
    shift buf[cursor..len] left by 1
    len--, cursor--

The display function redraws the line using termcap:
    move cursor to start of line
    clear to end of line
    write prompt + buf
    move cursor to correct position
```

**Job control terminal ownership lifecycle:**
```
Shell at prompt:
    terminal belongs to shell (shell's PGID is foreground)

User types "ls | grep foo":
    fork pipeline → all children in new PGID
    tcsetpgrp(terminal_fd, pipeline_pgid)     # give terminal to pipeline
    waitpid for all children
    tcsetpgrp(terminal_fd, shell_pgid)        # take terminal back
    back to prompt

User types "sleep 10 &":
    fork → child in own PGID
    do NOT tcsetpgrp (it's background)
    shell keeps terminal, back to prompt immediately

User types Ctrl-Z (foreground job stops):
    SIGTSTP sent to foreground PGID
    child stops → waitpid returns with WIFSTOPPED
    tcsetpgrp(terminal_fd, shell_pgid)        # take terminal back
    mark job as "stopped"
    back to prompt

User types "fg %1":
    tcsetpgrp(terminal_fd, job_pgid)          # give terminal to job
    kill(job_pgid, SIGCONT)                   # resume the job
    waitpid (block until it finishes or stops again)
    tcsetpgrp(terminal_fd, shell_pgid)        # take terminal back
```

**Common P4 mistakes to avoid:**
- Don't forget to restore terminal settings on exit - register `exit_raw_mode` with `atexit()`
- Don't disable OPOST - you need `\n` → `\r\n` translation for output
- Don't use `read()` without a timeout for escape sequences - you'll block waiting for bytes that won't come
- Don't call `tcsetpgrp` from a background process - it causes SIGTTOU. Only the foreground process can transfer terminal ownership.
- Don't forget to take the terminal back after a foreground job finishes - or your shell can't read input

---

## Start Here: GLIBC "Implementing a Shell"

**Every team member should read this first.** It's short (~30 min), free, and walks through job control with actual C code. It covers process groups, foreground/background, signals, and `tcsetpgrp` - exactly the hardest parts of the project.

- https://www.gnu.org/software/libc/manual/html_node/Implementing-a-Shell.html

## Key Concepts to Understand Before Coding

### Process Model
- `fork()` creates a child process (copy of parent)
- `execve()` replaces the current process image with a new program
- `waitpid()` waits for a child to change state
- `pipe()` creates a unidirectional data channel between two file descriptors
- `dup2(old, new)` redirects one fd to another

### Process Groups & Sessions
- Every process belongs to a process group (PGID)
- Every process group belongs to a session (SID)
- The terminal has one foreground process group at a time
- `setpgid()` moves a process into a group
- `tcsetpgrp()` gives a group the terminal (foreground)

### Signals
- Signals are asynchronous notifications to a process
- `sigaction()` sets up signal handlers (always use this, never `signal()`)
- SIGINT (Ctrl-C), SIGTSTP (Ctrl-Z), SIGQUIT (Ctrl-\) are sent to the foreground process group
- SIGCHLD is sent to the parent when a child exits or stops
- Signal handlers should only call async-signal-safe functions

### Terminal I/O
- `termios` struct controls terminal behavior
- Canonical mode: input is line-buffered (default)
- Raw mode: input is character-by-character (needed for line editor)
- OPOST: keep enabled so `\n` maps to `\r\n` automatically
- `tcgetattr()` / `tcsetattr()` to save/restore terminal settings

### File Descriptors & Redirections
- 0 = stdin, 1 = stdout, 2 = stderr
- `open()` returns a new fd
- `dup2(src, dst)` makes dst point to the same file as src
- Redirections are applied left-to-right, order matters
- `2>&1 >/dev/null` is different from `>/dev/null 2>&1`
