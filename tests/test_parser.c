/**
 * @file test_parser.c
 * @brief Unit tests for the 42sh parser module.
 *
 * ## How to enable
 *
 * Add `-DTEST_PARSER_ENABLED` to `TEST_FLAGS` in the root Makefile and declare
 * `test_parser_suite` in test_runner.c.  Once all assertions are permanently
 * green, remove the `#ifdef` guards and the `-D` flag per the workflow
 * described in test_runner.c.
 */

#ifdef TEST_PARSER_ENABLED
#endif /* TEST_PARSER_ENABLED */

# include "minunit.h"
# include "parser.h"
# include <stdlib.h>
# include <string.h>
# include <unistd.h>

static void	test_simple_command(void)
{
	t_ast	*ast;

	ast = parser_parse(lexer_tokenize("ls"));
	MU_ASSERT("ast not NULL", ast != NULL);
	MU_ASSERT_INT(NODE_COMMAND, ast->type);
	MU_ASSERT_STR("argv[0]", "ls", ast->data.cmd->argv[0]);
	ast_free(ast);
}

static void	test_command_args(void)
{
	t_ast	*ast;

	ast = parser_parse(lexer_tokenize("ls -la"));
	MU_ASSERT("ast not NULL", ast != NULL);
	MU_ASSERT_INT(NODE_COMMAND, ast->type);
	MU_ASSERT_STR("argv[0]", "ls", ast->data.cmd->argv[0]);
	MU_ASSERT_STR("argv[1]", "-la", ast->data.cmd->argv[1]);
	ast_free(ast);
}

static void	test_pipe(void)
{
	t_ast	*ast;

	ast = parser_parse(lexer_tokenize("ls | grep foo"));
	MU_ASSERT("ast not NULL", ast != NULL);
	MU_ASSERT_INT(NODE_PIPE, ast->type);
	MU_ASSERT_STR("left cmd", "ls", ast->data.binary->left->data.cmd->argv[0]);
	MU_ASSERT_STR("right cmd", "grep", ast->data.binary->right->data.cmd->argv[0]);
	MU_ASSERT_STR("right arg", "foo", ast->data.binary->right->data.cmd->argv[1]);
	ast_free(ast);
}

static void	test_and(void)
{
	t_ast	*ast;

	ast = parser_parse(lexer_tokenize("ls && echo ok"));
	MU_ASSERT("ast not NULL", ast != NULL);
	MU_ASSERT_INT(NODE_AND, ast->type);
	MU_ASSERT_STR("left cmd", "ls", ast->data.binary->left->data.cmd->argv[0]);
	MU_ASSERT_STR("right cmd", "echo", ast->data.binary->right->data.cmd->argv[0]);
	MU_ASSERT_STR("right arg", "ok", ast->data.binary->right->data.cmd->argv[1]);
	ast_free(ast);
}

static void	test_or(void)
{
	t_ast	*ast;

	ast = parser_parse(lexer_tokenize("ls || echo fail"));
	MU_ASSERT("ast not NULL", ast != NULL);
	MU_ASSERT_INT(NODE_OR, ast->type);
	MU_ASSERT_STR("left cmd", "ls", ast->data.binary->left->data.cmd->argv[0]);
	MU_ASSERT_STR("right cmd", "echo", ast->data.binary->right->data.cmd->argv[0]);
	MU_ASSERT_STR("right arg", "fail", ast->data.binary->right->data.cmd->argv[1]);
	ast_free(ast);
}

static void test_sequence(void)
{
	t_ast	*ast = parser_parse(lexer_tokenize("ls ; pwd"));

	MU_ASSERT_INT(NODE_SEQUENCE, ast->type);
	MU_ASSERT_STR("left", "ls", ast->data.binary->left->data.cmd->argv[0]);
	MU_ASSERT_STR("right", "pwd", ast->data.binary->right->data.cmd->argv[0]);

	ast_free(ast);
}

static void test_subshell_pipe(void)
{
	t_ast	*ast = parser_parse(lexer_tokenize("(ls ; pwd) | grep foo"));
	t_ast	*left = ast->data.binary->left;
	t_ast	*child = left->data.group->child;

	MU_ASSERT_INT(NODE_PIPE, ast->type);
	MU_ASSERT_INT(NODE_SUBSHELL, left->type);
	MU_ASSERT_INT(NODE_SEQUENCE, child->type);
	MU_ASSERT_STR("child left", "ls", child->data.binary->left->data.cmd->argv[0]);
	MU_ASSERT_STR("child right", "pwd", child->data.binary->right->data.cmd->argv[0]);

	ast_free(ast);
}

static void test_subshell_redir(void)
{
	t_ast	*ast = parser_parse(lexer_tokenize("(ls) > file"));

	MU_ASSERT_INT(NODE_SUBSHELL, ast->type);
	MU_ASSERT("has redirs", ast->data.group->redirs != NULL);
	MU_ASSERT_INT(TOK_REDIR_OUT,  ((t_redir *)(ast->data.group->redirs->content))->type);
	MU_ASSERT_STR("redir", "file",  ((t_redir *)(ast->data.group->redirs->content))->target);

	ast_free(ast);
}

static void test_redirection(void)
{
	t_ast	*ast = parser_parse(lexer_tokenize("ls > file"));

	MU_ASSERT_INT(NODE_COMMAND, ast->type);
	MU_ASSERT("has redirs", ast->data.cmd->redirs != NULL);
	MU_ASSERT_INT(TOK_REDIR_OUT,  ((t_redir *)(ast->data.cmd->redirs->content))->type);
	MU_ASSERT_STR("redir", "file",  ((t_redir *)(ast->data.cmd->redirs->content))->target);

	ast_free(ast);
}

static void test_complex_redir(void)
{
	t_ast	*ast = parser_parse(lexer_tokenize("ls >> file 2>&1"));

	MU_ASSERT_INT(NODE_COMMAND, ast->type);
	MU_ASSERT("has redirs", ast->data.cmd->redirs != NULL);
	MU_ASSERT_INT(TOK_REDIR_APPEND,  ((t_redir *)(ast->data.cmd->redirs->content))->type);
	MU_ASSERT_STR("redir", "file",  ((t_redir *)(ast->data.cmd->redirs->content))->target);
	MU_ASSERT_INT(TOK_REDIR_DUP_OUT,  ((t_redir *)(ast->data.cmd->redirs->next->content))->type);
	MU_ASSERT_INT(2,  ((t_redir *)(ast->data.cmd->redirs->next->content))->fd);

	ast_free(ast);
}

static void	test_assignment(void)
{
	t_ast	*ast = parser_parse(lexer_tokenize("VAR=value cmd"));

	MU_ASSERT_INT(NODE_COMMAND, ast->type);
	MU_ASSERT_STR("cmd", "cmd", ast->data.cmd->argv[0]);
	MU_ASSERT("has redirs", ast->data.cmd->assignments != NULL);
	MU_ASSERT_STR("assignment", "VAR=value", ast->data.cmd->assignments->content);
	ast_free(ast);
}

static void test_multiple_redirs(void)
{
	t_ast	*ast = parser_parse(lexer_tokenize("< input cmd > output"));

	MU_ASSERT_INT(NODE_COMMAND, ast->type);
	MU_ASSERT_STR("cmd", "cmd", ast->data.cmd->argv[0]);
	MU_ASSERT("has redirs", ast->data.cmd->redirs != NULL);

	MU_ASSERT_INT(TOK_REDIR_IN,  ((t_redir *)(ast->data.cmd->redirs->content))->type);
	MU_ASSERT_STR("first redir", "input",  ((t_redir *)(ast->data.cmd->redirs->content))->target);
	MU_ASSERT_INT(TOK_REDIR_OUT,  ((t_redir *)(ast->data.cmd->redirs->next->content))->type);
	MU_ASSERT_STR("second redir", "output",  ((t_redir *)(ast->data.cmd->redirs->next->content))->target);

	ast_free(ast);
}

static void test_background_separator(void)
{
	t_ast	*ast = parser_parse(lexer_tokenize("sleep 10 & echo done"));

	MU_ASSERT("ast not NULL", ast != NULL);
	MU_ASSERT_INT(NODE_SEQUENCE, ast->type);

	t_ast *left = ast->data.binary->left;
	t_ast *right = ast->data.binary->right;

	MU_ASSERT_INT(NODE_BACKGROUND, left->type);
	MU_ASSERT_STR("bg cmd", "sleep", left->data.group->child->data.cmd->argv[0]);
	MU_ASSERT_STR("arg", "10", left->data.group->child->data.cmd->argv[1]);

	MU_ASSERT_STR("fg cmd", "echo", right->data.cmd->argv[0]);
	MU_ASSERT_STR("fg arg", "done", right->data.cmd->argv[1]);

	ast_free(ast);
}

static void test_multiple_background_chain(void)
{
	t_ast	*ast = parser_parse(lexer_tokenize("cmd1 & cmd2 & cmd3"));

	MU_ASSERT_INT(NODE_SEQUENCE, ast->type);

	t_ast *left = ast->data.binary->left;
	t_ast *right = ast->data.binary->right;

	MU_ASSERT_INT(NODE_BACKGROUND, left->type);
	MU_ASSERT_INT(NODE_SEQUENCE, right->type);

	MU_ASSERT_STR("cmd1", "cmd1",
			left->data.group->child->data.cmd->argv[0]);

	MU_ASSERT_INT(NODE_BACKGROUND, right->data.binary->left->type);
	MU_ASSERT_STR("cmd2", "cmd2",
			right->data.binary->left->data.group->child->data.cmd->argv[0]);

	MU_ASSERT_STR("cmd3", "cmd3",
			right->data.binary->right->data.cmd->argv[0]);

	ast_free(ast);
}

void	test_parser_suite(void)
{
	test_simple_command();
	test_command_args();
	test_pipe();
	test_and();
	test_or();
	test_sequence();
	test_subshell_pipe();
	test_subshell_redir();
	test_redirection();
	test_complex_redir();
	test_assignment();
	test_multiple_redirs();
	test_background_separator();
	test_multiple_background_chain();
}
