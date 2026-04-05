/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   executor.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: wengzhang <marvin@42.fr>                   +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/02 16:53:32 by wengzhang         #+#    #+#             */
/*   Updated: 2026/03/27 00:00:00 by wengzhang        ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "42sh.h"
#include "executor.h"

static int	dispatch_node(t_shell *shell, t_ast *ast)
{
	if (ast->type == NODE_COMMAND)
		return (execute_simple_command(shell, &ast->data.cmd));
	if (ast->type == NODE_PIPE)
		return (execute_pipeline(shell, ast));
	if (ast->type == NODE_AND)
		return (execute_and(shell, ast));
	if (ast->type == NODE_OR)
		return (execute_or(shell, ast));
	if (ast->type == NODE_SEQUENCE)
		return (execute_sequence(shell, ast));
	if (ast->type == NODE_SUBSHELL)
		return (execute_subshell(shell, ast));
	if (ast->type == NODE_BLOCK)
		return (execute_block(shell, ast));
	if (ast->type == NODE_BACKGROUND)
		return (execute_background(shell, ast));
	return (1);
}

int	executor_execute(t_shell *shell, t_ast *ast)
{
	int	status;

	if (!ast)
		return (0);
	status = dispatch_node(shell, ast);
	shell->last_exit_status = status;
	return (status);
}
