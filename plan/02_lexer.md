# Lexer Module

## Purpose

Convert a raw input string into a linked list of tokens. The lexer handles:

- Operator recognition (`|`, `&&`, `||`, `;`, `&`, redirections)
- Word tokenization (preserving quotes in token values)
- Quote-aware scanning (so operators inside quotes are not tokenized)
- IO number detection (fd prefix before redirections)

**NOT the lexer's job:** Assignment detection. That is handled by the parser.

## Interface

```c
// Main function - tokenize input, return linked list ending with TOK_EOF
t_token *lexer_tokenize(const char *input);

// Check for unclosed quotes (for multi-line input)
int lexer_check_quotes(const char *input, char *unclosed_quote);

// Free token list
void lexer_free_tokens(t_token *tokens);
```

## State Machine

The lexer uses a state machine to track whether it's inside quotes:

```
┌─────────────────────────────────────────────────────────────────┐
│                         STATE MACHINE                           │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│   ┌─────────┐    whitespace    ┌─────────┐                      │
│   │  START  │◄─────────────────│  START  │                      │
│   └────┬────┘                  └─────────┘                      │
│        │                                                        │
│        ├── ' ──────────────────► SINGLE_QUOTE                   │
│        ├── " ──────────────────► DOUBLE_QUOTE                   │
│        ├── \ ──────────────────► ESCAPE                         │
│        ├── operator ───────────► emit operator token            │
│        └── other ──────────────► IN_WORD                        │
│                                                                 │
│   IN_WORD:                                                      │
│        ├── whitespace ─────────► emit word, → START             │
│        ├── operator ───────────► emit word, → handle operator   │
│        ├── ' ──────────────────► SINGLE_QUOTE (in word)         │
│        ├── " ──────────────────► DOUBLE_QUOTE (in word)         │
│        ├── \ ──────────────────► ESCAPE (in word)               │
│        └── other ──────────────► append to word                 │
│                                                                 │
│   SINGLE_QUOTE:                                                 │
│        ├── ' ──────────────────► return to previous state       │
│        └── other ──────────────► append literally               │
│                                                                 │
│   DOUBLE_QUOTE:                                                 │
│        ├── " ──────────────────► return to previous state       │
│        ├── \ ──────────────────► ESCAPE_IN_DQUOTE               │
│        ├── $ ──────────────────► append (kept for expander)     │
│        └── other ──────────────► append to word                 │
│                                                                 │
│   ESCAPE:                                                       │
│        └── any ────────────────► append literally, → previous   │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

**Key point:** Quotes are kept in the token value. The state machine tracks quote state so it knows when operators/whitespace are inside quotes and should NOT cause token breaks. But the actual quote characters are preserved in the output so the expander can interpret them.

## Operator Recognition

Operators must be recognized longest-match-first:

```
Two-character operators:
    "&&"  → TOK_AND
    "||"  → TOK_OR
    ">>"  → TOK_REDIR_APPEND
    "<<"  → TOK_HEREDOC
    ">&"  → TOK_REDIR_DUP_OUT
    "<&"  → TOK_REDIR_DUP_IN

Single-character operators:
    "|"   → TOK_PIPE
    ";"   → TOK_SEMICOLON
    "&"   → TOK_AMPERSAND
    ">"   → TOK_REDIR_OUT
    "<"   → TOK_REDIR_IN
    "("   → TOK_LPAREN    (operator - breaks words, even inside a word)
    ")"   → TOK_RPAREN    (operator - breaks words)
    "\n"  → TOK_NEWLINE

NOT operators (reserved words - handled by parser):
    "{"   → tokenized as TOK_WORD with value "{"
    "}"   → tokenized as TOK_WORD with value "}"
    These are only special when they appear as a standalone word
    at the start of a command. "echo {a,b}" → WORD("echo") WORD("{a,b}").
```

## IO Number Detection

When a digit immediately precedes `>` or `<` (no space), it's a file descriptor number:

```bash
2>&1    # io_number=2, type=TOK_REDIR_DUP_OUT, target="1"
2>file  # io_number=2, type=TOK_REDIR_OUT, target="file"
>file   # io_number=-1 (default), type=TOK_REDIR_OUT
```

Pseudo code:
```
before emitting a redirect operator:
    if the previous characters were digits with no space before the operator:
        extract those digits as io_number
        remove them from the current word (if building one)
    else:
        io_number = -1
```

## Tokenization Pseudo Code

```
lexer_tokenize(input):
    tokens = empty list
    pos = 0

    while input[pos] != '\0':
        skip whitespace

        if at end of input:
            break

        if is_operator_start(input[pos]):
            check for io_number (digits right before operator)
            tok = read_operator(input, &pos)
            if io_number found:
                tok.io_number = that number
            append tok to tokens

        else:
            tok = read_word(input, &pos)
            append tok to tokens

    append TOK_EOF token
    return tokens
```

```
read_word(input, pos):
    word = empty string buffer
    in_squote = false
    in_dquote = false

    while input[pos] is not end/whitespace/unquoted-operator:
        if input[pos] == '\'' and not in_dquote:
            append '\'' to word          ← preserve the quote
            toggle in_squote
            advance pos
        else if input[pos] == '"' and not in_squote:
            append '"' to word           ← preserve the quote
            toggle in_dquote
            advance pos
        else if input[pos] == '\\' and not in_squote:
            append '\\' to word          ← preserve the backslash
            advance pos
            if not at end:
                append input[pos] to word
                advance pos
        else:
            append input[pos] to word
            advance pos

    return new TOK_WORD with value = word
```

## Edge Cases

1. **Empty input**: Return just `TOK_EOF`
2. **Only whitespace**: Return just `TOK_EOF`
3. **Unclosed quotes**: Return error or signal for continuation (multi-line)
4. **Backslash at end of line**: Signal for continuation
5. **Empty quotes**: `""` and `''` produce a word token containing just the quotes
6. **Adjacent quoted sections**: `"hello"world` → single token `"hello"world`
7. **Operators in quotes**: `"|"` → TOK_WORD with value `"|"`, not TOK_PIPE
8. **Escaped operators**: `\|` → TOK_WORD with value `\|`

## Testing

```bash
# Test cases (showing token types, values include quotes)
echo hello world          → WORD("echo") WORD("hello") WORD("world")
ls -la | grep foo         → WORD("ls") WORD("-la") PIPE WORD("grep") WORD("foo")
echo "hello world"        → WORD("echo") WORD('"hello world"')
VAR=value                 → WORD("VAR=value")
echo $VAR                 → WORD("echo") WORD("$VAR")
ls > file 2>&1            → WORD("ls") REDIR_OUT WORD("file") REDIR_DUP_OUT(io=2) WORD("1")
cmd1 && cmd2 || cmd3      → WORD("cmd1") AND WORD("cmd2") OR WORD("cmd3")
echo "hello"'world'       → WORD("echo") WORD("\"hello\"'world'")
```

## Files

```
src/lexer/
├── lexer.c           # Main tokenize function
├── lexer_state.c     # State machine helpers
├── lexer_operators.c # Operator recognition
├── lexer_words.c     # Word reading (with quotes)
├── lexer_utils.c     # Helper functions
└── token.c           # Token creation/deletion
```
