/**
 * @file job_notify.c
 * @brief Reap background jobs and print status notifications.
 * @author pulgamecanica
 */

#include "42sh.h"
#include "job_control.h"
#include <stdlib.h>
#include <sys/wait.h>

const char	*job_status_str(t_job_status s)
{
	if (s == JOB_RUNNING)
		return ("Running");
	if (s == JOB_STOPPED)
		return ("Stopped");
	if (s == JOB_DONE)
		return ("Done");
	return ("Terminated");
}

static t_process	*process_find(t_job *job, pid_t pid)
{
	t_list	*node;

	node = job->processes;
	while (node)
	{
		if (LST_PROC(node)->pid == pid)
			return (LST_PROC(node));
		node = node->next;
	}
	return (NULL);
}

static void	recompute_job_status(t_job *job)
{
	t_list		*node;
	t_process	*p;
	int			all_done;
	int			any_stopped;

	all_done = 1;
	any_stopped = 0;
	node = job->processes;
	while (node)
	{
		p = LST_PROC(node);
		if (!p->completed)
			all_done = 0;
		if (p->stopped)
			any_stopped = 1;
		node = node->next;
	}
	if (all_done)
	{
		if (job->status != JOB_TERMINATED)
			job->status = JOB_DONE;
	}
	else if (any_stopped)
		job->status = JOB_STOPPED;
	else
		job->status = JOB_RUNNING;
}

static void	apply_wait_result(t_shell *shell, pid_t pid, int status)
{
	t_job		*job;
	t_process	*p;

	job = job_find_by_pid(shell, pid);
	if (!job)
		return ;
	p = process_find(job, pid);
	if (!p)
		return ;
	p->status = status;
	if (WIFSTOPPED(status))
		p->stopped = 1;
	else
	{
		p->completed = 1;
		p->stopped = 0;
		if (WIFSIGNALED(status))
			job->status = JOB_TERMINATED;
	}
	job->notified = 0;
	recompute_job_status(job);
}

void	job_update_statuses(t_shell *shell)
{
	pid_t	pid;
	int		status;

	while (1)
	{
		pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED);
		if (pid <= 0)
			break ;
		apply_wait_result(shell, pid, status);
	}
}

static void	print_notification(t_job *job)
{
	ft_putstr_fd("[", STDERR_FILENO);
	ft_putnbr_fd(job->id, STDERR_FILENO);
	ft_putstr_fd("]  ", STDERR_FILENO);
	ft_putstr_fd((char *)job_status_str(job->status), STDERR_FILENO);
	ft_putstr_fd("\t\t", STDERR_FILENO);
	ft_putstr_fd(job->cmd_line ? job->cmd_line : "", STDERR_FILENO);
	ft_putstr_fd("\n", STDERR_FILENO);
}

static void	lst_remove_node(t_list **head, t_list *target,
	void (*del)(void *))
{
	t_list	*prev;
	t_list	*node;

	if (!head || !*head || !target)
		return ;
	if (*head == target)
	{
		*head = target->next;
		ft_lstdelone(&target, del);
		return ;
	}
	prev = *head;
	node = (*head)->next;
	while (node)
	{
		if (node == target)
		{
			prev->next = node->next;
			ft_lstdelone(&node, del);
			return ;
		}
		prev = node;
		node = node->next;
	}
}

void	job_notify(t_shell *shell)
{
	t_list	*node;
	t_list	*next;
	t_job	*job;

	node = shell->jobs;
	while (node)
	{
		next = node->next;
		job = LST_JOB(node);
		if (!job->notified)
		{
			print_notification(job);
			job->notified = 1;
		}
		if (job->status == JOB_DONE || job->status == JOB_TERMINATED)
		{
			if (shell->current_job == job)
				shell->current_job = NULL;
			lst_remove_node(&shell->jobs, node, job_free);
		}
		node = next;
	}
}

void	job_control_cleanup(t_shell *shell)
{
	ft_lstdel(&shell->jobs, job_free);
	shell->jobs = NULL;
	shell->current_job = NULL;
}
