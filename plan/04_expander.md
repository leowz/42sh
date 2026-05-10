# Expander Module

## Purpose

The expander is a **service** called by the executor, NOT a separate pipeline pass. It expands a command's words right before that command runs. This is critical because `$?` and other state-dependent values must reflect execution-time state, not parse-time state.

## Architecture

```
Executor walks AST
    │
    ├── Reaches NODE_COMMAND
    │       │
    │       ▼
    │   expand_command(shell, cmd)    ← expander called HERE
    │       │
    │       ├── Expand each argv word → may produce multiple fields
    │       ├── Expand each assignment value → single string
    │       └── Expand each redirection target → single string
    │
    ├── Reaches NODE_PIPE
    │       └── Recurse into left/right (each child expands its own commands)
    │
    └── Reaches NODE_AND / NODE_OR
            └── Recurse (short-circuit), expansion happens lazily per-command
```

## Interface

```c
// Expand a single word to a single string (for assignments, redir targets)
// Performs: tilde, parameter, command subst, arithmetic, quote removal
// Does NOT perform: field splitting, globbing
char    *expand_word(t_shell *shell, const char *word);

// Expand a single word to multiple fields (for argv words)
// Performs all expansions including field splitting and globbing
char    **expand_word_to_fields(t_shell *shell, const char *word);

// Expand a whole command node (argv, assignments, redirections)
// Called by executor before running each simple command
int     expand_command(t_shell *shell, t_cmd *cmd);
```

## Expansion Order (POSIX)

Expansions happen in this order for each word:

```
1. Tilde expansion         (only at word start or after : in assignments)
2. Parameter expansion     ($VAR, ${VAR}, $?, $$, etc.)
3. Command substitution    $(cmd) - modular feature
4. Arithmetic expansion    $((expr)) - modular feature
5. Field splitting         (on unquoted expansion results, using $IFS)
6. Pathname expansion      (globbing: *, ?, [] - modular feature)
7. Quote removal           (remove unescaped quotes)
```

## Quote Behavior

Quotes in the raw token value control what gets expanded:

| Context | Tilde | Param | Cmd Subst | Arithmetic | Split | Glob |
|---------|-------|-------|-----------|------------|-------|------|
| Unquoted | Yes | Yes | Yes | Yes | Yes | Yes |
| Double `"` | No | Yes | Yes | Yes | No | No |
| Single `'` | No | No | No | No | No | No |

## Core Expansion Algorithm

The expander walks the raw word string character by character, tracking quote state:

```
expand_word_internal(shell, input):
    result = new empty string buffer
    pos = 0
    in_single_quote = false
    in_double_quote = false

    while input[pos] is not end:
        c = input[pos]

        if c == '\'' and not in_double_quote:
            toggle in_single_quote
            pos++                          # skip quote char
            continue

        if c == '"' and not in_single_quote:
            toggle in_double_quote
            pos++                          # skip quote char
            continue

        if in_single_quote:
            append c to result             # everything literal
            pos++
            continue

        if c == '$':
            expanded = expand_dollar(shell, input, &pos, in_double_quote)
            append expanded to result
            continue

        if c == '~' and pos == 0 and not in_double_quote:
            expanded = expand_tilde(shell, input, &pos)
            append expanded to result
            continue

        if c == '\\' and in_double_quote:
            next = input[pos + 1]
            if next is one of $ ` " \ newline:
                append next to result      # escape recognized
                pos += 2
            else:
                append '\\' to result      # literal backslash
                pos++
            continue

        if c == '\\' and not in_single_quote and not in_double_quote:
            pos++                          # skip backslash
            if input[pos] is not end:
                append input[pos] to result
                pos++
            continue

        append c to result
        pos++

    return result as string
```

## Tilde Expansion

Only at the beginning of a word or after `:` in assignment values:

```
Examples:
    ~           → $HOME
    ~/foo       → $HOME/foo
    ~user       → user's home directory (getpwnam)
    "~"         → ~ (quoted, no expansion)
    a~b         → a~b (not at start)

expand_tilde(shell, input, pos):
    skip the '~' character
    read until '/' or end of word (or ':' in assignment context)
    name = the characters read

    if name is empty:
        home = var_get_value(shell, "HOME")
        if home is NULL: return "~" literally
        return home + rest of word

    else:
        pw = getpwnam(name)
        if pw is NULL: return "~name" literally
        return pw->pw_dir + rest of word
```

## Parameter Expansion

```
expand_dollar(shell, input, pos, in_double_quote):
    skip '$'

    if next char is '?':
        skip it, return itoa(shell->last_exit_status)

    if next char is '$':
        skip it, return itoa(getpid())

    if next char is '0':
        skip it, return shell name ("42sh")

    if next char is '{':
        return expand_braced(shell, input, pos)

    if next char is '(' and char after is '(':
        return expand_arithmetic(shell, input, pos)    # modular

    if next char is '(':
        return expand_command_subst(shell, input, pos) # modular

    # Simple $NAME form
    read while char is alphanumeric or '_'
    name = characters read
    if name is empty: return "$" literally
    value = var_get_value(shell, name)
    return value if not NULL, else ""
```

### Braced Parameters: `${VAR}`, `${?}`, `${#param}`, ...

```
expand_braced(shell, input, pos):
    skip '${'
    read until '}' (watching for nested ${})
    content = characters read
    skip '}'

    # Special parameters inside braces: ${?}, ${$}, ${0}
    if content is "?":
        return itoa(shell->last_exit_status)
    if content is "$":
        return itoa(getpid())
    if content is "0":
        return shell name ("42sh")

    # Basic: just a variable name
    if content is a valid identifier:
        return var_get_value(shell, content) or ""

    # Modular: ${param:-word}, ${#param}, etc.
    # Parse the operator and dispatch to format handler
```

**Note:** The subject explicitly requires `${?}`. The braced path must handle special parameters before checking for identifiers, because `?` is not a valid identifier name.

### Modular Parameter Formats

```
${param:-word}    Use word if param unset/null
${param:=word}    Assign word if param unset/null
${param:?word}    Error if param unset/null
${param:+word}    Use word if param is set
${#param}         String length
${param%pattern}  Remove shortest suffix
${param%%pattern} Remove longest suffix
${param#pattern}  Remove shortest prefix
${param##pattern} Remove longest prefix
```

## Command Substitution (Modular)

`$(command)` or `` `command` ``

```
expand_command_subst(shell, input, pos):
    skip '$('
    find matching ')' (track depth for nested $())
    cmd_string = content between parens
    skip ')'

    create pipe
    fork child:
        redirect stdout to pipe write end
        close pipe read end
        parse and execute cmd_string as a complete shell input
        exit with status

    parent:
        close pipe write end
        read all output from pipe read end
        wait for child
        strip trailing newlines from output
        return output
```

## Arithmetic Expansion (Modular)

`$((expression))`

```
expand_arithmetic(shell, input, pos):
    skip '$(('
    find matching '))'
    expression = content
    skip '))'

    expand variables within expression
    evaluate expression (recursive descent: +, -, *, /, %, comparisons, logical)
    return result as string
```

## Field Splitting

After expansion, unquoted results are split on `$IFS`:

```
field_split(shell, expanded_string, was_quoted):
    if was_quoted: return single-element array with the string

    ifs = var_get_value(shell, "IFS")
    if ifs is NULL: ifs = " \t\n"
    if ifs is "": return single-element array (no splitting)

    ifs_whitespace = whitespace chars in ifs (space, tab, newline)
    ifs_non_whitespace = non-whitespace chars in ifs

    Rules:
        - Leading/trailing IFS whitespace is trimmed
        - Sequences of IFS whitespace = single separator
        - Each IFS non-whitespace char is a separate delimiter
        - Adjacent non-whitespace delimiters produce empty fields

    split string according to rules
    return array of fields
```

## Pathname Expansion / Globbing (Modular)

After field splitting, each unquoted field is checked for glob characters:

```
glob_expand(fields):
    result = empty array

    for each field:
        if field contains unquoted *, ?, or [:
            matches = match against filesystem
            if matches found:
                sort matches alphabetically
                append all matches to result
            else:
                append field as-is to result (no match = literal)
        else:
            append field as-is to result

    return result
```

**Note:** Glob characters that were quoted (inside `''` or `""` or preceded by `\`) must NOT trigger globbing. The expander needs to track which characters were quoted - a common approach is to use internal marker characters for "literal" versions of `*`, `?`, `[`.

## Quote Removal

Final step - remove all unescaped quote characters from each field:

```
remove_quotes(string):
    walk string, removing:
        unescaped single quotes
        unescaped double quotes
        backslashes that were used for escaping
    compact the string in place
```

By this point, quote tracking has already controlled expansion behavior, so the quotes themselves can be stripped.

## Expanding a Command Node

Called by the executor for each simple command:

```
expand_command(shell, cmd):
    # Expand argv (may produce multiple fields per word)
    new_argv = empty array
    for each word in cmd->argv:
        fields = expand_word_to_fields(shell, word)
        append all fields to new_argv
    replace cmd->argv with new_argv
    update cmd->argc

    # Expand assignment values (single string, no splitting)
    # cmd->assignments is char** of "NAME=VALUE" strings
    for each assignment_str in cmd->assignments:
        find first '=' in assignment_str
        name = substring before '='
        raw_value = substring after '='
        expanded_value = expand_word(shell, raw_value)
        rebuild assignment_str as "name=expanded_value"

    # Expand redirection targets (single string, no splitting/globbing)
    for each redir in cmd->redirs:
        if redir is not heredoc:
            redir->target = expand_word(shell, redir->target)
        # Heredoc content: expand $VAR inside unless delimiter was quoted

    return 0 on success, -1 on error
```

## Heredoc Expansion

Heredoc content is expanded at execution time (not during heredoc collection):

```
expand_heredoc(shell, content, delimiter_was_quoted):
    if delimiter_was_quoted:
        return content as-is (no expansion)
    else:
        expand only parameter expansion and command substitution
        (no tilde, no field splitting, no globbing)
        return expanded content
```

## Files

```
src/expander/
├── expander.c            # expand_command, expand_word, expand_word_to_fields
├── expand_word.c         # Core char-by-char expansion (expand_word_internal)
├── expand_tilde.c        # Tilde expansion
├── expand_parameter.c    # $VAR, ${VAR}, $?, $$
├── expand_command.c      # Command substitution $() - modular
├── expand_arithmetic.c   # Arithmetic $() - modular
├── expand_glob.c         # Pathname expansion - modular
├── field_split.c         # IFS-based field splitting
├── quote_removal.c       # Final quote stripping
└── expand_utils.c        # Helpers (string buffer, markers, etc.)
```
