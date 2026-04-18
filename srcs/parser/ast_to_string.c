/**
 * @file ast_to_string.c
 * @brief Stringify an AST subtree for human-readable job listings (`jobs`).
 * @details Not a perfect round-trip of the input: redirections and
 *          sub-expressions are rendered in a canonical form.
 * @author pulgamecanica
 */

#include "42sh.h"
#include "ast.h"
#include "lexer.h"
#include <stdlib.h>

static char	*join_free(char *a, const char *b)
{
	char	*out;

	if (!a)
		return (b ? ft_strdup(b) : NULL);
	if (!b)
		return (a);
	out = ft_strjoin(a, b);
	free(a);
	return (out);
}

static char	*append_redirs(char *out, t_list *redirs)
{
	t_list			*node;
	t_redir			*r;
	const char		*op;

	node = redirs;
	while (node)
	{
		r = REDIR(node);
		op = " ";
		if (r->type == TOK_REDIR_OUT)
			op = " > ";
		else if (r->type == TOK_REDIR_APPEND)
			op = " >> ";
		else if (r->type == TOK_REDIR_IN)
			op = " < ";
		else if (r->type == TOK_HEREDOC)
			op = " << ";
		out = join_free(out, op);
		if (r->type == TOK_HEREDOC && r->heredoc_delim)
			out = join_free(out, r->heredoc_delim);
		else if (r->target)
			out = join_free(out, r->target);
		node = node->next;
	}
	return (out);
}

static char	*cmd_to_string(t_cmd *cmd)
{
	char	*out;
	t_list	*node;
	int		i;

	out = NULL;
	node = cmd->assignments;
	while (node)
	{
		if (out)
			out = join_free(out, " ");
		out = join_free(out, (char *)node->content);
		node = node->next;
	}
	i = 0;
	while (cmd->argv && cmd->argv[i])
	{
		if (out)
			out = join_free(out, " ");
		out = join_free(out, cmd->argv[i]);
		i++;
	}
	if (!out)
		out = ft_strdup("");
	return (append_redirs(out, cmd->redirs));
}

char	*ast_to_string(t_ast *ast)
{
	char	*left;
	char	*right;
	char	*out;
	const char	*sep;

	if (!ast)
		return (ft_strdup(""));
	if (ast->type == NODE_COMMAND)
		return (cmd_to_string(ast->data.cmd));
	if (ast->type == NODE_SUBSHELL || ast->type == NODE_BLOCK
		|| ast->type == NODE_BACKGROUND)
	{
		left = ast_to_string(ast->data.group->child);
		if (ast->type == NODE_SUBSHELL)
		{
			out = join_free(ft_strdup("( "), left);
			free(left);
			out = join_free(out, " )");
		}
		else if (ast->type == NODE_BLOCK)
		{
			out = join_free(ft_strdup("{ "), left);
			free(left);
			out = join_free(out, " ; }");
		}
		else
			out = join_free(left, " &");
		return (append_redirs(out, ast->data.group->redirs));
	}
	left = ast_to_string(ast->data.binary->left);
	right = ast_to_string(ast->data.binary->right);
	sep = " ; ";
	if (ast->type == NODE_PIPE)
		sep = " | ";
	else if (ast->type == NODE_AND)
		sep = " && ";
	else if (ast->type == NODE_OR)
		sep = " || ";
	out = join_free(left, sep);
	out = join_free(out, right);
	free(right);
	return (out);
}
