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

#define BUFSIZE 256

static void	test_simple_command(void)
{
	t_ast	*ast;
	t_shell	shell;

	ast = parser_parse(lexer_tokenize("ls"), &shell);
	MU_ASSERT("ast not NULL", ast != NULL);
	MU_ASSERT_INT(NODE_COMMAND, ast->type);
	MU_ASSERT_STR("argv[0]", "ls", ast->data.cmd->argv[0]);
	ast_free(ast);
}

static void	test_command_args(void)
{
	t_ast	*ast;
	t_shell	shell;

	ast = parser_parse(lexer_tokenize("ls -la"), &shell);
	MU_ASSERT("ast not NULL", ast != NULL);
	MU_ASSERT_INT(NODE_COMMAND, ast->type);
	MU_ASSERT_STR("argv[0]", "ls", ast->data.cmd->argv[0]);
	MU_ASSERT_STR("argv[1]", "-la", ast->data.cmd->argv[1]);
	ast_free(ast);
}

static void	test_pipe(void)
{
	t_ast	*ast;
	t_shell	shell;

	ast = parser_parse(lexer_tokenize("ls | grep foo"), &shell);
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
	t_shell	shell;

	ast = parser_parse(lexer_tokenize("ls && echo ok"), &shell);
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
	t_shell	shell;

	ast = parser_parse(lexer_tokenize("ls || echo fail"), &shell);
	MU_ASSERT("ast not NULL", ast != NULL);
	MU_ASSERT_INT(NODE_OR, ast->type);
	MU_ASSERT_STR("left cmd", "ls", ast->data.binary->left->data.cmd->argv[0]);
	MU_ASSERT_STR("right cmd", "echo", ast->data.binary->right->data.cmd->argv[0]);
	MU_ASSERT_STR("right arg", "fail", ast->data.binary->right->data.cmd->argv[1]);
	ast_free(ast);
}

static void test_sequence(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("ls ; pwd"), &shell);

	MU_ASSERT_INT(NODE_SEQUENCE, ast->type);
	MU_ASSERT_STR("left", "ls", ast->data.binary->left->data.cmd->argv[0]);
	MU_ASSERT_STR("right", "pwd", ast->data.binary->right->data.cmd->argv[0]);

	ast_free(ast);
}

static void test_subshell_pipe(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("(ls ; pwd) | grep foo"), &shell);
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
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("(ls) > file"), &shell);

	MU_ASSERT_INT(NODE_SUBSHELL, ast->type);
	MU_ASSERT("has redirs", ast->data.group->redirs != NULL);
	MU_ASSERT_INT(TOK_REDIR_OUT,  ((t_redir *)(ast->data.group->redirs->content))->type);
	MU_ASSERT_STR("redir", "file",  ((t_redir *)(ast->data.group->redirs->content))->target);

	ast_free(ast);
}

static void test_redirection(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("ls > file"), &shell);

	MU_ASSERT_INT(NODE_COMMAND, ast->type);
	MU_ASSERT("has redirs", ast->data.cmd->redirs != NULL);
	MU_ASSERT_INT(TOK_REDIR_OUT,  ((t_redir *)(ast->data.cmd->redirs->content))->type);
	MU_ASSERT_STR("redir", "file",  ((t_redir *)(ast->data.cmd->redirs->content))->target);

	ast_free(ast);
}

static void test_complex_redir(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("ls >> file 2>&1"), &shell);

	MU_ASSERT_INT(NODE_COMMAND, ast->type);
	MU_ASSERT("has redirs", ast->data.cmd->redirs != NULL);
	MU_ASSERT_INT(TOK_REDIR_APPEND,  ((t_redir *)(ast->data.cmd->redirs->content))->type);
	MU_ASSERT_STR("redir", "file",  ((t_redir *)(ast->data.cmd->redirs->content))->target);
	MU_ASSERT_INT(TOK_REDIR_DUP_OUT,  ((t_redir *)(ast->data.cmd->redirs->next->content))->type);
	MU_ASSERT_INT(2,  ((t_redir *)(ast->data.cmd->redirs->next->content))->fd);

	ast_free(ast);
}

static void test_multiple_redirs(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("< input cmd > output"), &shell);

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
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("sleep 10 & echo done"), &shell);

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
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("cmd1 & cmd2 & cmd3"), &shell);

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

static void test_trailing_background(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("cmd1 & cmd2 &"), &shell);

	MU_ASSERT("ast not NULL", ast != NULL);
	MU_ASSERT_INT(NODE_SEQUENCE, ast->type);

	t_ast *left = ast->data.binary->left;
	t_ast *right = ast->data.binary->right;

	MU_ASSERT_INT(NODE_BACKGROUND, left->type);
	MU_ASSERT_INT(NODE_BACKGROUND, right->type);

	MU_ASSERT_STR("cmd1", "cmd1",
			left->data.group->child->data.cmd->argv[0]);
	MU_ASSERT_STR("cmd2", "cmd2",
			right->data.group->child->data.cmd->argv[0]);

	ast_free(ast);
}
static void test_pipe_newline(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("ls |\n grep foo"), &shell);

	MU_ASSERT("ast not NULL", ast != NULL);
	MU_ASSERT_INT(NODE_PIPE, ast->type);

	MU_ASSERT_STR("left", "ls",
			ast->data.binary->left->data.cmd->argv[0]);
	MU_ASSERT_STR("right", "grep",
			ast->data.binary->right->data.cmd->argv[0]);
	MU_ASSERT_STR("arg", "foo",
			ast->data.binary->right->data.cmd->argv[1]);
	ast_free(ast);
}

static void test_and_newline(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("true &&\n echo ok"), &shell);

	MU_ASSERT("ast not NULL", ast != NULL);
	MU_ASSERT_INT(NODE_AND, ast->type);

	MU_ASSERT_STR("left", "true",
			ast->data.binary->left->data.cmd->argv[0]);
	MU_ASSERT_STR("right", "echo",
			ast->data.binary->right->data.cmd->argv[0]);
	MU_ASSERT_STR("arg", "ok",
			ast->data.binary->right->data.cmd->argv[1]);

	ast_free(ast);
}

static void	test_only_pipe(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("|"), &shell);

	MU_ASSERT("ast NULL", !ast);

	ast_free(ast);
}

static void test_only_and(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("&&"), &shell);

	MU_ASSERT("ast NULL", !ast);

	ast_free(ast);
}

static void	test_only_or_and(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("|| &&"), &shell);

	MU_ASSERT("ast NULL", !ast);

	ast_free(ast);
}

static void	test_unclose_parenthesis(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("(ls"), &shell);

	MU_ASSERT("ast NULL", !ast);

	ast_free(ast);
}

static void	test_unopen_parenthesis(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize(")"), &shell);

	MU_ASSERT("ast NULL", !ast);

	ast_free(ast);
}

static void	test_redirs_in_a_row(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("> < file"), &shell);

	MU_ASSERT("ast NULL", !ast);

	ast_free(ast);
}

static t_ast	*write_to_pipe(char *content, char *cmd)
{
	t_shell	shell;
	int		pipefd[2];

	t_ast	*ast;

	pipe(pipefd);
	write(pipefd[1], content, strlen(content));
	write(pipefd[1], "EOF\n", 4);
	close(pipefd[1]);

	int saved_stdin = dup(STDIN_FILENO);
	dup2(pipefd[0], STDIN_FILENO);
	close(pipefd[0]);

	ast = parser_parse(lexer_tokenize(cmd), &shell);

	dup2(saved_stdin, STDIN_FILENO);
	close(saved_stdin);
	return (ast);
}

static void test_heredoc_basic(void)
{
	t_ast	*ast;
	t_redir	*redir;
	char	buf[BUFSIZE];
	ssize_t	n;

	ast = write_to_pipe("hello\n", "cat << EOF");
	redir = REDIR(ast->data.cmd->redirs);

	MU_ASSERT("ast not NULL", ast != NULL);
	MU_ASSERT_INT(NODE_COMMAND, ast->type);
	MU_ASSERT("has redirs", ast->data.cmd->redirs != NULL);
	MU_ASSERT_INT(TOK_HEREDOC, redir->type);
	MU_ASSERT("heredoc_fd >= 0", redir->heredoc_fd >= 0);
	n = read(redir->heredoc_fd, buf, BUFSIZE);
	buf[n] = '\0';
	MU_ASSERT_STR("heredoc content", "hello\n", buf);
	close(redir->heredoc_fd);
}

static void test_heredoc_stripped_basic(void)
{
	ssize_t	n;
	char	buf[BUFSIZE];
	t_ast	*ast;
	t_redir	*redir;

	ast = write_to_pipe("hello\n", "cat <<- EOF");
	redir = REDIR(ast->data.cmd->redirs);

	MU_ASSERT("ast not NULL", ast != NULL);
	MU_ASSERT_INT(NODE_COMMAND, ast->type);
	MU_ASSERT("has redirs", ast->data.cmd->redirs != NULL);
	MU_ASSERT("heredoc_fd >= 0", redir->heredoc_fd >= 0);
	n = read(redir->heredoc_fd, buf, BUFSIZE);
	buf[n] = '\0';
	MU_ASSERT_INT(TOK_HEREDOC_STRIP, redir->type);
	MU_ASSERT_STR("heredoc content", "hello\n", buf);

	close(redir->heredoc_fd);
}

static void test_heredoc_multiline(void)
{
	ssize_t	n;
	char	buf[BUFSIZE];
	t_ast	*ast;
	t_redir	*redir;

	ast = write_to_pipe("line1\nline2\nline3\n", "cat << EOF");
	redir = REDIR(ast->data.cmd->redirs);

	MU_ASSERT("ast not NULL", ast != NULL);
	MU_ASSERT("has redirs", ast->data.cmd->redirs != NULL);
	MU_ASSERT("heredoc_fd >= 0", redir->heredoc_fd >= 0);
	n = read(redir->heredoc_fd, buf, BUFSIZE);
	buf[n] = '\0';
	MU_ASSERT_INT(TOK_HEREDOC, redir->type);
	MU_ASSERT_STR("heredoc content", "line1\nline2\nline3\n", buf);

	close(redir->heredoc_fd);
}

static void test_heredoc_stripped_multiline(void)
{
	ssize_t	n;
	char	buf[BUFSIZE];
	t_ast	*ast;
	t_redir	*redir;

	ast = write_to_pipe("\tline1\nline2\n\t\tline3\n\t\t\t", "cat <<-EOF");
	redir = REDIR(ast->data.cmd->redirs);

	MU_ASSERT("ast not NULL", ast != NULL);
	MU_ASSERT("has redirs", ast->data.cmd->redirs != NULL);
	MU_ASSERT("heredoc_fd >= 0", redir->heredoc_fd >= 0);
	n = read(redir->heredoc_fd, buf, BUFSIZE);
	buf[n] = '\0';
	MU_ASSERT_INT(TOK_HEREDOC_STRIP, redir->type);
	MU_ASSERT_STR("heredoc content", "line1\nline2\nline3\n", buf);

	close(redir->heredoc_fd);
}

static void test_heredoc_quoted_no_expand(void)
{
	ssize_t	n;
	char	buf[BUFSIZE];
	t_ast	*ast;
	t_redir	*redir;

	ast = write_to_pipe("$USER\nEOF\n", "cat << 'EOF'");
	redir = REDIR(ast->data.cmd->redirs);

	MU_ASSERT("ast not NULL", ast != NULL);
	MU_ASSERT_INT(TOK_HEREDOC, redir->type);
	MU_ASSERT("heredoc_fd >= 0", redir->heredoc_fd >= 0);
	n = read(redir->heredoc_fd, buf, BUFSIZE);
	buf[n] = '\0';
	MU_ASSERT_STR("heredoc content", "$USER\n", buf);
	MU_ASSERT("no expand", redir->heredoc_quoted == 1);

	close(redir->heredoc_fd);
}

static void test_heredoc_stripped_quoted_no_expand(void)
{
	ssize_t	n;
	char	buf[BUFSIZE];
	t_ast	*ast;
	t_redir	*redir;

	ast = write_to_pipe("\t$USER\n\t", "cat <<- 'EOF'");
	redir = REDIR(ast->data.cmd->redirs);

	MU_ASSERT("ast not NULL", ast != NULL);
	MU_ASSERT_INT(TOK_HEREDOC_STRIP, redir->type);
	MU_ASSERT("heredoc_fd >= 0", redir->heredoc_fd >= 0);
	n = read(redir->heredoc_fd, buf, BUFSIZE);
	buf[n] = '\0';
	MU_ASSERT_STR("heredoc content", "$USER\n", buf);
	MU_ASSERT("no expand", redir->heredoc_quoted == 1);

	close(redir->heredoc_fd);
}

static void test_heredoc_pipe(void)
{
	ssize_t	n;
	char	buf[BUFSIZE];
	t_ast	*ast;
	t_redir	*redir;

	ast = write_to_pipe("hello\nEOF\n", "cat <<EOF | grep hello");
	printf("DEBUG!!!\n");
	redir = REDIR(ast->data.binary->left->data.cmd->redirs);

	MU_ASSERT("ast not NULL", ast != NULL);
	MU_ASSERT_INT(NODE_PIPE, ast->type);
	MU_ASSERT_STR("left cmd", "cat",
			ast->data.binary->left->data.cmd->argv[0]);
	MU_ASSERT_STR("right cmd", "grep",
			ast->data.binary->right->data.cmd->argv[0]);
	n = read(redir->heredoc_fd, buf, BUFSIZE);
	buf[n] = '\0';
	MU_ASSERT_INT(TOK_HEREDOC, redir->type);
	MU_ASSERT_STR("heredoc content", "hello\n", buf);

	close(redir->heredoc_fd);
}

static void test_heredoc_stripped_pipe(void)
{
	ssize_t	n;
	char	buf[BUFSIZE];
	t_ast	*ast;
	t_redir	*redir;

	ast = write_to_pipe("\thello\n\t\t", "cat <<- \"EOF\" | grep hello");
	redir = REDIR(ast->data.binary->left->data.cmd->redirs);

	MU_ASSERT("ast not NULL", ast != NULL);
	MU_ASSERT_INT(NODE_PIPE, ast->type);
	MU_ASSERT_STR("left cmd", "cat",
			ast->data.binary->left->data.cmd->argv[0]);
	MU_ASSERT_STR("right cmd", "grep",
			ast->data.binary->right->data.cmd->argv[0]);
	n = read(redir->heredoc_fd, buf, BUFSIZE);
	buf[n] = '\0';
	MU_ASSERT_INT(TOK_HEREDOC_STRIP, redir->type);
	MU_ASSERT_STR("heredoc content", "hello\n", buf);

	close(redir->heredoc_fd);
}

static void test_heredoc_with_redir(void)
{
	t_ast	*ast;
	t_list	*redirs;

	ast = write_to_pipe("hello\n", "cat << EOF > out");

	MU_ASSERT("ast not NULL", ast != NULL);
	MU_ASSERT_INT(NODE_COMMAND, ast->type);
	redirs = ast->data.cmd->redirs;
	MU_ASSERT("has redirs", redirs != NULL);
	MU_ASSERT_INT(TOK_HEREDOC, ((t_redir *)(redirs->content))->type);
	MU_ASSERT_INT(TOK_REDIR_OUT, ((t_redir *)(redirs->next->content))->type);
	MU_ASSERT_STR("out file", "out", ((t_redir *)(redirs->next->content))->target);
	close(((t_redir *)redirs->content)->heredoc_fd);
}

static void test_heredoc_stripped_with_redir(void)
{
	t_ast	*ast;
	t_list	*redirs;

	ast = write_to_pipe("\t\thello\n\t", "cat <<- EOF > out");

	MU_ASSERT("ast not NULL", ast != NULL);
	MU_ASSERT_INT(NODE_COMMAND, ast->type);
	redirs = ast->data.cmd->redirs;
	MU_ASSERT("has redirs", redirs != NULL);
	MU_ASSERT_INT(TOK_HEREDOC_STRIP, ((t_redir *)(redirs->content))->type);
	MU_ASSERT_INT(TOK_REDIR_OUT, ((t_redir *)(redirs->next->content))->type);
	MU_ASSERT_STR("out file", "out", ((t_redir *)(redirs->next->content))->target);
	close(((t_redir *)redirs->content)->heredoc_fd);
}

static void test_heredoc_unterminated(void)
{
	ssize_t	n;
	char	buf[BUFSIZE];
	t_ast	*ast;
	t_redir	*redir;

	ast = write_to_pipe("hello\n", "cat << EOF");
	redir = REDIR(ast->data.cmd->redirs);

	MU_ASSERT("ast not NULL", ast != NULL);
	MU_ASSERT_INT(NODE_COMMAND, ast->type);
	MU_ASSERT("has redirs", ast->data.cmd->redirs != NULL);
	MU_ASSERT("heredoc_fd >= 0", redir->heredoc_fd >= 0);
	n = read(redir->heredoc_fd, buf, BUFSIZE);
	buf[n] = '\0';
	MU_ASSERT_INT(TOK_HEREDOC, redir->type);
	MU_ASSERT_STR("heredoc content", "hello\n", buf);

	close(redir->heredoc_fd);
}

static void test_heredoc_stripped_unterminated(void)
{
	ssize_t	n;
	char	buf[BUFSIZE];
	t_ast	*ast;
	t_redir	*redir;

	ast = write_to_pipe("\t\thello\n", "cat <<- EOF");
	redir = REDIR(ast->data.cmd->redirs);

	MU_ASSERT("ast not NULL", ast != NULL);
	MU_ASSERT_INT(NODE_COMMAND, ast->type);
	MU_ASSERT("has redirs", ast->data.cmd->redirs != NULL);
	MU_ASSERT("heredoc_fd >= 0", redir->heredoc_fd >= 0);
	n = read(redir->heredoc_fd, buf, BUFSIZE);
	buf[n] = '\0';
	MU_ASSERT_INT(TOK_HEREDOC_STRIP, redir->type);
	MU_ASSERT_STR("heredoc content", "hello\n", buf);

	close(redir->heredoc_fd);
}

static void test_heredoc_group(void)
{
	ssize_t	n;
	char	buf[BUFSIZE];
	t_ast	*ast;
	t_redir	*redir;

	ast = write_to_pipe("hello\n", "(cat) << EOF");
	redir = REDIR(ast->data.group->redirs);


	MU_ASSERT("ast not NULL", ast != NULL);
	MU_ASSERT_INT(NODE_SUBSHELL, ast->type);
	MU_ASSERT_STR("cmd", "cat", ast->data.group->child->data.cmd->argv[0]);
	MU_ASSERT("heredoc_fd >= 0", redir->heredoc_fd >= 0);
	n = read(redir->heredoc_fd, buf, BUFSIZE);
	buf[n] = '\0';
	MU_ASSERT_INT(TOK_HEREDOC, redir->type);
	MU_ASSERT_STR("heredoc content", "hello\n", buf);

	close(redir->heredoc_fd);
}

static void test_heredoc_stripped_group(void)
{
	ssize_t	n;
	char	buf[BUFSIZE];
	t_ast	*ast;
	t_redir	*redir;

	ast = write_to_pipe("\t\t\thello\n\t", "(cat) <<- EOF");
	redir = REDIR(ast->data.group->redirs);

	MU_ASSERT("ast not NULL", ast != NULL);
	MU_ASSERT_INT(NODE_SUBSHELL, ast->type);
	MU_ASSERT("has redirs", ast->data.cmd->redirs != NULL);
	MU_ASSERT("heredoc_fd >= 0", redir->heredoc_fd >= 0);
	n = read(redir->heredoc_fd, buf, BUFSIZE);
	buf[n] = '\0';
	MU_ASSERT_INT(TOK_HEREDOC_STRIP, redir->type);
	MU_ASSERT_STR("heredoc content", "hello\n", buf);

	close(redir->heredoc_fd);
}

static void	test_heredoc_empty(void)
{
	ssize_t	n;
	char	buf[BUFSIZE];
	t_ast	*ast;
	t_redir	*redir;

	ast = write_to_pipe("", "cat << EOF");
	redir = REDIR(ast->data.cmd->redirs);


	MU_ASSERT("heredoc_fd >= 0", redir->heredoc_fd >= 0);
	n = read(redir->heredoc_fd, buf, BUFSIZE);
	MU_ASSERT_INT(0, (int)n);

	close(redir->heredoc_fd);
}

static void	test_heredoc_stripped_empty(void)
{
	ssize_t	n;
	char	buf[BUFSIZE];
	t_ast	*ast;
	t_redir	*redir;

	ast = write_to_pipe("", "cat <<- EOF");
	redir = REDIR(ast->data.cmd->redirs);

	MU_ASSERT("heredoc_fd >= 0", redir->heredoc_fd >= 0);
	n = read(redir->heredoc_fd, buf, BUFSIZE);
	MU_ASSERT_INT(0, (int)n);

	close(redir->heredoc_fd);
}

static void	test_no_heredoc(void)
{
	t_ast	*ast;
	t_redir	*redir;
	t_shell	shell;

	ast	= parser_parse(lexer_tokenize("no heredoc > out"), &shell);
	redir = REDIR(ast->data.cmd->redirs);


	MU_ASSERT_INT(-1, redir->heredoc_fd);

	close(redir->heredoc_fd);
	ast_free(ast);
}

static void	test_assignment(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("VAR=value cmd"), &shell);

	MU_ASSERT_INT(NODE_COMMAND, ast->type);
	MU_ASSERT_STR("cmd", "cmd", ast->data.cmd->argv[0]);
	MU_ASSERT("has redirs", ast->data.cmd->assignments != NULL);
	MU_ASSERT_STR("assignment", "VAR=value", ast->data.cmd->assignments->content);
	ast_free(ast);
}

static void test_multiple_assignments(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("A=1 B=2 C=3 cmd"), &shell);

	MU_ASSERT_INT(NODE_COMMAND, ast->type);
	MU_ASSERT_STR("cmd", "cmd", ast->data.cmd->argv[0]);

	t_list *a = ast->data.cmd->assignments;
	MU_ASSERT_STR("A=1", "A=1", a->content);
	MU_ASSERT_STR("B=2", "B=2", a->next->content);
	MU_ASSERT_STR("C=3", "C=3", a->next->next->content);

	ast_free(ast);
}

static void test_assignment_only(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("VAR=value"), &shell);

	MU_ASSERT_INT(NODE_COMMAND, ast->type);
	MU_ASSERT("no argv or empty", ast->data.cmd->argv == NULL
			|| ast->data.cmd->argv[0] == NULL);

	MU_ASSERT("has assignment", ast->data.cmd->assignments != NULL);
	MU_ASSERT_STR("assignment", "VAR=value",
			ast->data.cmd->assignments->content);

	ast_free(ast);
}

static void test_assignment_after_command(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("echo VAR=value"), &shell);

	MU_ASSERT_INT(NODE_COMMAND, ast->type);

	MU_ASSERT_STR("cmd", "echo", ast->data.cmd->argv[0]);
	MU_ASSERT_STR("arg", "VAR=value", ast->data.cmd->argv[1]);

	MU_ASSERT("no assignments", ast->data.cmd->assignments == NULL);

	ast_free(ast);
}

static void test_assignment_and_args(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("A=1 cmd arg1 arg2"), &shell);

	MU_ASSERT_INT(NODE_COMMAND, ast->type);

	MU_ASSERT_STR("cmd", "cmd", ast->data.cmd->argv[0]);
	MU_ASSERT_STR("arg1", "arg1", ast->data.cmd->argv[1]);
	MU_ASSERT_STR("arg2", "arg2", ast->data.cmd->argv[2]);

	MU_ASSERT_STR("assignment", "A=1",
			ast->data.cmd->assignments->content);

	ast_free(ast);
}

static void test_assignment_with_redir(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("A=1 cmd > file"), &shell);

	MU_ASSERT_INT(NODE_COMMAND, ast->type);

	MU_ASSERT_STR("cmd", "cmd", ast->data.cmd->argv[0]);
	MU_ASSERT("assignment exists", ast->data.cmd->assignments != NULL);
	MU_ASSERT("redir exists", ast->data.cmd->redirs != NULL);

	MU_ASSERT_STR("assignment", "A=1",
			ast->data.cmd->assignments->content);
	MU_ASSERT_STR("redir target", "file",
			((t_redir *)ast->data.cmd->redirs->content)->target);

	ast_free(ast);
}

static void test_assignment_in_pipeline(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("A=1 cmd1 | B=2 cmd2"), &shell);

	MU_ASSERT_INT(NODE_PIPE, ast->type);

	t_ast *left = ast->data.binary->left;
	t_ast *right = ast->data.binary->right;

	MU_ASSERT_STR("left cmd", "cmd1", left->data.cmd->argv[0]);
	MU_ASSERT_STR("left assign", "A=1",
			left->data.cmd->assignments->content);

	MU_ASSERT_STR("right cmd", "cmd2", right->data.cmd->argv[0]);
	MU_ASSERT_STR("right assign", "B=2",
			right->data.cmd->assignments->content);

	ast_free(ast);
}

static void test_invalid_assignment_digit(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("1A=foo cmd"), &shell);

	MU_ASSERT("ast NULL or treated as arg",
			!ast || ast->data.cmd->assignments == NULL);

	ast_free(ast);
}

static void test_empty_value_assignment(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("VAR= cmd"), &shell);

	MU_ASSERT_INT(NODE_COMMAND, ast->type);

	MU_ASSERT_STR("cmd", "cmd", ast->data.cmd->argv[0]);
	MU_ASSERT_STR("assignment", "VAR=",
			ast->data.cmd->assignments->content);

	ast_free(ast);
}

static void test_plus_equals_not_assignment(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("VAR+=value cmd"), &shell);

	MU_ASSERT_INT(NODE_COMMAND, ast->type);

	MU_ASSERT_STR("cmd", "VAR+=value", ast->data.cmd->argv[0]);
	MU_ASSERT("no assignments", ast->data.cmd->assignments == NULL);

	ast_free(ast);
}

static void test_assignment_before_subshell(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("A=1 (echo hi)"), &shell);

	MU_ASSERT_INT(NODE_SUBSHELL, ast->type);
	MU_ASSERT("assignment applied to group?",
			ast->data.group->redirs == NULL);

	ast_free(ast);
}

/*
 * Helpers used by the assignment tests below.  They keep the assertions
 * NULL-safe: every deref is preceded by a guard so a failing test reports
 * the missing field instead of segfaulting and aborting the whole suite.
 */
static int	assert_command_node(t_ast *ast)
{
	MU_ASSERT("ast not NULL", ast != NULL);
	if (!ast)
		return (0);
	MU_ASSERT_INT(NODE_COMMAND, ast->type);
	if (ast->type != NODE_COMMAND)
		return (0);
	MU_ASSERT("cmd not NULL", ast->data.cmd != NULL);
	if (!ast->data.cmd)
		return (0);
	return (1);
}

static int	assert_argv_at(t_ast *ast, int i, const char *expected, const char *func_name)
{
	char	label[64];

	if (!ast->data.cmd->argv)
	{
		MU_ASSERT("argv not NULL", 0);
		return (0);
	}
	for (int k = 0; k <= i; k++)
	{
		if (!ast->data.cmd->argv[k])
		{
			snprintf(label, sizeof(label), "\033[1;36m[%s]\033[0m argv[%d] not NULL", func_name, k);
			MU_ASSERT(label, 0);
			return (0);
		}
	}
	snprintf(label, sizeof(label), "\033[1;36m[%s]\033[0m argv[%d]", func_name, i);
	MU_ASSERT_STR(label, expected, ast->data.cmd->argv[i]);
	return (1);
}

static int	assert_assignment_at(t_list *a, int i, const char *expected, const char *func_name)
{
	char	label[64];

	for (int k = 0; k < i; k++)
	{
		if (!a)
		{
			snprintf(label, sizeof(label), "\033[1;36m[%s]\033[0m assignments[%d] not NULL", func_name, k);
			MU_ASSERT(label, 0);
			return (0);
		}
		a = a->next;
	}
	if (!a)
	{
		snprintf(label, sizeof(label), "\033[1;36m[%s]\033[0m assignments[%d] not NULL", func_name, i);
		MU_ASSERT(label, 0);
		return (0);
	}
	snprintf(label, sizeof(label), "\033[1;36m[%s]\033[0m assignments[%d]", func_name, i);
	MU_ASSERT_STR(label, expected, (char *)a->content);
	return (1);
}

static void test_assignment_underscore_name(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("_VAR=value cmd"), &shell);

	if (!assert_command_node(ast))
		return ((void)ast_free(ast));
	assert_argv_at(ast, 0, "cmd", "test_assignment_underscore_name");
	assert_assignment_at(ast->data.cmd->assignments, 0, "_VAR=value", "test_assignment_underscore_name");
	ast_free(ast);
}

static void test_assignment_underscore_inside_name(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("MY_VAR=val cmd"), &shell);

	if (!assert_command_node(ast))
		return ((void)ast_free(ast));
	assert_argv_at(ast, 0, "cmd", "test_assignment_underscore_inside_name");
	assert_assignment_at(ast->data.cmd->assignments, 0, "MY_VAR=val", "test_assignment_underscore_inside_name");
	ast_free(ast);
}

static void test_assignment_alnum_value(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("VAR1=val123 cmd"), &shell);

	if (!assert_command_node(ast))
		return ((void)ast_free(ast));
	assert_argv_at(ast, 0, "cmd", "test_assignment_alnum_value");
	assert_assignment_at(ast->data.cmd->assignments, 0, "VAR1=val123", "test_assignment_alnum_value");
	ast_free(ast);
}

static void test_assignment_value_with_extra_equals(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("VAR=a=b=c cmd"), &shell);

	if (!assert_command_node(ast))
		return ((void)ast_free(ast));
	assert_argv_at(ast, 0, "cmd", "test_assignment_value_with_extra_equals");
	assert_assignment_at(ast->data.cmd->assignments, 0, "VAR=a=b=c", "test_assignment_value_with_extra_equals");
	ast_free(ast);
}

static void test_assignment_double_quoted(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("VAR=\"value\" cmd"), &shell);

	if (!assert_command_node(ast))
		return ((void)ast_free(ast));
	assert_argv_at(ast, 0, "cmd", "test_assignment_double_quoted");
	assert_assignment_at(ast->data.cmd->assignments, 0, "VAR=\"value\"", "test_assignment_double_quoted");
	ast_free(ast);
}

static void test_assignment_single_quoted(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("VAR='value' cmd"), &shell);

	if (!assert_command_node(ast))
		return ((void)ast_free(ast));
	assert_argv_at(ast, 0, "cmd", "test_assignment_single_quoted");
	assert_assignment_at(ast->data.cmd->assignments, 0, "VAR='value'", "test_assignment_single_quoted");
	ast_free(ast);
}

static void test_assignment_double_quoted_with_space(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(
			lexer_tokenize("VAR=\"hello world\" cmd"), &shell);

	if (!assert_command_node(ast))
		return ((void)ast_free(ast));
	assert_argv_at(ast, 0, "cmd", "test_assignment_double_quoted_with_space");
	assert_assignment_at(ast->data.cmd->assignments, 0,
			"VAR=\"hello world\"", "test_assignment_double_quoted_with_space");
	ast_free(ast);
}

static void test_assignment_double_quoted_empty(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("VAR=\"\" cmd"), &shell);

	if (!assert_command_node(ast))
		return ((void)ast_free(ast));
	assert_argv_at(ast, 0, "cmd",  "test_assignment_double_quoted_empty");
	assert_assignment_at(ast->data.cmd->assignments, 0, "VAR=\"\"", "test_assignment_double_quoted_empty");
	ast_free(ast);
}

static void test_assignment_double_quoted_with_hash_and_equals(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(
			lexer_tokenize("HELLO=\"#WORLD=\" cmd"), &shell);

	if (!assert_command_node(ast))
		return ((void)ast_free(ast));
	assert_argv_at(ast, 0, "cmd", "test_assignment_double_quoted_with_hash_and_equals");
	assert_assignment_at(ast->data.cmd->assignments, 0,
			"HELLO=\"#WORLD=\"", "test_assignment_double_quoted_with_hash_and_equals");
	ast_free(ast);
}

static void test_assignment_dollar_value(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("VAR=$HOME cmd"), &shell);

	if (!assert_command_node(ast))
		return ((void)ast_free(ast));
	assert_argv_at(ast, 0, "cmd", "test_assignment_dollar_value");
	assert_assignment_at(ast->data.cmd->assignments, 0, "VAR=$HOME", "test_assignment_dollar_value");
	ast_free(ast);
}

static void test_assignment_double_quoted_dollar(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(
			lexer_tokenize("VAR=\"$HOME/bin\" cmd"), &shell);

	if (!assert_command_node(ast))
		return ((void)ast_free(ast));
	assert_argv_at(ast, 0, "cmd", "test_assignment_double_quoted_dollar");
	assert_assignment_at(ast->data.cmd->assignments, 0,
			"VAR=\"$HOME/bin\"", "test_assignment_double_quoted_dollar");
	ast_free(ast);
}

static void test_assignment_mixed_quoted_and_bare(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(
			lexer_tokenize("A=1 B=\"2 3\" C='x' cmd"), &shell);

	if (!assert_command_node(ast))
		return ((void)ast_free(ast));
	assert_argv_at(ast, 0, "cmd", "test_assignment_mixed_quoted_and_bare");
	assert_assignment_at(ast->data.cmd->assignments, 0, "A=1", "test_assignment_mixed_quoted_and_bare");
	assert_assignment_at(ast->data.cmd->assignments, 1, "B=\"2 3\"", "test_assignment_mixed_quoted_and_bare");
	assert_assignment_at(ast->data.cmd->assignments, 2, "C='x'", "test_assignment_mixed_quoted_and_bare");
	ast_free(ast);
}

static void test_assignment_many_bare_then_args(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(
			lexer_tokenize("K1=v1 K2=v2 K3=v3 cmd arg1 arg2"), &shell);

	if (!assert_command_node(ast))
		return ((void)ast_free(ast));
	assert_argv_at(ast, 0, "cmd", "test_assignment_many_bare_then_args");
	assert_argv_at(ast, 1, "arg1", "test_assignment_many_bare_then_args");
	assert_argv_at(ast, 2, "arg2", "test_assignment_many_bare_then_args");
	assert_assignment_at(ast->data.cmd->assignments, 0, "K1=v1", "test_assignment_many_bare_then_args");
	assert_assignment_at(ast->data.cmd->assignments, 1, "K2=v2", "test_assignment_many_bare_then_args");
	assert_assignment_at(ast->data.cmd->assignments, 2, "K3=v3", "test_assignment_many_bare_then_args");

	t_list *a = ast->data.cmd->assignments;
	int n = 0;
	while (a)
	{
		n++;
		a = a->next;
	}
	MU_ASSERT_INT(3, n);
	ast_free(ast);
}

static void test_assignment_after_argv_is_arg(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("A=1 cmd B=2 C=3"), &shell);

	if (!assert_command_node(ast))
		return ((void)ast_free(ast));
	assert_argv_at(ast, 0, "cmd", "test_assignment_after_argv_is_arg");
	assert_argv_at(ast, 1, "B=2", "test_assignment_after_argv_is_arg");
	assert_argv_at(ast, 2, "C=3", "test_assignment_after_argv_is_arg");
	assert_assignment_at(ast->data.cmd->assignments, 0, "A=1",  "test_assignment_after_argv_is_arg");

	if (ast->data.cmd->assignments)
		MU_ASSERT("only one assignment",
				ast->data.cmd->assignments->next == NULL);
	ast_free(ast);
}

static void test_assignment_only_empty_value(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("VAR="), &shell);

	if (!assert_command_node(ast))
		return ((void)ast_free(ast));
	MU_ASSERT("no argv or empty", ast->data.cmd->argv == NULL
			|| ast->data.cmd->argv[0] == NULL);
	assert_assignment_at(ast->data.cmd->assignments, 0, "VAR=", "test_assignment_only_empty_value");
	ast_free(ast);
}

static void test_assignment_only_multiple(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("A=1 B=2 C=3"), &shell);

	if (!assert_command_node(ast))
		return ((void)ast_free(ast));
	MU_ASSERT("no argv or empty", ast->data.cmd->argv == NULL
			|| ast->data.cmd->argv[0] == NULL);
	assert_assignment_at(ast->data.cmd->assignments, 0, "A=1", "test_assignment_only_multiple");
	assert_assignment_at(ast->data.cmd->assignments, 1, "B=2", "test_assignment_only_multiple");
	assert_assignment_at(ast->data.cmd->assignments, 2, "C=3", "test_assignment_only_multiple");
	ast_free(ast);
}

static void test_assignment_leading_equals_falls_to_argv(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("=value cmd"), &shell);

	if (!assert_command_node(ast))
		return ((void)ast_free(ast));
	MU_ASSERT("no assignments", ast->data.cmd->assignments == NULL);
	assert_argv_at(ast, 0, "=value", "test_assignment_leading_equals_falls_to_argv");
	assert_argv_at(ast, 1, "cmd", "test_assignment_leading_equals_falls_to_argv");
	ast_free(ast);
}

static void test_assignment_dash_in_name_falls_to_argv(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(lexer_tokenize("MY-VAR=val cmd"), &shell);

	if (!assert_command_node(ast))
		return ((void)ast_free(ast));
	MU_ASSERT("no assignments", ast->data.cmd->assignments == NULL);
	assert_argv_at(ast, 0, "MY-VAR=val", "test_assignment_dash_in_name_falls_to_argv");
	assert_argv_at(ast, 1, "cmd", "test_assignment_dash_in_name_falls_to_argv");
	ast_free(ast);
}

static void test_assignment_pipeline_quoted_both_sides(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(
			lexer_tokenize("A=\"1 2\" cmd1 | B='x y' cmd2 arg"), &shell);

	MU_ASSERT("ast not NULL", ast != NULL);
	if (!ast)
		return;
	MU_ASSERT_INT(NODE_PIPE, ast->type);
	if (ast->type != NODE_PIPE)
		return ((void)ast_free(ast));
	if (!ast->data.binary || !ast->data.binary->left
		|| !ast->data.binary->right)
	{
		MU_ASSERT("pipe children not NULL", 0);
		ast_free(ast);
		return ;
	}

	t_ast *left = ast->data.binary->left;
	t_ast *right = ast->data.binary->right;

	if (assert_command_node(left))
	{
		assert_argv_at(left, 0, "cmd1", "test_assignment_pipeline_quoted_both_sides");
		assert_assignment_at(left->data.cmd->assignments, 0, "A=\"1 2\"", "test_assignment_pipeline_quoted_both_sides");
	}
	if (assert_command_node(right))
	{
		assert_argv_at(right, 0, "cmd2", "test_assignment_pipeline_quoted_both_sides");
		assert_argv_at(right, 1, "arg", "test_assignment_pipeline_quoted_both_sides");
		assert_assignment_at(right->data.cmd->assignments, 0, "B='x y'", "test_assignment_pipeline_quoted_both_sides");
	}
	ast_free(ast);
}

static void test_assignment_with_two_redirs(void)
{
	t_shell	shell;
	t_ast	*ast = parser_parse(
			lexer_tokenize("A=1 B=\"2 3\" cmd < in > out"), &shell);

	if (!assert_command_node(ast))
		return ((void)ast_free(ast));
	assert_argv_at(ast, 0, "cmd",  "test_assignment_with_two_redirs");
	assert_assignment_at(ast->data.cmd->assignments, 0, "A=1", "test_assignment_with_two_redirs");
	assert_assignment_at(ast->data.cmd->assignments, 1, "B=\"2 3\"", "test_assignment_with_two_redirs");

	t_list *r = ast->data.cmd->redirs;
	if (!r)
	{
		MU_ASSERT("redirs not NULL", 0);
		ast_free(ast);
		return ;
	}
	t_redir *r0 = (t_redir *)r->content;
	if (r0)
	{
		MU_ASSERT_INT(TOK_REDIR_IN, r0->type);
		MU_ASSERT_STR("in target", "in", r0->target);
	}
	if (!r->next)
	{
		MU_ASSERT("redirs[1] not NULL", 0);
		ast_free(ast);
		return ;
	}
	t_redir *r1 = (t_redir *)r->next->content;
	if (r1)
	{
		MU_ASSERT_INT(TOK_REDIR_OUT, r1->type);
		MU_ASSERT_STR("out target", "out", r1->target);
	}
	ast_free(ast);
}

static t_ast	*write_to_pipe_nontty(const char *stdin_payload, char *cmd)
{
	t_shell	shell;
	int		pipefd[2];
	t_ast	*ast;

	pipe(pipefd);
	write(pipefd[1], stdin_payload, strlen(stdin_payload));
	close(pipefd[1]);

	int saved_stdin = dup(STDIN_FILENO);
	dup2(pipefd[0], STDIN_FILENO);
	close(pipefd[0]);

	ast = parser_parse(lexer_tokenize(cmd), &shell);

	dup2(saved_stdin, STDIN_FILENO);
	close(saved_stdin);
	return (ast);
}

static void	test_heredoc_nontty_basic(void)
{
	t_ast	*ast;
	t_redir	*redir;
	char	buf[BUFSIZE];
	ssize_t	n;

	ast = write_to_pipe_nontty("hello\nEOF\n", "cat << EOF");
	MU_ASSERT("ast not NULL", ast != NULL);
	MU_ASSERT_INT(NODE_COMMAND, ast->type);
	MU_ASSERT("has redirs", ast->data.cmd->redirs != NULL);

	redir = REDIR(ast->data.cmd->redirs);
	MU_ASSERT_INT(TOK_HEREDOC, redir->type);
	MU_ASSERT("heredoc_fd >= 0", redir->heredoc_fd >= 0);

	n = read(redir->heredoc_fd, buf, BUFSIZE - 1);
	buf[n] = '\0';
	MU_ASSERT_STR("content", "hello\n", buf);

	close(redir->heredoc_fd);
	ast_free(ast);
}

static void	test_heredoc_nontty_multiline(void)
{
	t_ast	*ast;
	t_redir	*redir;
	char	buf[BUFSIZE];
	ssize_t	n;

	ast = write_to_pipe_nontty("line1\nline2\nline3\nEOF\n", "cat << EOF");
	MU_ASSERT("ast not NULL", ast != NULL);
	redir = REDIR(ast->data.cmd->redirs);
	MU_ASSERT("heredoc_fd >= 0", redir->heredoc_fd >= 0);

	n = read(redir->heredoc_fd, buf, BUFSIZE - 1);
	buf[n] = '\0';
	MU_ASSERT_STR("content", "line1\nline2\nline3\n", buf);

	close(redir->heredoc_fd);
	ast_free(ast);
}

static void	test_heredoc_nontty_empty_body(void)
{
	t_ast	*ast;
	t_redir	*redir;
	ssize_t	n;
	char	buf[BUFSIZE];

	ast = write_to_pipe_nontty("EOF\n", "cat << EOF");
	MU_ASSERT("ast not NULL", ast != NULL);
	redir = REDIR(ast->data.cmd->redirs);
	MU_ASSERT("heredoc_fd >= 0", redir->heredoc_fd >= 0);

	n = read(redir->heredoc_fd, buf, BUFSIZE - 1);
	MU_ASSERT_INT(0, (int)n); /* pipe must be empty */

	close(redir->heredoc_fd);
	ast_free(ast);
}

static void	test_heredoc_nontty_eof_before_delim(void)
{
	t_ast	*ast;
	t_redir	*redir;
	char	buf[BUFSIZE];
	ssize_t	n;

	ast = write_to_pipe_nontty("hello\n", "cat << EOF");
	MU_ASSERT("ast not NULL", ast != NULL);
	redir = REDIR(ast->data.cmd->redirs);
	MU_ASSERT("heredoc_fd >= 0", redir->heredoc_fd >= 0);

	n = read(redir->heredoc_fd, buf, BUFSIZE - 1);
	buf[n] = '\0';
	MU_ASSERT_STR("content before EOF", "hello\n", buf);

	close(redir->heredoc_fd);
	ast_free(ast);
}

static void	test_heredoc_nontty_strip_tabs(void)
{
	t_ast	*ast;
	t_redir	*redir;
	char	buf[BUFSIZE];
	ssize_t	n;

	ast = write_to_pipe_nontty("\thello\n\tworld\n\tEOF\n", "cat <<- EOF");
	MU_ASSERT("ast not NULL", ast != NULL);
	redir = REDIR(ast->data.cmd->redirs);
	MU_ASSERT_INT(TOK_HEREDOC_STRIP, redir->type);
	MU_ASSERT("heredoc_fd >= 0", redir->heredoc_fd >= 0);

	n = read(redir->heredoc_fd, buf, BUFSIZE - 1);
	buf[n] = '\0';
	MU_ASSERT_STR("tabs stripped", "hello\nworld\n", buf);

	close(redir->heredoc_fd);
	ast_free(ast);
}

static void	test_heredoc_nontty_strip_multiple_tabs(void)
{
	t_ast	*ast;
	t_redir	*redir;
	char	buf[BUFSIZE];
	ssize_t	n;

	ast = write_to_pipe_nontty("\t\t\tindented\n\tEOF\n", "cat <<- EOF");
	MU_ASSERT("ast not NULL", ast != NULL);
	redir = REDIR(ast->data.cmd->redirs);
	MU_ASSERT_INT(TOK_HEREDOC_STRIP, redir->type);
	MU_ASSERT("heredoc_fd >= 0", redir->heredoc_fd >= 0);

	n = read(redir->heredoc_fd, buf, BUFSIZE - 1);
	buf[n] = '\0';
	MU_ASSERT_STR("all tabs stripped", "indented\n", buf);

	close(redir->heredoc_fd);
	ast_free(ast);
}

static void	test_heredoc_nontty_quoted_delim_no_expand(void)
{
	t_ast	*ast;
	t_redir	*redir;
	char	buf[BUFSIZE];
	ssize_t	n;

	ast = write_to_pipe_nontty("$USER\nEOF\n", "cat << 'EOF'");
	MU_ASSERT("ast not NULL", ast != NULL);
	redir = REDIR(ast->data.cmd->redirs);
	MU_ASSERT_INT(TOK_HEREDOC, redir->type);
	MU_ASSERT("quoted flag set", redir->heredoc_quoted == 1);
	MU_ASSERT("heredoc_fd >= 0", redir->heredoc_fd >= 0);

	n = read(redir->heredoc_fd, buf, BUFSIZE - 1);
	buf[n] = '\0';
	MU_ASSERT_STR("literal dollar", "$USER\n", buf);

	close(redir->heredoc_fd);
	ast_free(ast);
}

static void	test_heredoc_nontty_dquoted_delim_no_expand(void)
{
	t_ast	*ast;
	t_redir	*redir;
	char	buf[BUFSIZE];
	ssize_t	n;

	ast = write_to_pipe_nontty("$HOME\nEOF\n", "cat << \"EOF\"");
	MU_ASSERT("ast not NULL", ast != NULL);
	redir = REDIR(ast->data.cmd->redirs);
	MU_ASSERT_INT(TOK_HEREDOC, redir->type);
	MU_ASSERT("quoted flag set", redir->heredoc_quoted == 1);
	MU_ASSERT("heredoc_fd >= 0", redir->heredoc_fd >= 0);

	n = read(redir->heredoc_fd, buf, BUFSIZE - 1);
	buf[n] = '\0';
	MU_ASSERT_STR("literal dollar", "$HOME\n", buf);

	close(redir->heredoc_fd);
	ast_free(ast);
}

static void	test_heredoc_nontty_in_pipeline(void)
{
	t_ast	*ast;
	t_redir	*redir;
	char	buf[BUFSIZE];
	ssize_t	n;

	ast = write_to_pipe_nontty("hello\nEOF\n", "cat << EOF | grep hello");
	MU_ASSERT("ast not NULL", ast != NULL);
	MU_ASSERT_INT(NODE_PIPE, ast->type);

	redir = REDIR(ast->data.binary->left->data.cmd->redirs);
	MU_ASSERT_INT(TOK_HEREDOC, redir->type);
	MU_ASSERT("heredoc_fd >= 0", redir->heredoc_fd >= 0);

	n = read(redir->heredoc_fd, buf, BUFSIZE - 1);
	buf[n] = '\0';
	MU_ASSERT_STR("content", "hello\n", buf);
	MU_ASSERT_STR("right cmd", "grep",
			ast->data.binary->right->data.cmd->argv[0]);

	close(redir->heredoc_fd);
	ast_free(ast);
}

static void	test_heredoc_nontty_two_heredocs(void)
{
	t_ast	*ast;
	t_list	*redirs;
	t_redir	*r0;
	t_redir	*r1;
	char	buf[BUFSIZE];
	ssize_t	n;

	ast = write_to_pipe_nontty("aaa\nEOF1\nbbb\nEOF2\n",
			"cmd << EOF1 << EOF2");
	MU_ASSERT("ast not NULL", ast != NULL);
	MU_ASSERT_INT(NODE_COMMAND, ast->type);

	redirs = ast->data.cmd->redirs;
	MU_ASSERT("has two redirs", redirs != NULL && redirs->next != NULL);

	r0 = (t_redir *)redirs->content;
	r1 = (t_redir *)redirs->next->content;

	MU_ASSERT_INT(TOK_HEREDOC, r0->type);
	MU_ASSERT_INT(TOK_HEREDOC, r1->type);
	MU_ASSERT("fd0 >= 0", r0->heredoc_fd >= 0);
	MU_ASSERT("fd1 >= 0", r1->heredoc_fd >= 0);

	n = read(r0->heredoc_fd, buf, BUFSIZE - 1);
	buf[n] = '\0';
	MU_ASSERT_STR("first heredoc", "aaa\n", buf);

	n = read(r1->heredoc_fd, buf, BUFSIZE - 1);
	buf[n] = '\0';
	MU_ASSERT_STR("second heredoc", "bbb\n", buf);

	close(r0->heredoc_fd);
	close(r1->heredoc_fd);
	ast_free(ast);
}

static void	test_heredoc_nontty_with_output_redir(void)
{
	t_ast	*ast;
	t_list	*redirs;
	t_redir	*hd;
	t_redir	*out;

	ast = write_to_pipe_nontty("hello\nEOF\n", "cat << EOF > /dev/null");
	MU_ASSERT("ast not NULL", ast != NULL);
	MU_ASSERT_INT(NODE_COMMAND, ast->type);

	redirs = ast->data.cmd->redirs;
	MU_ASSERT("has two redirs", redirs != NULL && redirs->next != NULL);

	hd  = (t_redir *)redirs->content;
	out = (t_redir *)redirs->next->content;

	MU_ASSERT_INT(TOK_HEREDOC, hd->type);
	MU_ASSERT("heredoc_fd >= 0", hd->heredoc_fd >= 0);
	MU_ASSERT_INT(TOK_REDIR_OUT, out->type);
	MU_ASSERT_STR("out target", "/dev/null", out->target);

	close(hd->heredoc_fd);
	ast_free(ast);
}

static void	test_heredoc_nontty_subshell(void)
{
	t_ast	*ast;
	t_redir	*redir;
	char	buf[BUFSIZE];
	ssize_t	n;

	ast = write_to_pipe_nontty("hello\nEOF\n", "(cat) << EOF");
	MU_ASSERT("ast not NULL", ast != NULL);
	MU_ASSERT_INT(NODE_SUBSHELL, ast->type);

	redir = REDIR(ast->data.group->redirs);
	MU_ASSERT_INT(TOK_HEREDOC, redir->type);
	MU_ASSERT("heredoc_fd >= 0", redir->heredoc_fd >= 0);

	n = read(redir->heredoc_fd, buf, BUFSIZE - 1);
	buf[n] = '\0';
	MU_ASSERT_STR("content", "hello\n", buf);

	close(redir->heredoc_fd);
	ast_free(ast);
}

static void	test_heredoc_nontty_special_delim(void)
{
	t_ast	*ast;
	t_redir	*redir;
	char	buf[BUFSIZE];
	ssize_t	n;

	ast = write_to_pipe_nontty("data\nSTOP!\n", "cat << STOP!");
	MU_ASSERT("ast not NULL", ast != NULL);
	redir = REDIR(ast->data.cmd->redirs);
	MU_ASSERT("heredoc_fd >= 0", redir->heredoc_fd >= 0);

	n = read(redir->heredoc_fd, buf, BUFSIZE - 1);
	buf[n] = '\0';
	MU_ASSERT_STR("content before special delim", "data\n", buf);

	close(redir->heredoc_fd);
	ast_free(ast);
}

static void	test_heredoc_nontty_partial_delim_not_matched(void)
{
	t_ast	*ast;
	t_redir	*redir;
	char	buf[BUFSIZE];
	ssize_t	n;

	ast = write_to_pipe_nontty("EOFX\nEOF\n", "cat << EOF");
	MU_ASSERT("ast not NULL", ast != NULL);
	redir = REDIR(ast->data.cmd->redirs);
	MU_ASSERT("heredoc_fd >= 0", redir->heredoc_fd >= 0);

	n = read(redir->heredoc_fd, buf, BUFSIZE - 1);
	buf[n] = '\0';
	/* "EOFX" must be kept; it is not the delimiter */
	MU_ASSERT_STR("partial delim kept", "EOFX\n", buf);

	close(redir->heredoc_fd);
	ast_free(ast);
}

static void	test_heredoc_nontty_blank_lines_preserved(void)
{
	t_ast	*ast;
	t_redir	*redir;
	char	buf[BUFSIZE];
	ssize_t	n;

	ast = write_to_pipe_nontty("\n\nhello\n\nEOF\n", "cat << EOF");
	MU_ASSERT("ast not NULL", ast != NULL);
	redir = REDIR(ast->data.cmd->redirs);
	MU_ASSERT("heredoc_fd >= 0", redir->heredoc_fd >= 0);

	n = read(redir->heredoc_fd, buf, BUFSIZE - 1);
	buf[n] = '\0';
	MU_ASSERT_STR("blank lines kept", "\n\nhello\n\n", buf);

	close(redir->heredoc_fd);
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
	test_multiple_redirs();
	test_background_separator();
	test_multiple_background_chain();
	test_trailing_background();
	test_pipe_newline();
	test_and_newline();
	test_only_pipe();
	test_only_and();
	test_only_or_and();
	test_unclose_parenthesis();
	test_unopen_parenthesis();
	test_redirs_in_a_row();
	test_heredoc_basic();
	test_heredoc_stripped_basic();
	test_heredoc_multiline();
	test_heredoc_stripped_multiline();
	test_heredoc_quoted_no_expand();
	test_heredoc_stripped_quoted_no_expand();
	test_heredoc_pipe();
	test_heredoc_stripped_pipe();
	test_heredoc_with_redir();
	test_heredoc_stripped_with_redir();
	test_heredoc_unterminated();
	test_heredoc_stripped_unterminated();
	test_heredoc_group();
	test_heredoc_stripped_group();
	test_heredoc_empty();
	test_heredoc_stripped_empty();
	test_no_heredoc();
	test_assignment();
	test_multiple_assignments();
	test_assignment_only();
	test_assignment_after_command();
	test_assignment_and_args();
	test_assignment_with_redir();
	test_assignment_in_pipeline();
	test_invalid_assignment_digit();
	test_empty_value_assignment();
	test_plus_equals_not_assignment();
	test_assignment_before_subshell();
	test_assignment_underscore_name();
	test_assignment_underscore_inside_name();
	test_assignment_alnum_value();
	test_assignment_value_with_extra_equals();
	test_assignment_double_quoted();
	test_assignment_single_quoted();
	test_assignment_double_quoted_with_space();
	test_assignment_double_quoted_empty();
	test_assignment_double_quoted_with_hash_and_equals();
	test_assignment_dollar_value();
	test_assignment_double_quoted_dollar();
	test_assignment_mixed_quoted_and_bare();
	test_assignment_many_bare_then_args();
	test_assignment_after_argv_is_arg();
	test_assignment_only_empty_value();
	test_assignment_only_multiple();
	test_assignment_leading_equals_falls_to_argv();
	test_assignment_dash_in_name_falls_to_argv();
	test_assignment_pipeline_quoted_both_sides();
	test_assignment_with_two_redirs();
	test_heredoc_nontty_basic();
	test_heredoc_nontty_multiline();
	test_heredoc_nontty_empty_body();
	test_heredoc_nontty_eof_before_delim();
	test_heredoc_nontty_strip_tabs();
	test_heredoc_nontty_strip_multiple_tabs();
	test_heredoc_nontty_quoted_delim_no_expand();
	test_heredoc_nontty_dquoted_delim_no_expand();
	test_heredoc_nontty_in_pipeline();
	test_heredoc_nontty_two_heredocs();
	test_heredoc_nontty_with_output_redir();
	test_heredoc_nontty_subshell();
	test_heredoc_nontty_special_delim();
	test_heredoc_nontty_partial_delim_not_matched();
	test_heredoc_nontty_blank_lines_preserved();
}
