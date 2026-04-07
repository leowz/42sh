/**
 * @file exec_logical.c
 * @brief Logical command execution functionality for 42sh.
 * @author wengzhang, pulgamecanica
 */

#include "42sh.h"
#include "executor.h"

/**
 * @brief Execute logical AND operation.
 * @details cmd1 && cmd2 : run right only if left succeeds (exit 0)
 * @param shell The shell instance.
 * @param ast The abstract syntax tree node.
 * @return The exit status of the operation.
 */
int	execute_and(t_shell *shell, t_ast *ast)
{
	int	left_status;

	left_status = executor_execute(shell, ast->data.binary->left);
	if (left_status == 0)
		return (executor_execute(shell, ast->data.binary->right));
	return (left_status);
}

/**
 * @brief Execute logical OR operation.
 * @details cmd1 || cmd2 : run right only if left fails (exit != 0)
 * @param shell The shell instance.
 * @param ast The abstract syntax tree node.
 * @return The exit status of the operation.
 */
int	execute_or(t_shell *shell, t_ast *ast)
{
	int	left_status;

	left_status = executor_execute(shell, ast->data.binary->left);
	if (left_status != 0)
		return (executor_execute(shell, ast->data.binary->right));
	return (left_status);
}

/**
 * @brief Execute sequential commands.
 * @details cmd1 ; cmd2 : run both, return status of right
 * @param shell The shell instance.
 * @param ast The abstract syntax tree node.
 * @return The exit status of the operation.
 */
int	execute_sequence(t_shell *shell, t_ast *ast)
{
	executor_execute(shell, ast->data.binary->left);
	return (executor_execute(shell, ast->data.binary->right));
}
