# 42sh Architecture Plan

## Quick Start

Read the documents in order. Each builds on concepts from the previous ones.

## Document Index

| # | Document | Description |
|---|----------|-------------|
| 00 | [Overview](00_overview.md) | High-level architecture, data flow, t_shell structure |
| 01 | [Data Structures](01_data_structures.md) | Token, AST, command, redirect, job structs |
| 02 | [Lexer](02_lexer.md) | Tokenizer: state machine, quote preservation |
| 03 | [Parser](03_parser.md) | Recursive descent parser, AST building, assignment detection |
| 04 | [Expander](04_expander.md) | Expansion service (called by executor per-command) |
| 05 | [Executor](05_executor.md) | AST walker, pipes, redirections, fork/exec |
| 06 | [Builtins](06_builtins.md) | cd, echo, exit, type, export, unset, set, jobs, fg, bg |
| 07 | [Line Editor](07_line_editor.md) | Termcap-based input with editing and history |
| 08 | [Job Control](08_job_control.md) | Process groups, background jobs, fg/bg |
| 09 | [Signals](09_signals.md) | Signal handling in three contexts |
| 10 | [Variables](10_variables.md) | Variable storage, environment management |
| 11 | [Team Assignment](11_team_assignment.md) | 4-person team roles and interface contracts |
| 12 | [Development Phases](12_development_phases.md) | Phase-by-phase development roadmap |
| 13 | [Modular Features](13_modular_features.md) | Optional features guide (need 6) |
| 14 | [Reference Shell](14_reference_shell_and_posix.md) | POSIX standard and bash reference notes |
| 15 | [Review Report](15_review_report.md) | Architecture review findings and fixes applied |
| 16 | [Background Knowledge](16_background_knowledge.md) | Books, tutorials, and key concepts for the team |
| 17 | [History Module](17_history.md) | History data structure, navigation API, file persistence (P1-owned) |

## Architecture Summary

```
Input → Lexer → Parser → Heredoc Collection → Executor → Result
                                                  │
                                              Expander
                                          (called per-command)
```

**Key design decisions:**

1. **Expander is a service, not a pipeline pass.** The executor calls `expand_command()` for each simple command right before running it. This ensures `$?` and other state reflect execution-time values.

2. **Quotes are preserved in token values.** The lexer keeps quote characters in the raw string. The expander interprets them during expansion. Quote removal is the final expansion step.

3. **Heredoc collection happens after parsing, before execution.** The parser records heredoc redirections in the AST. A separate pass walks the AST and reads heredoc content from input. Then execution begins.

4. **No double-fork in pipelines.** Each pipeline child is forked once by the parent shell. The child either calls execve directly or runs a builtin and exits - never forks again.

5. **Assignment detection is in the parser, not the lexer.** This avoids misclassifying `echo VAR=value` as an assignment.

## Platform

Target: **Linux Ubuntu**. Reference shell: **bash**.
