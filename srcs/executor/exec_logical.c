/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   exec_logical.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: wengzhang <marvin@42.fr>                   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/27 00:00:00 by wengzhang         #+#    #+#             */
/*   Updated: 2026/03/27 00:00:00 by wengzhang        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "42sh.h"
#include "executor.h"

/*
** cmd1 && cmd2 : run right only if left succeeds (exit 0)
*/
int	execute_and(t_shell *shell, t_ast *ast)
{
	int	left_status;

	left_status = executor_execute(shell, ast->data.binary.left);
	if (left_status == 0)
		return (executor_execute(shell, ast->data.binary.right));
	return (left_status);
}

/*
** cmd1 || cmd2 : run right only if left fails (exit != 0)
*/
int	execute_or(t_shell *shell, t_ast *ast)
{
	int	left_status;

	left_status = executor_execute(shell, ast->data.binary.left);
	if (left_status != 0)
		return (executor_execute(shell, ast->data.binary.right));
	return (left_status);
}

/*
** cmd1 ; cmd2 : run both, return status of right
*/
int	execute_sequence(t_shell *shell, t_ast *ast)
{
	executor_execute(shell, ast->data.binary.left);
	return (executor_execute(shell, ast->data.binary.right));
}
