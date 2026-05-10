/**
 * @file executor.c
 * @brief Command execution functionality for 42sh.
 * @author wengzhang, pulgamecanica
 */

#include "42sh.h"
#include "executor.h"

#ifdef FT_EXTRA_VERBOSE
static const char	*node_label(t_node_type type)
{
	if (type == NODE_COMMAND)
		return ("NODE_COMMAND");
	if (type == NODE_PIPE)
		return ("NODE_PIPE");
	if (type == NODE_AND)
		return ("NODE_AND");
	if (type == NODE_OR)
		return ("NODE_OR");
	if (type == NODE_SEQUENCE)
		return ("NODE_SEQUENCE");
	if (type == NODE_SUBSHELL)
		return ("NODE_SUBSHELL");
	if (type == NODE_BLOCK)
		return ("NODE_BLOCK");
	if (type == NODE_BACKGROUND)
		return ("NODE_BACKGROUND");
	return ("NODE_?");
}

/**
 * @brief Print a "Dispatch → <type>: <text>" line for compound nodes.
 * @details Skips NODE_COMMAND because execute_simple_command emits its
 *          own, more detailed Pre/Post-expand trace.
 */
static void	debug_dispatch(t_ast *ast)
{
	char	*rendered;

	if (ast->type == NODE_COMMAND)
		return ;
	rendered = ast_to_string(ast);
	fprintf(stderr, "  \033[2mDebug → Dispatch %s: %s\033[0m\n",
		node_label(ast->type), rendered ? rendered : "");
	free(rendered);
}
#endif

static int	dispatch_node(t_shell *shell, t_ast *ast)
{
#ifdef FT_EXTRA_VERBOSE
	debug_dispatch(ast);
#endif
	if (ast->type == NODE_COMMAND)
		return (execute_simple_command(shell, ast->data.cmd));
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
