/**
 * @file job_find.c
 * @brief Resolve a bash-style `%spec` to a `t_job*`.
 * @author pulgamecanica
 *
 * Spec forms (all operate on `shell->jobs`):
 *   NULL / ""      — `shell->current_job`
 *   "%" / "%+" / "%%" — `shell->current_job`
 *   "%-"           — the job immediately preceding `current_job` in the list
 *   "%N"           — job whose `id` equals N
 *   "%prefix"      — first job whose `cmd_line` starts with `prefix`
 */

#include "42sh.h"
#include "job_control.h"

static int	is_all_digits(const char *s)
{
	if (!s || !*s)
		return (0);
	while (*s)
	{
		if (!ft_isdigit((unsigned char)*s))
			return (0);
		s++;
	}
	return (1);
}

static t_job	*find_previous_current(t_shell *shell)
{
	t_list	*prev;
	t_list	*node;

	if (!shell->current_job)
		return (NULL);
	prev = NULL;
	node = shell->jobs;
	while (node)
	{
		if (LST_JOB(node) == shell->current_job)
		{
			if (prev)
				return (LST_JOB(prev));
			return (NULL);
		}
		prev = node;
		node = node->next;
	}
	return (NULL);
}

static t_job	*find_by_prefix(t_shell *shell, const char *prefix)
{
	t_list	*node;
	size_t	plen;
	t_job	*j;

	plen = ft_strlen(prefix);
	node = shell->jobs;
	while (node)
	{
		j = LST_JOB(node);
		if (j->cmd_line && !ft_strncmp(j->cmd_line, prefix, plen))
			return (j);
		node = node->next;
	}
	return (NULL);
}

t_job	*job_find_by_spec(t_shell *shell, const char *spec)
{
	if (!spec || !*spec)
		return (shell->current_job);
	if (!ft_strcmp(spec, "%") || !ft_strcmp(spec, "%+")
		|| !ft_strcmp(spec, "%%"))
		return (shell->current_job);
	if (spec[0] != '%')
		return (NULL);
	spec++;
	if (!ft_strcmp(spec, "-"))
		return (find_previous_current(shell));
	if (is_all_digits(spec))
		return (job_find_by_id(shell, ft_atoi(spec)));
	return (find_by_prefix(shell, spec));
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
