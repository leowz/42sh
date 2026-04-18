/**
 * @file test_job_control.c
 * @brief Unit tests for the job_control module.
 *
 * Covers:
 *   - job_create / job_add_process / job_find_by_id / job_find_by_pid
 *   - id assignment across create + cleanup cycles
 *   - job_update_statuses + job_notify with a real short-lived child
 *   - ast_to_string for common AST shapes (protects `jobs` display output)
 *
 * No tcsetpgrp / foreground behavior is tested here — that belongs to a
 * later increment.
 */

#ifdef TEST_JOB_CONTROL_ENABLED

#include "minunit.h"
#include "../includes/42sh.h"
#include "../includes/ast.h"
#include "../includes/job_control.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

extern void	stub_shell_init(t_shell *shell);
extern void	stub_shell_cleanup(t_shell *shell);

/* ================================================================
 * 1. job_create / process registration / lookup
 * ================================================================ */

static void	test_job_create_basic(void)
{
	t_shell	shell;
	t_job	*j;

	stub_shell_init(&shell);
	j = job_create(&shell, "sleep 1");
	MU_ASSERT("job_create returns non-null", j != NULL);
	MU_ASSERT_INT(1, j->id);
	MU_ASSERT_STR("job cmd_line copied", "sleep 1", j->cmd_line);
	MU_ASSERT("shell.jobs populated", shell.jobs != NULL);
	MU_ASSERT("current_job set", shell.current_job == j);
	MU_ASSERT_INT(JOB_RUNNING, j->status);
	MU_ASSERT_INT(0, j->notified);
	job_control_cleanup(&shell);
	MU_ASSERT("jobs list freed", shell.jobs == NULL);
	MU_ASSERT("current_job cleared", shell.current_job == NULL);
	stub_shell_cleanup(&shell);
}

static void	test_job_create_incremental_ids(void)
{
	t_shell	shell;
	t_job	*a;
	t_job	*b;
	t_job	*c;

	stub_shell_init(&shell);
	a = job_create(&shell, "a");
	b = job_create(&shell, "b");
	c = job_create(&shell, "c");
	MU_ASSERT_INT(1, a->id);
	MU_ASSERT_INT(2, b->id);
	MU_ASSERT_INT(3, c->id);
	MU_ASSERT("current_job = last", shell.current_job == c);
	job_control_cleanup(&shell);
	stub_shell_cleanup(&shell);
}

static void	test_job_add_process(void)
{
	t_shell		shell;
	t_job		*j;
	t_process	*p;

	stub_shell_init(&shell);
	j = job_create(&shell, "cmd");
	job_add_process(j, 4242, "cmd");
	MU_ASSERT("processes list set", j->processes != NULL);
	p = LST_PROC(j->processes);
	MU_ASSERT_INT(4242, (int)p->pid);
	MU_ASSERT_STR("proc cmd copied", "cmd", p->cmd);
	MU_ASSERT_INT(0, p->completed);
	MU_ASSERT_INT(0, p->stopped);
	job_control_cleanup(&shell);
	stub_shell_cleanup(&shell);
}

static void	test_job_find_by_id_and_pid(void)
{
	t_shell	shell;
	t_job	*j1;
	t_job	*j2;

	stub_shell_init(&shell);
	j1 = job_create(&shell, "one");
	j1->pgid = 100;
	job_add_process(j1, 100, "one");
	j2 = job_create(&shell, "two");
	j2->pgid = 200;
	job_add_process(j2, 200, "two");
	job_add_process(j2, 201, "two-b");

	MU_ASSERT("find_by_id 1", job_find_by_id(&shell, 1) == j1);
	MU_ASSERT("find_by_id 2", job_find_by_id(&shell, 2) == j2);
	MU_ASSERT("find_by_id missing", job_find_by_id(&shell, 99) == NULL);
	MU_ASSERT("find_by_pid j1", job_find_by_pid(&shell, 100) == j1);
	MU_ASSERT("find_by_pid j2 first", job_find_by_pid(&shell, 200) == j2);
	MU_ASSERT("find_by_pid j2 second", job_find_by_pid(&shell, 201) == j2);
	MU_ASSERT("find_by_pid missing", job_find_by_pid(&shell, 999) == NULL);

	job_control_cleanup(&shell);
	stub_shell_cleanup(&shell);
}

/* ================================================================
 * 2. update_statuses + notify with a real child
 * ================================================================ */

static pid_t	fork_quick_child(void)
{
	pid_t	pid;

	fflush(stdout);
	fflush(stderr);
	pid = fork();
	if (pid == 0)
	{
		setpgid(0, 0);
		_exit(0);
	}
	setpgid(pid, pid);
	/* Give the child a beat to exit so WNOHANG picks it up. */
	usleep(50000);
	return (pid);
}

static void	test_update_and_notify_completes_job(void)
{
	t_shell	shell;
	t_job	*job;
	pid_t	pid;
	int		fd;

	stub_shell_init(&shell);
	pid = fork_quick_child();
	job = job_create(&shell, "quick");
	job->pgid = pid;
	job_add_process(job, pid, "quick");

	job_update_statuses(&shell);
	MU_ASSERT_INT(JOB_DONE, job->status);
	MU_ASSERT_INT(1, LST_PROC(job->processes)->completed);

	/* redirect stderr to /dev/null so the notification line doesn't pollute output */
	fflush(stderr);
	fd = dup(STDERR_FILENO);
	int devnull = open("/dev/null", O_WRONLY);
	dup2(devnull, STDERR_FILENO);
	close(devnull);

	job_notify(&shell);

	fflush(stderr);
	dup2(fd, STDERR_FILENO);
	close(fd);

	MU_ASSERT("notify drops completed job", shell.jobs == NULL);
	MU_ASSERT("current_job cleared", shell.current_job == NULL);
	stub_shell_cleanup(&shell);
}

/* ================================================================
 * 3. ast_to_string — protects `jobs` display output
 * ================================================================ */

static t_cmd	*mk_cmd(const char *const argv[])
{
	t_cmd	*c;
	int		n;
	int		i;

	n = 0;
	while (argv[n])
		n++;
	c = (t_cmd *)calloc(1, sizeof(t_cmd));
	c->argv = (char **)calloc((size_t)n + 1, sizeof(char *));
	i = 0;
	while (i < n)
	{
		c->argv[i] = ft_strdup(argv[i]);
		i++;
	}
	c->argc = n;
	return (c);
}

static void	test_ast_to_string_simple_cmd(void)
{
	const char	*argv[] = {"sleep", "10", NULL};
	t_ast		*node;
	char		*s;

	node = ast_new_command(mk_cmd(argv));
	s = ast_to_string(node);
	MU_ASSERT_STR("simple command", "sleep 10", s);
	free(s);
	ast_free(node);
}

static void	test_ast_to_string_pipeline(void)
{
	const char	*la[] = {"ls", NULL};
	const char	*lb[] = {"grep", "foo", NULL};
	t_ast		*left;
	t_ast		*right;
	t_ast		*pipe_node;
	char		*s;

	left = ast_new_command(mk_cmd(la));
	right = ast_new_command(mk_cmd(lb));
	pipe_node = ast_new_binary(NODE_PIPE, left, right);
	s = ast_to_string(pipe_node);
	MU_ASSERT_STR("pipe", "ls | grep foo", s);
	free(s);
	ast_free(pipe_node);
}

static void	test_ast_to_string_logical_chain(void)
{
	const char	*la[] = {"a", NULL};
	const char	*lb[] = {"b", NULL};
	t_ast		*node;
	char		*s;

	node = ast_new_binary(NODE_AND,
			ast_new_command(mk_cmd(la)),
			ast_new_command(mk_cmd(lb)));
	s = ast_to_string(node);
	MU_ASSERT_STR("and", "a && b", s);
	free(s);
	ast_free(node);
}

/* ================================================================
 * Master suite
 * ================================================================ */

void	test_job_control_suite(void)
{
	test_job_create_basic();
	test_job_create_incremental_ids();
	test_job_add_process();
	test_job_find_by_id_and_pid();
	test_update_and_notify_completes_job();
	test_ast_to_string_simple_cmd();
	test_ast_to_string_pipeline();
	test_ast_to_string_logical_chain();
}

#else

void	test_job_control_suite(void)
{
}

#endif
