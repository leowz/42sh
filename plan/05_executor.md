# Executor Module

## Purpose

Walk the AST and execute commands. The executor is the central module that:

- Dispatches based on AST node type
- Calls the **expander** for each command's words before execution
- Handles fork/exec for external commands
- Constructs pipelines
- Implements logical operators (`&&`, `||`) and sequences (`;`)
- Runs builtins in the current process
- Manages redirections
- Integrates with job control for background processes

## Interface

```c
// Execute an AST, return exit status
int executor_execute(t_shell *shell, t_ast *ast);
```

## Execution Strategy by Node Type

### Main Dispatch

```
executor_execute(shell, ast):
    int status

    if ast is NULL: return 0

    switch ast->type:
        NODE_COMMAND:    status = execute_simple_command(shell, &ast->cmd)
        NODE_PIPE:       status = execute_pipeline(shell, ast)
        NODE_AND:        status = execute_and(shell, ast)
        NODE_OR:         status = execute_or(shell, ast)
        NODE_SEQUENCE:   status = execute_sequence(shell, ast)
        NODE_SUBSHELL:   status = execute_subshell(shell, ast)
        NODE_BLOCK:      status = execute_block(shell, ast)
        NODE_BACKGROUND: status = execute_background(shell, ast)
        default:         status = 1

    shell->last_exit_status = status
    return status
```

### NODE_COMMAND (Simple Command)

This is where expansion happens - right before execution.

```
execute_simple_command(shell, cmd):
    # 1. EXPAND words now (lazy expansion)
    expand_command(shell, cmd)

    # 2. Handle empty command with just assignments
    #    Example: FOO=bar        → set FOO permanently in shell
    #    Example: FOO=bar > file → also open/create file as side effect
    if cmd->argv is empty or cmd->argv[0] is NULL:
        for each assign in cmd->assignments:
            split assign at first '=' → name, value
            var_set(shell, name, value)
        if cmd->redirs is not NULL:
            save fds with dup()
            setup_redirections(cmd->redirs)
            restore_redirections(saved_fds)
        return 0

    # 3. Check if builtin
    #    Assignments are TEMPORARY — scoped to this command only.
    #    Example: FOO=bar echo $FOO → FOO exists only during echo
    builtin_fn = builtin_get(cmd->argv[0])
    if builtin_fn:
        # save old values and apply assignments
        for each assign in cmd->assignments:
            split assign at first '=' → name, value
            old_values[name] = var_get_value(shell, name)  # NULL if unset
            var_set(shell, name, value)

        save fds, setup redirections
        status = builtin_fn(shell, cmd->argc, cmd->argv)
        restore redirections

        # restore old values (or unset if they didn't exist before)
        for each name in old_values:
            if old_values[name] was NULL:
                var_unset(shell, name)
            else:
                var_set(shell, name, old_values[name])
        return status

    # 4. External command - fork
    #    Assignments are applied to the CHILD's environment only.
    #    Example: FOO=bar /usr/bin/env → FOO only in child's envp
    pid = fork()
    if pid == 0:  # child
        setup signals to default
        setup process group (for job control)

        # apply assignments to environment for execve
        for each assign in cmd->assignments:
            split assign at first '=' → name, value
            var_set(shell, name, value)
            var_export(shell, name)

        setup redirections (no need to save/restore, we're in child)
        path = find_command(shell, cmd->argv[0])
        if not found:
            print "42sh: cmd: command not found"
            exit(127)
        execve(path, cmd->argv, var_get_environ(shell))
        print error
        exit(126)

    # parent (shell's variables are unchanged — fork copied them)
    wait for child (or add to job for foreground tracking)
    return exit status from child
```

### NODE_PIPE

**No double-fork.** Each pipeline stage forks once. The child either execve's directly (external) or runs the builtin and exits.

```
execute_pipeline(shell, ast):
    # Flatten pipeline: collect all commands from nested PIPE nodes
    commands[] = collect left-to-right from PIPE tree
    n = number of commands

    # Create n-1 pipes
    pipes[n-1][2]
    for i in 0..n-2:
        pipe(pipes[i])

    # Pipeline PGID: all processes in a pipeline share ONE process group.
    # The first child's PID becomes the PGID.
    # IMPORTANT: setpgid must be called in BOTH parent AND child to avoid
    # the race where the parent tries to set the group before the child
    # exists, or the child tries to join before the parent has recorded it.
    pgid = 0  # will be set to first child's PID

    # Fork each command
    pids[n]
    for i in 0..n-1:
        pids[i] = fork()
        if pids[i] == 0:  # child
            # Process group setup (child side)
            if pgid == 0: pgid = getpid()     # first child creates the group
            setpgid(getpid(), pgid)           # join the pipeline's group
            setup signals to default

            # Wire pipe fds
            if i > 0:        dup2(pipes[i-1][0], STDIN)
            if i < n-1:      dup2(pipes[i][1], STDOUT)

            # Close ALL pipe fds in child
            for j in 0..n-2:
                close(pipes[j][0])
                close(pipes[j][1])

            # Execute this command directly (NO second fork)
            cmd_ast = commands[i]
            if cmd_ast is NODE_COMMAND:
                expand_command(shell, &cmd_ast->cmd)
                setup command's own redirections
                builtin = builtin_get(cmd_ast->cmd.argv[0])
                if builtin:
                    status = builtin(shell, argc, argv)
                    exit(status)
                else:
                    path = find_command(shell, cmd_ast->cmd.argv[0])
                    if not found: exit(127)
                    execve(path, argv, var_get_environ(shell))
                    exit(126)
            else:
                # Subshell or compound command in pipeline
                status = executor_execute(shell, cmd_ast)
                exit(status)

        # Parent side: also call setpgid (race avoidance)
        if pgid == 0: pgid = pids[i]         # first child's PID = PGID
        setpgid(pids[i], pgid)               # may fail if child already exec'd — that's ok

    # Parent: close all pipe fds
    for i in 0..n-2:
        close(pipes[i][0])
        close(pipes[i][1])

    # Give terminal to pipeline's process group (for foreground jobs)
    if interactive and foreground:
        tcsetpgrp(terminal_fd, pgid)

    # Wait for all children
    for i in 0..n-1:
        waitpid(pids[i], &status, 0)

    # Take terminal back
    if interactive and foreground:
        tcsetpgrp(terminal_fd, shell_pgid)

    # Return exit status of LAST command (bash behavior)
    return exit_status_of(pids[n-1])
```

### NODE_AND and NODE_OR

```
execute_and(shell, ast):
    left_status = executor_execute(shell, ast->left)
    if left_status == 0:
        return executor_execute(shell, ast->right)
    return left_status

execute_or(shell, ast):
    left_status = executor_execute(shell, ast->left)
    if left_status != 0:
        return executor_execute(shell, ast->right)
    return left_status
```

### NODE_SEQUENCE

```
execute_sequence(shell, ast):
    executor_execute(shell, ast->left)
    return executor_execute(shell, ast->right)
```

### NODE_SUBSHELL

```
execute_subshell(shell, ast):
    pid = fork()
    if pid == 0:  # child
        setup signals to default
        setup group redirections (ast->group.redirs)
        status = executor_execute(shell, ast->group.child)
        exit(status)

    # parent
    waitpid(pid, &status, 0)
    return exit_status_of(status)
```

### NODE_BLOCK

Blocks `{ cmd; }` run in the current shell (no fork), but with their own redirections:

```
execute_block(shell, ast):
    save fds
    setup group redirections (ast->group.redirs)
    status = executor_execute(shell, ast->group.child)
    restore fds
    return status
```

### NODE_BACKGROUND

```
execute_background(shell, ast):
    pid = fork()
    if pid == 0:  # child
        setup own process group: setpgid(0, 0)
        redirect stdin from /dev/null
        setup group redirections
        status = executor_execute(shell, ast->group.child)
        exit(status)

    # parent
    add job to shell's job list
    print "[job_id] pid"
    return 0  # background returns immediately
```

## Redirection Implementation

```
setup_redirections(redirs, saved_fds):
    if saved_fds is not NULL:
        save current stdin/stdout/stderr with dup()

    for each redir:
        fd = redir->fd (or default: < uses 0, > uses 1)

        TOK_REDIR_IN (<):
            open target as read-only
            dup2 to fd

        TOK_REDIR_OUT (>):
            open target as write-only, create, truncate, mode 0644
            dup2 to fd

        TOK_REDIR_APPEND (>>):
            open target as write-only, create, append, mode 0644
            dup2 to fd

        TOK_HEREDOC (<<):
            create pipe
            write redir->heredoc_content to pipe write end
            close pipe write end
            dup2 pipe read end to fd

        TOK_REDIR_DUP_OUT (>&):
            if target is "-": close(fd)
            else: dup2(atoi(target), fd)

        TOK_REDIR_DUP_IN (<&):
            if target is "-": close(fd)
            else: dup2(atoi(target), fd)

        on any error: print "42sh: target: error message", return -1

restore_redirections(saved_fds):
    dup2 saved fds back to 0, 1, 2
    close saved fds
```

## Command Search (PATH)

```
find_command(shell, name):
    if name contains '/':
        if accessible and executable: return name
        return NULL

    path_var = var_get_value(shell, "PATH")
    if not set: return NULL

    for each directory in PATH (split on ':'):
        full = directory + "/" + name
        if accessible and executable: return full

    return NULL
```

## Exit Status

```
get_exit_status(wstatus):
    if WIFEXITED: return WEXITSTATUS (0-255)
    if WIFSIGNALED: return 128 + WTERMSIG
    return 1
```

Standard exit codes:
- `0`: success
- `1`: general error
- `2`: syntax/usage error
- `126`: command found but not executable
- `127`: command not found
- `128+N`: killed by signal N

## Files

```
src/executor/
├── executor.c            # Main dispatch (executor_execute)
├── exec_command.c        # Simple command execution
├── exec_pipeline.c       # Pipeline construction
├── exec_logical.c        # && and || execution
├── exec_subshell.c       # Subshell, block, and background
├── redirections.c        # Redirection setup/restore
├── heredoc.c             # Heredoc pipe setup
├── command_search.c      # PATH search
└── exec_utils.c          # Exit status helpers
```
