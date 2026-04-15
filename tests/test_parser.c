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
}
