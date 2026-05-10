# Job Control Module

## Purpose

Manage background jobs and process groups.  Job control allows:
- Running commands in background (`&`)
- Suspending foreground jobs (Ctrl-Z / SIGTSTP)
- Resuming jobs in foreground (`fg`) or background (`bg`)
- Listing jobs (`jobs`)

## Concepts

### Process Groups
Each pipeline runs in its own process group (pgid).  The shell is its own group.

### Controlling Terminal
Only the foreground process group receives SIGINT/SIGTSTP from the keyboard.
`tcsetpgrp(fd, pgid)` moves terminal ownership.

```
SESSION
├── Shell process group (pgid = shell_pgid)   [Foreground when at prompt]
├── Job 1 pgid                                 [Foreground or Background]
└── Job 2 pgid                                 [Stopped]
```

## Data Structures

```c
// Process within a pipeline - NO *next; stored in t_job.processes (t_list*).
typedef struct s_process
{
    pid_t   pid;
    char    *cmd;
    int     status;
    int     completed;
    int     stopped;
}   t_process;

// Job - NO *next; stored in t_shell.jobs (t_list*).
typedef struct s_job
{
    int             id;
    pid_t           pgid;
    char            *cmd_line;
    t_list          *processes;     // list of t_process*
    t_job_status    status;
    int             notified;
    int             foreground;
}   t_job;
```

Access patterns:
- Iterate jobs:     `for (node = shell->jobs; node; node = node->next)` → `LST_JOB(node)`
- Iterate procs:    `for (node = job->processes; node; node = node->next)` → `LST_PROC(node)`

## Interface

```c
int     job_control_init(t_shell *shell);
void    job_control_cleanup(t_shell *shell);

t_job   *job_create(t_shell *shell, const char *cmd_line);
void    job_add_process(t_job *job, pid_t pid, const char *cmd);

int     job_launch_foreground(t_shell *shell, t_job *job);
int     job_launch_background(t_shell *shell, t_job *job);
int     job_continue_foreground(t_shell *shell, t_job *job);
int     job_continue_background(t_shell *shell, t_job *job);
int     job_wait(t_shell *shell, t_job *job);

void    job_update_statuses(t_shell *shell);
void    job_notify(t_shell *shell);

t_job   *job_find_by_id(t_shell *shell, int id);
t_job   *job_find_by_spec(t_shell *shell, const char *spec);
```

## Initialization

```
job_control_init(shell):
    terminal_fd = STDIN_FILENO
    if not isatty(terminal_fd): return 0   // no job control in non-interactive

    // Wait until we own the foreground
    while tcgetpgrp(terminal_fd) != getpgrp():
        kill(-getpgrp(), SIGTTIN)

    // Shell takes its own process group
    shell_pgid = getpid()
    setpgid(shell_pgid, shell_pgid)
    tcsetpgrp(terminal_fd, shell_pgid)
    tcgetattr(terminal_fd, &shell->original_termios)

    // Ignore job-control signals in the shell itself
    signal SIGINT, SIGQUIT, SIGTSTP, SIGTTIN, SIGTTOU → SIG_IGN
```

## Launching Jobs

### Foreground

```
job_launch_foreground(shell, job):
    job->foreground = 1
    tcsetpgrp(shell->terminal_fd, job->pgid)
    status = job_wait(shell, job)
    tcsetpgrp(shell->terminal_fd, shell->shell_pgid)
    tcsetattr(shell->terminal_fd, TCSADRAIN, &shell->original_termios)
    return status
```

### Background

```
job_launch_background(shell, job):
    job->foreground = 0
    ft_dprintf(STDERR_FILENO, "[%d] %d\n", job->id, job->pgid)
    return 0
```

## Waiting

```
job_wait(shell, job):
    loop:
        pid = waitpid(-job->pgid, &status, WUNTRACED)
        if pid < 0:
            if errno == ECHILD: break
            return -1

        find t_process in job->processes by pid (iterate t_list)
        update process->status, completed, stopped

        if WIFSTOPPED: job->status = JOB_STOPPED; return 128 + WSTOPSIG
        if all processes completed: job->status = JOB_DONE; break

    return exit status of last process
```

## Background Job Monitoring (before each prompt)

```
job_update_statuses(shell):
    loop: waitpid(-1, &status, WNOHANG | WUNTRACED)
        find job/process for pid in shell->jobs (iterate t_list)
        update process + job status

job_notify(shell):
    for each t_job in shell->jobs (iterate t_list):
        if needs notification: print status line
        if JOB_DONE/JOB_TERMINATED and notified:
            remove from shell->jobs list; free job and its processes
```

## Child Setup

```c
child_setup_job_control(shell, job, foreground):
    pid = getpid()
    if job->pgid == 0: job->pgid = pid
    setpgid(pid, job->pgid)
    if foreground && shell->interactive:
        tcsetpgrp(shell->terminal_fd, job->pgid)
    // Restore signals to SIG_DFL (shell ignores them)
    signals_setup_child()
```

## Job Spec Parsing

```
job_find_by_spec(shell, spec):
    NULL / ""   → shell->current_job
    "%+" / "%%" → shell->current_job
    "%-"        → job before current_job in list
    "%N"        → job with id == N
    "%string"   → first job whose cmd_line starts with string
```

## Files

```
src/job_control/
├── job_control.c       # init, cleanup
├── job.c               # job_create, job_add_process, job_find_*
├── job_launch.c        # launch_foreground, launch_background
├── job_wait.c          # job_wait, child_setup_job_control
├── job_continue.c      # continue_foreground, continue_background
├── job_notify.c        # job_update_statuses, job_notify
└── job_utils.c         # job_find_by_spec, helper for list traversal
```
