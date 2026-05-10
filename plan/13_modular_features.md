# Modular Features Guide

## Overview

You must implement **6 features** from this list to pass. Choose based on:
- Team expertise
- Features that build on each other
- Stability (don't add broken features)

## Important: Mandatory vs Modular Quote Handling

**Basic quote handling is MANDATORY** - the lexer must preserve quotes in token values, and the expander must respect quote context (single quotes = literal, double quotes = expand `$` but no splitting/globbing). This is part of the core expansion logic.

The **Inhibitors modular feature** goes beyond this basic handling to cover:
- Backslash escaping (`\` before special characters)
- Nested quotes (`"it's"`, `'say "hi"'`)
- Escaped quotes inside double quotes (`"hello \"world\""`)
- Complete POSIX quote semantics

In practice, you should implement inhibitors as one of your 6 features since it makes the shell much more usable.

## Recommended Set (Balanced)

| # | Feature | Difficulty | Value | Notes |
|---|---------|------------|-------|-------|
| 1 | Inhibitors (`"`, `'`, `\`) | Medium | Essential | Makes shell usable |
| 2 | Tilde expansion (`~`) | Easy | High | Simple, useful |
| 3 | Globbing (`*`, `?`, `[]`) | Medium | High | Very useful |
| 4 | Subshells `()` | Medium | Medium | Tests fork understanding |
| 5 | History (file + expansions) | Medium | High | Great UX |
| 6 | Aliases | Easy | Medium | Simple to add |

## Alternative Sets

### Easier Set
1. Inhibitors
2. Tilde expansion
3. Aliases
4. Hash table
5. History (basic)
6. test builtin

### More Advanced Set
1. Inhibitors
2. Globbing
3. Command substitution `$()`
4. Arithmetic `$(())`
5. Process substitution `<()`, `>()`
6. Vi/Readline modes

---

## Feature Details

### 1. Inhibitors (`"`, `'`, `\`)

**What it does:**
- Single quotes: Everything literal, no expansion at all
- Double quotes: Allow `$` expansion, command substitution, arithmetic, but no globbing or field splitting
- Backslash: In unquoted context, escape the next character. In double quotes, only escape `$`, `` ` ``, `"`, `\`, and newline

**What the mandatory base already provides:**
- Lexer preserves quotes in token values
- Expander recognizes `'` and `"` boundaries and adjusts expansion behavior

**What this feature adds:**
- Complete backslash escaping in all contexts
- Proper handling of escaped characters inside double quotes
- Nested quote handling (single inside double and vice versa)
- Backslash-newline continuation (line continuation)

**Examples:**
```bash
echo 'hello $USER'        # hello $USER
echo "hello $USER"        # hello john
echo "hello \"world\""    # hello "world"
echo hello\ world         # hello world (single argument)
echo "it's working"       # it's working
echo 'say "hi"'           # say "hi"
```

**Complexity:** Medium (touches lexer and expander)

---

### 2. Globbing (`*`, `?`, `[]`, `!`)

**What it does:**
- `*` matches any string (including empty)
- `?` matches any single character
- `[abc]` matches any char in set
- `[a-z]` matches range
- `[!abc]` matches any char NOT in set

**Where it happens:**
- In the expander, after field splitting, before quote removal
- Only on unquoted glob characters (quoted `*` is literal)
- If no match found, the pattern is kept literally (POSIX behavior)

**Implementation notes:**
- Track which glob chars were quoted (use internal markers after quote processing)
- Can use POSIX `glob()` function or implement pattern matching manually
- Results are sorted alphabetically
- Hidden files (starting with `.`) are not matched by `*` unless pattern starts with `.`

**Examples:**
```bash
ls *.c              # All .c files
ls file?.txt        # file1.txt, fileA.txt, etc.
ls [abc]*           # Files starting with a, b, or c
echo "*"            # Literal * (quoted)
```

**Complexity:** Medium (self-contained in expander)

---

### 3. Tilde Expansion

**What it does:**
- `~` expands to $HOME
- `~/path` expands to $HOME/path
- `~user` expands to user's home directory (via getpwnam)
- Only at start of word or after `:` in assignments

**Implementation:**

```
expand_tilde(shell, word):
    if word doesn't start with '~': return word unchanged
    if ~ is quoted: return word unchanged

    find end of tilde-prefix (next '/' or end of word)
    name = characters between ~ and /

    if name is empty: return $HOME + rest
    else:
        pw = getpwnam(name)
        if found: return pw->pw_dir + rest
        else: return word unchanged
```

**Examples:**
```bash
echo ~              # /home/user
echo ~/Documents    # /home/user/Documents
echo ~root          # /root
echo "~"            # ~ (quoted, no expansion)
```

**Complexity:** Easy

---

### 4. Parameter Expansion Formats

**What it does:**
```bash
${param:-word}    # Use word if param unset/null
${param:=word}    # Assign word if param unset/null
${param:?word}    # Error with word if param unset/null
${param:+word}    # Use word if param is set
${#param}         # Length of param value
${param%pattern}  # Remove shortest suffix match
${param%%pattern} # Remove longest suffix match
${param#pattern}  # Remove shortest prefix match
${param##pattern} # Remove longest prefix match
```

**Examples:**
```bash
unset FOO
echo ${FOO:-default}     # default
echo ${FOO:=assigned}    # assigned (and sets FOO)
echo ${#HOME}            # length of HOME value
echo ${PATH##*:}         # last component of PATH
```

**Complexity:** Medium-Hard (parsing operators, pattern matching for %/# forms)

---

### 5. Control Groups: `()` and `{}`

**What it does:**
- `( commands )` - Run in subshell (separate environment, changes don't affect parent)
- `{ commands; }` - Run in current shell (grouping only)
- Both can have redirections applied to the whole group

**Implementation:**
- Parser recognizes `(` and `{` as group starters
- AST uses NODE_SUBSHELL (fork) and NODE_BLOCK (no fork)
- Both store child AST and redirections list (t_group struct)
- `{}` requires space after `{` and `;` before `}`

**Examples:**
```bash
(cd /tmp; pwd)           # /tmp (but parent dir unchanged)
{ cd /tmp; pwd; }        # /tmp (parent dir IS changed)
(echo hello) > file      # Redirect subshell output
```

**Complexity:** Medium

---

### 6. Command Substitution `$()`

**What it does:**
- Execute command and substitute its stdout output
- Trailing newlines are removed
- Can be nested: `$(echo $(whoami))`

**Implementation:**

```
expand_command_subst(shell, input, pos):
    find matching ')' (tracking depth for nested $())
    extract command string

    create pipe
    fork:
        child: redirect stdout to pipe write, parse and execute command, exit
        parent: read all output from pipe read, wait for child

    strip trailing newlines
    return output
```

**Examples:**
```bash
echo "Today is $(date)"
files=$(ls *.c)
echo "Count: $(ls | wc -l)"
echo "User: $(echo $(whoami))"
```

**Complexity:** Hard (recursive parsing, pipe management)

---

### 7. Arithmetic Expansion `$(())`

**What it does:**
- Evaluate arithmetic expression, substitute result as string
- Variables are expanded to their values
- Standard operators: `+ - * / %`, comparisons, logical

**Implementation:**
- Recursive descent expression parser
- Variable lookup within expressions (no `$` needed: `$((x + 1))`)
- Integer arithmetic only

**Examples:**
```bash
echo $((1 + 2))          # 3
echo $((5 > 3))          # 1 (true)
x=5; echo $((x * 2))     # 10
```

**Complexity:** Hard (expression parser)

---

### 8. Process Substitution `<()` and `>()`

**What it does:**
- `<(command)` - Command output available as a file path (via /dev/fd/N)
- `>(command)` - Write to a file path that pipes into command

**Implementation:**
- Create pipe
- Fork child for the command
- Substitute with `/dev/fd/N` path (Linux provides this)

**Examples:**
```bash
diff <(ls dir1) <(ls dir2)
tee >(gzip > file.gz) < input
```

**Complexity:** Hard

---

### 9. History Management

**Components:**
- History list in memory (already exists from line editor)
- History file (~/.42sh_history) with load/save
- Expansions: `!!`, `!n`, `!-n`, `!string`
- fc builtin (list, re-execute, edit)
- Ctrl-R reverse search (optional bonus)

**History expansion happens BEFORE tokenization** - it's a pre-processing step on the raw input line.

```
History expansion:
    !!          → repeat last command
    !n          → command number n
    !-n         → n commands ago
    !string     → most recent command starting with string
    !?string    → most recent command containing string
```

**Complexity:** Medium

---

### 10. Contextual Completion

**What it does:**
- Tab completes commands, files, variables
- Context-aware: command position completes commands, argument position completes files

**Implementation:**
- Detect context (first word = command, after `$` = variable, otherwise = file)
- Generate candidates from PATH (commands), directory listing (files), or variable list
- Single match: complete inline. Multiple matches: display list.

**Complexity:** Hard (context detection, display management)

---

### 11. Vi/Readline Editing Modes

**What it does:**
- Two editing modes selectable via `set -o vi` or `set -o emacs`
- Vi: Command/insert modes, movement commands (h, j, k, l, w, b, etc.)
- Readline/Emacs: Ctrl shortcuts (C-b, C-f, C-p, C-n, etc.)

**Complexity:** Hard (many commands to implement for each mode)

---

### 12. Aliases

**What it does:**
- Define shortcut names for commands
- Expanded before tokenization of command words
- Only first word of a simple command is checked for alias expansion

**Implementation:**
- Alias table (linked list or hash map)
- After tokenization, check first word token against alias table
- If match found, replace with alias value and re-tokenize the replacement
- Prevent infinite recursion (don't re-expand the same alias)

**Builtins:**
- `alias` - no args: print all. `alias name=value`: set.
- `unalias name` - remove. `unalias -a` - remove all.

**Examples:**
```bash
alias ll='ls -la'
alias grep='grep --color=auto'
ll              # Executes: ls -la
unalias ll
```

**Complexity:** Easy

---

### 13. Hash Table

**What it does:**
- Cache resolved command paths to avoid repeated PATH searches
- Cleared when PATH changes

**Implementation:**
- Hash map: command name → full path
- On first execution of a command, store the resolved path
- `hash` builtin: list all cached, `hash -r` clear, `hash name` add

**Complexity:** Easy

---

### 14. test Builtin

**What it does:**
- Evaluate conditional expressions
- `test expr` and `[ expr ]` forms (same behavior, `[` requires `]` as last arg)

**Operators:**
- File tests: `-b, -c, -d, -e, -f, -g, -L, -p, -r, -S, -s, -u, -w, -x`
- String tests: `-z` (empty), `=`, `!=`
- Numeric tests: `-eq, -ne, -ge, -gt, -lt, -le`
- Logical: `!` (negation)

**Examples:**
```bash
test -f file.txt && echo exists
[ -d /tmp ] && echo "is directory"
test "$a" = "$b"
[ 5 -gt 3 ] && echo "yes"
```

**Complexity:** Medium (many operators to implement, but each is simple)

---

## Feature Dependencies

```
                    Inhibitors (quotes/backslash)
                        │
            ┌───────────┼───────────┐
            ▼           ▼           ▼
        Globbing    Tilde      Param formats
                                    │
                                    ▼
                    Command Substitution
                        │
                        ▼
                    Arithmetic
```

**Recommendation:** Implement inhibitors first - they affect the behavior of most other features.
