/**
 * @file job.c
 * @brief Job bookkeeping: creation, process registration, lookup, teardown.
 * @author pulgamecanica
 */

#include "42sh.h"
#include "job_control.h"
#include <stdlib.h>

static int	next_job_id(t_shell *shell)
{
	t_list	*node;
	int		max_id;
	int		id;

	max_id = 0;
	node = shell->jobs;
	while (node)
	{
		id = LST_JOB(node)->id;
		if (id > max_id)
			max_id = id;
		node = node->next;
	}
	return (max_id + 1);
}

t_job	*job_create(t_shell *shell, const char *cmd_line)
{
	t_job	*job;
	t_list	*node;

	job = (t_job *)calloc(1, sizeof(t_job));
	if (!job)
		return (NULL);
	job->id = next_job_id(shell);
	job->pgid = 0;
	job->cmd_line = cmd_line ? ft_strdup(cmd_line) : NULL;
	job->processes = NULL;
	job->status = JOB_RUNNING;
	job->notified = 0;
	job->foreground = 0;
	node = ft_lstnew(job);
	if (!node)
	{
		free(job->cmd_line);
		free(job);
		return (NULL);
	}
	ft_lstappend(&shell->jobs, node);
	shell->current_job = job;
	return (job);
}

void	job_add_process(t_job *job, pid_t pid, const char *cmd)
{
	t_process	*proc;
	t_list		*node;

	if (!job)
		return ;
	proc = (t_process *)calloc(1, sizeof(t_process));
	if (!proc)
		return ;
	proc->pid = pid;
	proc->cmd = cmd ? ft_strdup(cmd) : NULL;
	proc->status = 0;
	proc->completed = 0;
	proc->stopped = 0;
	node = ft_lstnew(proc);
	if (!node)
	{
		free(proc->cmd);
		free(proc);
		return ;
	}
	ft_lstappend(&job->processes, node);
}

t_job	*job_find_by_id(t_shell *shell, int id)
{
	t_list	*node;

	node = shell->jobs;
	while (node)
	{
		if (LST_JOB(node)->id == id)
			return (LST_JOB(node));
		node = node->next;
	}
	return (NULL);
}

t_job	*job_find_by_pid(t_shell *shell, pid_t pid)
{
	t_list	*jn;
	t_list	*pn;

	jn = shell->jobs;
	while (jn)
	{
		pn = LST_JOB(jn)->processes;
		while (pn)
		{
			if (LST_PROC(pn)->pid == pid)
				return (LST_JOB(jn));
			pn = pn->next;
		}
		jn = jn->next;
	}
	return (NULL);
}

static void	proc_free(void *proc_ptr)
{
	t_process	*p;

	p = (t_process *)proc_ptr;
	if (!p)
		return ;
	free(p->cmd);
	free(p);
}

void	job_free(void *job_ptr)
{
	t_job	*job;

	job = (t_job *)job_ptr;
	if (!job)
		return ;
	ft_lstdel(&job->processes, proc_free);
	free(job->cmd_line);
	free(job);
}
