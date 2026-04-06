/**
 * @file test_executor.c
 * @brief Comprehensive test suite for the executor module.
 *
 * Tests: exec_utils, command_search, redirections, heredoc,
 *        logical operators, pipeline, subshell, block, background,
 *        simple command execution (builtins + externals).
 *
 * Uses functional stubs from test_stubs.c for dependencies.
 */

#ifdef TEST_EXECUTOR_ENABLED

#include "minunit.h"
#include "../includes/42sh.h"
#include "../includes/executor.h"
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>

/* Stub helpers declared in test_stubs.c */
extern void	stub_shell_init(t_shell *shell);
extern void	stub_shell_cleanup(t_shell *shell);
extern void	stub_set_builtin(const char *name, t_builtin_fn fn);

/* ================================================================
 * 1. exec_utils: get_exit_status, split_assignment
 * ================================================================ */

static int	fork_exit_and_get(int code)
{
	pid_t	pid;
	int		wstatus;

	fflush(stdout);
	fflush(stderr);
	pid = fork();
	if (pid == 0)
		_exit(code);
	waitpid(pid, &wstatus, 0);
	return (get_exit_status(wstatus));
}

static int	fork_signal_and_get(int sig)
{
	pid_t	pid;
	int		wstatus;

	fflush(stdout);
	fflush(stderr);
	pid = fork();
	if (pid == 0)
	{
		pause();
		_exit(1);
	}
	kill(pid, sig);
	waitpid(pid, &wstatus, 0);
	return (get_exit_status(wstatus));
}

static void	test_get_exit_status(void)
{
	MU_ASSERT_INT(0, fork_exit_and_get(0));
	MU_ASSERT_INT(42, fork_exit_and_get(42));
	MU_ASSERT_INT(127, fork_exit_and_get(127));
	MU_ASSERT_INT(126, fork_exit_and_get(126));
	MU_ASSERT_INT(255, fork_exit_and_get(255));
	MU_ASSERT_INT(128 + SIGKILL, fork_signal_and_get(SIGKILL));
	MU_ASSERT_INT(128 + SIGTERM, fork_signal_and_get(SIGTERM));
}

static void	test_split_assignment(void)
{
	char	*name;
	char	*value;

	/* Normal case */
	split_assignment("FOO=bar", &name, &value);
	MU_ASSERT_STR("split name", "FOO", name);
	MU_ASSERT_STR("split value", "bar", value);
	free(name);
	free(value);

	/* Empty value */
	split_assignment("EMPTY=", &name, &value);
	MU_ASSERT_STR("split empty name", "EMPTY", name);
	MU_ASSERT_STR("split empty value", "", value);
	free(name);
	free(value);

	/* Value with = in it */
	split_assignment("KEY=a=b=c", &name, &value);
	MU_ASSERT_STR("split key with =", "KEY", name);
	MU_ASSERT_STR("split value with =", "a=b=c", value);
	free(name);
	free(value);

	/* No = sign */
	split_assignment("NOEQUAL", &name, &value);
	MU_ASSERT_STR("no = name", "NOEQUAL", name);
	MU_ASSERT_STR("no = value", "", value);
	free(name);
	free(value);

	/* Long variable */
	split_assignment("LONG_VAR_NAME=some long value with spaces", &name, &value);
	MU_ASSERT_STR("long name", "LONG_VAR_NAME", name);
	MU_ASSERT_STR("long value", "some long value with spaces", value);
	free(name);
	free(value);
}

/* ================================================================
 * 2. command_search: find_command
 * TODO: uncomment when var_set/var_get_value are implemented
 * ================================================================ */
#if 0

static void	test_find_command(void)
{
	t_shell	shell;
	char	*path;

	stub_shell_init(&shell);
	var_set(&shell, "PATH", "/usr/bin:/bin:/usr/sbin");

	/* Find common commands */
	path = find_command(&shell, "ls");
	MU_ASSERT("find ls", path != NULL);
	MU_ASSERT("ls is executable", access(path, X_OK) == 0);
	free(path);

	path = find_command(&shell, "echo");
	MU_ASSERT("find echo", path != NULL);
	free(path);

	path = find_command(&shell, "cat");
	MU_ASSERT("find cat", path != NULL);
	free(path);

	/* Non-existent command */
	path = find_command(&shell, "zzz_nonexistent_cmd_42sh");
	MU_ASSERT("nonexistent returns NULL", path == NULL);

	/* Empty name */
	path = find_command(&shell, "");
	MU_ASSERT("empty name returns NULL", path == NULL);

	/* NULL name */
	path = find_command(&shell, NULL);
	MU_ASSERT("NULL name returns NULL", path == NULL);

	/* Absolute path */
	path = find_command(&shell, "/bin/ls");
	MU_ASSERT("absolute path found", path != NULL);
	MU_ASSERT_STR("absolute path value", "/bin/ls", path);
	free(path);

	/* Absolute path that doesn't exist */
	path = find_command(&shell, "/nonexistent/path/foo");
	MU_ASSERT("bad absolute path returns NULL", path == NULL);

	/* Relative path with / */
	path = find_command(&shell, "./nonexistent");
	MU_ASSERT("bad relative path returns NULL", path == NULL);

	/* No PATH set */
	var_unset(&shell, "PATH");
	path = find_command(&shell, "ls");
	MU_ASSERT("no PATH returns NULL", path == NULL);

	stub_shell_cleanup(&shell);
}
#endif

/* ================================================================
 * 3. Heredoc
 * ================================================================ */

static void	test_heredoc(void)
{
	t_redir	redir;
	int		fd;
	char	buf[256];
	ssize_t	n;

	/* Normal heredoc */
	memset(&redir, 0, sizeof(redir));
	redir.type = TOK_HEREDOC;
	redir.fd = -1;
	redir.heredoc_content = strdup("hello world\nline 2\n");
	fd = setup_heredoc(&redir);
	MU_ASSERT("heredoc fd >= 0", fd >= 0);
	n = read(fd, buf, sizeof(buf) - 1);
	buf[n] = '\0';
	MU_ASSERT_STR("heredoc content", "hello world\nline 2\n", buf);
	close(fd);
	free(redir.heredoc_content);

	/* Empty heredoc */
	memset(&redir, 0, sizeof(redir));
	redir.type = TOK_HEREDOC;
	redir.fd = -1;
	redir.heredoc_content = strdup("");
	fd = setup_heredoc(&redir);
	MU_ASSERT("empty heredoc fd >= 0", fd >= 0);
	n = read(fd, buf, sizeof(buf) - 1);
	MU_ASSERT_INT(0, (int)n);
	close(fd);
	free(redir.heredoc_content);

	/* NULL content heredoc */
	memset(&redir, 0, sizeof(redir));
	redir.type = TOK_HEREDOC;
	redir.fd = -1;
	redir.heredoc_content = NULL;
	fd = setup_heredoc(&redir);
	MU_ASSERT("null heredoc fd >= 0", fd >= 0);
	n = read(fd, buf, sizeof(buf) - 1);
	MU_ASSERT_INT(0, (int)n);
	close(fd);
}

/* ================================================================
 * 4. Redirections
 * ================================================================ */

static void	test_redirections_output(void)
{
	t_redir	redir;
	t_list	*redirs;
	int		saved_fds[3];
	char	buf[256];
	int		fd;
	ssize_t	n;
	int		ret;

	/* Redirect stdout to a file */
	memset(&redir, 0, sizeof(redir));
	redir.type = TOK_REDIR_OUT;
	redir.fd = -1;
	redir.target = strdup("/tmp/42sh_test_redir_out");
	redirs = ft_lstnew(&redir);
	fflush(stdout);
	ret = setup_redirections(redirs, saved_fds);
	write(1, "test output\n", 12);
	restore_redirections(saved_fds);
	MU_ASSERT_INT(0, ret);

	/* Verify file contents */
	fd = open("/tmp/42sh_test_redir_out", O_RDONLY);
	MU_ASSERT("output file opened", fd >= 0);
	n = read(fd, buf, sizeof(buf) - 1);
	buf[n] = '\0';
	MU_ASSERT_STR("redirected output", "test output\n", buf);
	close(fd);
	unlink("/tmp/42sh_test_redir_out");
	free(redir.target);
	free(redirs);
}

static void	test_redirections_append(void)
{
	t_redir	redir;
	t_list	*redirs;
	int		saved_fds[3];
	char	buf[256];
	int		fd;
	ssize_t	n;
	int		ret;

	/* Create file with initial content */
	fd = open("/tmp/42sh_test_redir_append", O_WRONLY | O_CREAT | O_TRUNC, 0644);
	write(fd, "first\n", 6);
	close(fd);

	/* Append redirect */
	memset(&redir, 0, sizeof(redir));
	redir.type = TOK_REDIR_APPEND;
	redir.fd = -1;
	redir.target = strdup("/tmp/42sh_test_redir_append");
	redirs = ft_lstnew(&redir);
	fflush(stdout);
	ret = setup_redirections(redirs, saved_fds);
	write(1, "second\n", 7);
	restore_redirections(saved_fds);
	MU_ASSERT_INT(0, ret);

	/* Verify append */
	fd = open("/tmp/42sh_test_redir_append", O_RDONLY);
	n = read(fd, buf, sizeof(buf) - 1);
	buf[n] = '\0';
	MU_ASSERT_STR("appended output", "first\nsecond\n", buf);
	close(fd);
	unlink("/tmp/42sh_test_redir_append");
	free(redir.target);
	free(redirs);
}

static void	test_redirections_input(void)
{
	t_redir	redir;
	t_list	*redirs;
	int		saved_fds[3];
	char	buf[256];
	int		fd;
	ssize_t	n;

	/* Create input file */
	fd = open("/tmp/42sh_test_redir_in", O_WRONLY | O_CREAT | O_TRUNC, 0644);
	write(fd, "input data\n", 11);
	close(fd);

	/* Redirect stdin from file */
	memset(&redir, 0, sizeof(redir));
	redir.type = TOK_REDIR_IN;
	redir.fd = -1;
	redir.target = strdup("/tmp/42sh_test_redir_in");
	redirs = ft_lstnew(&redir);
	MU_ASSERT_INT(0, setup_redirections(redirs, saved_fds));
	n = read(0, buf, sizeof(buf) - 1);
	buf[n] = '\0';
	MU_ASSERT_STR("redirected input", "input data\n", buf);
	restore_redirections(saved_fds);

	unlink("/tmp/42sh_test_redir_in");
	free(redir.target);
	free(redirs);
}

static void	test_redirections_heredoc_in_list(void)
{
	t_redir	redir;
	t_list	*redirs;
	int		saved_fds[3];
	char	buf[256];
	ssize_t	n;

	/* Redirect stdin from heredoc via redirection list */
	memset(&redir, 0, sizeof(redir));
	redir.type = TOK_HEREDOC;
	redir.fd = -1;
	redir.heredoc_content = strdup("heredoc line\n");
	redirs = ft_lstnew(&redir);
	MU_ASSERT_INT(0, setup_redirections(redirs, saved_fds));
	n = read(0, buf, sizeof(buf) - 1);
	buf[n] = '\0';
	MU_ASSERT_STR("heredoc via redirs", "heredoc line\n", buf);
	restore_redirections(saved_fds);
	free(redir.heredoc_content);
	free(redirs);
}

static void	test_redirections_bad_file(void)
{
	t_redir	redir;
	t_list	*redirs;
	int		saved_fds[3];
	int		ret;

	/* Redirect to non-existent directory should fail */
	memset(&redir, 0, sizeof(redir));
	redir.type = TOK_REDIR_OUT;
	redir.fd = -1;
	redir.target = strdup("/nonexistent_dir_42sh/output");
	redirs = ft_lstnew(&redir);
	ret = setup_redirections(redirs, saved_fds);
	MU_ASSERT_INT(-1, ret);
	free(redir.target);
	free(redirs);
}

static void	test_redirections_save_restore(void)
{
	int	saved_fds[3];
	int	orig_stdin;
	int	orig_stdout;
	int	orig_stderr;

	/* Save original fds */
	orig_stdin = dup(0);
	orig_stdout = dup(1);
	orig_stderr = dup(2);

	/* Save and restore with no redirections */
	MU_ASSERT_INT(0, setup_redirections(NULL, saved_fds));
	restore_redirections(saved_fds);

	/* Verify fds are still valid */
	MU_ASSERT("stdin still valid", fcntl(0, F_GETFD) != -1);
	MU_ASSERT("stdout still valid", fcntl(1, F_GETFD) != -1);
	MU_ASSERT("stderr still valid", fcntl(2, F_GETFD) != -1);

	close(orig_stdin);
	close(orig_stdout);
	close(orig_stderr);
}

/* ================================================================
 * 5. Logical operators: &&, ||, ;
 * ================================================================ */

/* Helper: create a NODE_COMMAND AST that runs /bin/true or /bin/false */
static t_ast	*make_true_cmd(void)
{
	t_ast	*ast;

	ast = calloc(1, sizeof(t_ast));
	ast->type = NODE_COMMAND;
	ast->data.cmd = calloc(1, sizeof(t_cmd));
	ast->data.cmd->argv = calloc(2, sizeof(char *));
	ast->data.cmd->argv[0] = strdup("/usr/bin/true");
	ast->data.cmd->argc = 1;
	return (ast);
}

static t_ast	*make_false_cmd(void)
{
	t_ast	*ast;

	ast = calloc(1, sizeof(t_ast));
	ast->type = NODE_COMMAND;
	ast->data.cmd = calloc(1, sizeof(t_cmd));
	ast->data.cmd->argv = calloc(2, sizeof(char *));
	ast->data.cmd->argv[0] = strdup("/usr/bin/false");
	ast->data.cmd->argc = 1;
	return (ast);
}

static void	free_cmd_ast(t_ast *ast)
{
	if (!ast)
		return ;
	if (ast->type == NODE_COMMAND && ast->data.cmd)
	{
		if (ast->data.cmd->argv)
		{
			int i = 0;
			while (ast->data.cmd->argv[i])
				free(ast->data.cmd->argv[i++]);
			free(ast->data.cmd->argv);
		}
		free(ast->data.cmd);
	}
	free(ast);
}

/* Helper: make an echo command that writes to a file (for side-effect testing) */
static t_ast	*make_echo_to_file(const char *filepath, const char *text)
{
	t_ast	*ast;
	t_redir	*redir;

	ast = calloc(1, sizeof(t_ast));
	ast->type = NODE_COMMAND;
	ast->data.cmd = calloc(1, sizeof(t_cmd));
	ast->data.cmd->argv = calloc(3, sizeof(char *));
	ast->data.cmd->argv[0] = strdup("/bin/echo");
	ast->data.cmd->argv[1] = strdup(text);
	ast->data.cmd->argc = 2;
	redir = calloc(1, sizeof(t_redir));
	redir->type = TOK_REDIR_OUT;
	redir->fd = -1;
	redir->target = strdup(filepath);
	ast->data.cmd->redirs = ft_lstnew(redir);
	return (ast);
}

static void	free_echo_ast(t_ast *ast)
{
	t_redir	*r;

	if (!ast)
		return ;
	if (ast->data.cmd->redirs)
	{
		r = REDIR(ast->data.cmd->redirs);
		free(r->target);
		free(r);
		free(ast->data.cmd->redirs);
	}
	if (ast->data.cmd->argv)
	{
		int i = 0;
		while (ast->data.cmd->argv[i])
			free(ast->data.cmd->argv[i++]);
		free(ast->data.cmd->argv);
	}
	free(ast->data.cmd);
	free(ast);
}

static void	test_execute_and(void)
{
	t_shell	shell;
	t_ast	and_node;
	int		status;
	t_ast	*left;
	t_ast	*right;

	stub_shell_init(&shell);
	var_set(&shell, "PATH", "/usr/bin:/bin");

	and_node.data.binary = calloc(1, sizeof(t_binary));

	/* true && true => 0 */
	left = make_true_cmd();
	right = make_true_cmd();
	and_node.type = NODE_AND;
	and_node.data.binary->left = left;
	and_node.data.binary->right = right;
	status = executor_execute(&shell, &and_node);
	MU_ASSERT_INT(0, status);
	free_cmd_ast(left);
	free_cmd_ast(right);

	/* true && false => 1 */
	left = make_true_cmd();
	right = make_false_cmd();
	and_node.data.binary->left = left;
	and_node.data.binary->right = right;
	status = executor_execute(&shell, &and_node);
	MU_ASSERT("true && false != 0", status != 0);
	free_cmd_ast(left);
	free_cmd_ast(right);

	/* false && true => 1 (right not executed) */
	unlink("/tmp/42sh_and_skip");
	left = make_false_cmd();
	right = make_echo_to_file("/tmp/42sh_and_skip", "should_not_run");
	and_node.data.binary->left = left;
	and_node.data.binary->right = right;
	status = executor_execute(&shell, &and_node);
	MU_ASSERT("false && ... != 0", status != 0);
	MU_ASSERT("right not executed", access("/tmp/42sh_and_skip", F_OK) != 0);
	free_cmd_ast(left);
	free_echo_ast(right);

	/* false && false => 1 */
	left = make_false_cmd();
	right = make_false_cmd();
	and_node.data.binary->left = left;
	and_node.data.binary->right = right;
	status = executor_execute(&shell, &and_node);
	MU_ASSERT("false && false != 0", status != 0);
	free_cmd_ast(left);
	free_cmd_ast(right);

	free(and_node.data.binary);
	stub_shell_cleanup(&shell);
}

static void	test_execute_or(void)
{
	t_shell	shell;
	t_ast	or_node;
	int		status;
	t_ast	*left;
	t_ast	*right;

	stub_shell_init(&shell);
	var_set(&shell, "PATH", "/usr/bin:/bin");

	or_node.data.binary = calloc(1, sizeof(t_binary));

	/* false || true => 0 */
	left = make_false_cmd();
	right = make_true_cmd();
	or_node.type = NODE_OR;
	or_node.data.binary->left = left;
	or_node.data.binary->right = right;
	status = executor_execute(&shell, &or_node);
	MU_ASSERT_INT(0, status);
	free_cmd_ast(left);
	free_cmd_ast(right);

	/* true || false => 0 (right not executed) */
	unlink("/tmp/42sh_or_skip");
	left = make_true_cmd();
	right = make_echo_to_file("/tmp/42sh_or_skip", "should_not_run");
	or_node.data.binary->left = left;
	or_node.data.binary->right = right;
	status = executor_execute(&shell, &or_node);
	MU_ASSERT_INT(0, status);
	MU_ASSERT("or right not executed", access("/tmp/42sh_or_skip", F_OK) != 0);
	unlink("/tmp/42sh_or_skip");
	free_cmd_ast(left);
	free_echo_ast(right);

	/* false || false => 1 */
	left = make_false_cmd();
	right = make_false_cmd();
	or_node.data.binary->left = left;
	or_node.data.binary->right = right;
	status = executor_execute(&shell, &or_node);
	MU_ASSERT("false || false != 0", status != 0);
	free_cmd_ast(left);
	free_cmd_ast(right);

	/* true || true => 0 */
	left = make_true_cmd();
	right = make_true_cmd();
	or_node.data.binary->left = left;
	or_node.data.binary->right = right;
	status = executor_execute(&shell, &or_node);
	MU_ASSERT_INT(0, status);
	free_cmd_ast(left);
	free_cmd_ast(right);

	free(or_node.data.binary);
	stub_shell_cleanup(&shell);
}

static void	test_execute_sequence(void)
{
	t_shell	shell;
	t_ast	seq_node;
	int		status;
	t_ast	*left;
	t_ast	*right;

	stub_shell_init(&shell);
	var_set(&shell, "PATH", "/usr/bin:/bin");

	seq_node.data.binary = calloc(1, sizeof(t_binary));

	/* true ; false => 1 (returns last) */
	left = make_true_cmd();
	right = make_false_cmd();
	seq_node.type = NODE_SEQUENCE;
	seq_node.data.binary->left = left;
	seq_node.data.binary->right = right;
	status = executor_execute(&shell, &seq_node);
	MU_ASSERT("sequence returns right status", status != 0);
	free_cmd_ast(left);
	free_cmd_ast(right);

	/* false ; true => 0 */
	left = make_false_cmd();
	right = make_true_cmd();
	seq_node.data.binary->left = left;
	seq_node.data.binary->right = right;
	status = executor_execute(&shell, &seq_node);
	MU_ASSERT_INT(0, status);
	free_cmd_ast(left);
	free_cmd_ast(right);

	/* Both sides always execute */
	left = make_echo_to_file("/tmp/42sh_seq_left", "left_ran");
	right = make_echo_to_file("/tmp/42sh_seq_right", "right_ran");
	seq_node.data.binary->left = left;
	seq_node.data.binary->right = right;
	executor_execute(&shell, &seq_node);
	MU_ASSERT("seq left executed", access("/tmp/42sh_seq_left", F_OK) == 0);
	MU_ASSERT("seq right executed", access("/tmp/42sh_seq_right", F_OK) == 0);
	unlink("/tmp/42sh_seq_left");
	unlink("/tmp/42sh_seq_right");
	free_echo_ast(left);
	free_echo_ast(right);

	free(seq_node.data.binary);
	stub_shell_cleanup(&shell);
}

/* ================================================================
 * 6. Simple command: external + builtin
 * ================================================================ */

static void	test_execute_external_command(void)
{
	t_shell	shell;
	int		status;
	t_ast	*ast;
	char	buf[256];
	int		fd;
	ssize_t	n;

	stub_shell_init(&shell);
	var_set(&shell, "PATH", "/usr/bin:/bin");

	/* /bin/echo hello > file */
	ast = make_echo_to_file("/tmp/42sh_test_ext_cmd", "hello_world");
	status = executor_execute(&shell, ast);
	MU_ASSERT_INT(0, status);

	fd = open("/tmp/42sh_test_ext_cmd", O_RDONLY);
	MU_ASSERT("ext cmd output file exists", fd >= 0);
	n = read(fd, buf, sizeof(buf) - 1);
	buf[n] = '\0';
	MU_ASSERT_STR("ext cmd output", "hello_world\n", buf);
	close(fd);
	unlink("/tmp/42sh_test_ext_cmd");
	free_echo_ast(ast);

	/* Command not found => 127 */
	ast = calloc(1, sizeof(t_ast));
	ast->type = NODE_COMMAND;
	ast->data.cmd = calloc(1, sizeof(t_cmd));
	ast->data.cmd->argv = calloc(2, sizeof(char *));
	ast->data.cmd->argv[0] = strdup("zzz_nonexistent_42sh_cmd");
	ast->data.cmd->argc = 1;
	status = executor_execute(&shell, ast);
	MU_ASSERT_INT(127, status);
	free_cmd_ast(ast);

	stub_shell_cleanup(&shell);
}

static int	mock_builtin_echo(t_shell *shell, int argc, char **argv)
{
	int	i;

	(void)shell;
	i = 1;
	while (i < argc)
	{
		if (i > 1)
			write(1, " ", 1);
		write(1, argv[i], strlen(argv[i]));
		i++;
	}
	write(1, "\n", 1);
	return (0);
}

static void	test_execute_builtin_command(void)
{
	t_shell	shell;
	t_ast	ast;
	int		status;
	char	buf[256];
	int		fd;
	ssize_t	n;

	stub_shell_init(&shell);
	stub_set_builtin("echo", mock_builtin_echo);

	/* builtin echo with output redirect */
	memset(&ast, 0, sizeof(ast));
	ast.type = NODE_COMMAND;
	ast.data.cmd = calloc(1, sizeof(t_cmd));
	ast.data.cmd->argv = calloc(3, sizeof(char *));
	ast.data.cmd->argv[0] = strdup("echo");
	ast.data.cmd->argv[1] = strdup("builtin_test");
	ast.data.cmd->argc = 2;

	{
		t_redir	*redir = calloc(1, sizeof(t_redir));
		redir->type = TOK_REDIR_OUT;
		redir->fd = -1;
		redir->target = strdup("/tmp/42sh_test_builtin");
		ast.data.cmd->redirs = ft_lstnew(redir);
	}

	status = executor_execute(&shell, &ast);
	MU_ASSERT_INT(0, status);

	fd = open("/tmp/42sh_test_builtin", O_RDONLY);
	MU_ASSERT("builtin output file exists", fd >= 0);
	n = read(fd, buf, sizeof(buf) - 1);
	buf[n] = '\0';
	MU_ASSERT_STR("builtin output", "builtin_test\n", buf);
	close(fd);
	unlink("/tmp/42sh_test_builtin");

	/* Cleanup */
	{
		t_redir *r = REDIR(ast.data.cmd->redirs);
		free(r->target);
		free(r);
		free(ast.data.cmd->redirs);
	}
	free(ast.data.cmd->argv[0]);
	free(ast.data.cmd->argv[1]);
	free(ast.data.cmd->argv);
	free(ast.data.cmd);

	stub_set_builtin(NULL, NULL);
	stub_shell_cleanup(&shell);
}

/* ================================================================
 * 7. Simple command: empty (assignment only)
 * TODO: uncomment when var_set/var_get_value are implemented
 * ================================================================ */
#if 0

static void	test_execute_assignment_only(void)
{
	t_shell	shell;
	t_ast	ast;
	int		status;
	char	*assign_str;

	stub_shell_init(&shell);

	/* FOO=bar (no command) */
	memset(&ast, 0, sizeof(ast));
	ast.type = NODE_COMMAND;
	ast.data.cmd = calloc(1, sizeof(t_cmd));
	ast.data.cmd->argv = calloc(1, sizeof(char *));
	ast.data.cmd->argv[0] = NULL;
	ast.data.cmd->argc = 0;
	assign_str = strdup("FOO=bar");
	ast.data.cmd->assignments = ft_lstnew(assign_str);

	status = executor_execute(&shell, &ast);
	MU_ASSERT_INT(0, status);
	MU_ASSERT_STR("FOO set", "bar", var_get_value(&shell, "FOO"));

	free(assign_str);
	free(ast.data.cmd->assignments);
	free(ast.data.cmd->argv);
	free(ast.data.cmd);

	stub_shell_cleanup(&shell);
}
#endif

/* ================================================================
 * 8. Pipeline
 * ================================================================ */

static void	test_execute_pipeline(void)
{
	t_shell	shell;
	t_ast	pipe_node;
	t_ast	*left;
	t_ast	*right;
	int		status;
	int		fd;
	char	buf[256];
	ssize_t	n;

	stub_shell_init(&shell);
	var_set(&shell, "PATH", "/usr/bin:/bin");

	/* echo hello | cat > file */
	left = calloc(1, sizeof(t_ast));
	left->type = NODE_COMMAND;
	left->data.cmd = calloc(1, sizeof(t_cmd));
	left->data.cmd->argv = calloc(3, sizeof(char *));
	left->data.cmd->argv[0] = strdup("/bin/echo");
	left->data.cmd->argv[1] = strdup("pipe_test");
	left->data.cmd->argc = 2;

	right = calloc(1, sizeof(t_ast));
	right->type = NODE_COMMAND;
	right->data.cmd = calloc(1, sizeof(t_cmd));
	right->data.cmd->argv = calloc(2, sizeof(char *));
	right->data.cmd->argv[0] = strdup("/bin/cat");
	right->data.cmd->argc = 1;
	{
		t_redir	*redir = calloc(1, sizeof(t_redir));
		redir->type = TOK_REDIR_OUT;
		redir->fd = -1;
		redir->target = strdup("/tmp/42sh_test_pipeline");
		right->data.cmd->redirs = ft_lstnew(redir);
	}

	pipe_node.type = NODE_PIPE;
	pipe_node.data.binary = calloc(1, sizeof(t_binary));
	pipe_node.data.binary->left = left;
	pipe_node.data.binary->right = right;

	status = executor_execute(&shell, &pipe_node);
	MU_ASSERT_INT(0, status);

	fd = open("/tmp/42sh_test_pipeline", O_RDONLY);
	MU_ASSERT("pipeline output file exists", fd >= 0);
	n = read(fd, buf, sizeof(buf) - 1);
	buf[n] = '\0';
	MU_ASSERT_STR("pipeline output", "pipe_test\n", buf);
	close(fd);
	unlink("/tmp/42sh_test_pipeline");

	/* Cleanup */
	{
		t_redir *r = REDIR(right->data.cmd->redirs);
		free(r->target);
		free(r);
		free(right->data.cmd->redirs);
	}
	free_cmd_ast(left);
	free(right->data.cmd->argv[0]);
	free(right->data.cmd->argv);
	free(right->data.cmd);
	free(right);
	free(pipe_node.data.binary);

	stub_shell_cleanup(&shell);
}

static void	test_execute_pipeline_three(void)
{
	t_shell	shell;
	t_ast	pipe_outer;
	t_ast	pipe_inner;
	t_ast	*cmd1;
	t_ast	*cmd2;
	t_ast	*cmd3;
	int		status;
	int		fd;
	char	buf[1024];
	ssize_t	n;

	stub_shell_init(&shell);
	var_set(&shell, "PATH", "/usr/bin:/bin");

	/* echo -e "c\nb\na" | sort | head -1 > file */
	/* Simpler: printf "c\nb\na\n" | sort | head -n 1 */
	cmd1 = calloc(1, sizeof(t_ast));
	cmd1->type = NODE_COMMAND;
	cmd1->data.cmd = calloc(1, sizeof(t_cmd));
	cmd1->data.cmd->argv = calloc(3, sizeof(char *));
	cmd1->data.cmd->argv[0] = strdup("/usr/bin/printf");
	cmd1->data.cmd->argv[1] = strdup("c\nb\na\n");
	cmd1->data.cmd->argc = 2;

	cmd2 = calloc(1, sizeof(t_ast));
	cmd2->type = NODE_COMMAND;
	cmd2->data.cmd = calloc(1, sizeof(t_cmd));
	cmd2->data.cmd->argv = calloc(2, sizeof(char *));
	cmd2->data.cmd->argv[0] = strdup("/usr/bin/sort");
	cmd2->data.cmd->argc = 1;

	cmd3 = calloc(1, sizeof(t_ast));
	cmd3->type = NODE_COMMAND;
	cmd3->data.cmd = calloc(1, sizeof(t_cmd));
	cmd3->data.cmd->argv = calloc(4, sizeof(char *));
	cmd3->data.cmd->argv[0] = strdup("/usr/bin/head");
	cmd3->data.cmd->argv[1] = strdup("-n");
	cmd3->data.cmd->argv[2] = strdup("1");
	cmd3->data.cmd->argc = 3;
	{
		t_redir	*redir = calloc(1, sizeof(t_redir));
		redir->type = TOK_REDIR_OUT;
		redir->fd = -1;
		redir->target = strdup("/tmp/42sh_test_pipe3");
		cmd3->data.cmd->redirs = ft_lstnew(redir);
	}

	/* (cmd1 | cmd2) | cmd3 */
	pipe_inner.type = NODE_PIPE;
	pipe_inner.data.binary = calloc(1, sizeof(t_binary));
	pipe_inner.data.binary->left = cmd1;
	pipe_inner.data.binary->right = cmd2;

	pipe_outer.type = NODE_PIPE;
	pipe_outer.data.binary = calloc(1, sizeof(t_binary));
	pipe_outer.data.binary->left = &pipe_inner;
	pipe_outer.data.binary->right = cmd3;

	status = executor_execute(&shell, &pipe_outer);
	MU_ASSERT_INT(0, status);

	fd = open("/tmp/42sh_test_pipe3", O_RDONLY);
	MU_ASSERT("3-stage pipeline output exists", fd >= 0);
	n = read(fd, buf, sizeof(buf) - 1);
	buf[n] = '\0';
	MU_ASSERT_STR("3-stage pipeline output", "a\n", buf);
	close(fd);
	unlink("/tmp/42sh_test_pipe3");

	/* Cleanup */
	{
		t_redir *r = REDIR(cmd3->data.cmd->redirs);
		free(r->target);
		free(r);
		free(cmd3->data.cmd->redirs);
	}
	free_cmd_ast(cmd1);
	free_cmd_ast(cmd2);
	free(cmd3->data.cmd->argv[0]);
	free(cmd3->data.cmd->argv[1]);
	free(cmd3->data.cmd->argv[2]);
	free(cmd3->data.cmd->argv);
	free(cmd3->data.cmd);
	free(cmd3);
	free(pipe_inner.data.binary);
	free(pipe_outer.data.binary);

	stub_shell_cleanup(&shell);
}

static void	test_pipeline_exit_status(void)
{
	t_shell	shell;
	t_ast	pipe_node;
	t_ast	*left;
	t_ast	*right;
	int		status;

	stub_shell_init(&shell);
	var_set(&shell, "PATH", "/usr/bin:/bin");

	pipe_node.data.binary = calloc(1, sizeof(t_binary));

	/* Pipeline exit status = last command's exit status */
	/* true | false => 1 */
	left = make_true_cmd();
	right = make_false_cmd();
	pipe_node.type = NODE_PIPE;
	pipe_node.data.binary->left = left;
	pipe_node.data.binary->right = right;
	status = executor_execute(&shell, &pipe_node);
	MU_ASSERT("true | false != 0", status != 0);
	free_cmd_ast(left);
	free_cmd_ast(right);

	/* false | true => 0 */
	left = make_false_cmd();
	right = make_true_cmd();
	pipe_node.data.binary->left = left;
	pipe_node.data.binary->right = right;
	status = executor_execute(&shell, &pipe_node);
	MU_ASSERT_INT(0, status);
	free_cmd_ast(left);
	free_cmd_ast(right);

	free(pipe_node.data.binary);
	stub_shell_cleanup(&shell);
}

/* ================================================================
 * 9. Subshell and Block
 * ================================================================ */

static void	test_execute_subshell(void)
{
	t_shell	shell;
	t_ast	subshell;
	t_ast	*child;
	int		status;

	stub_shell_init(&shell);
	var_set(&shell, "PATH", "/usr/bin:/bin");

	subshell.data.group = calloc(1, sizeof(t_group));

	/* ( true ) => 0 */
	child = make_true_cmd();
	subshell.type = NODE_SUBSHELL;
	subshell.data.group->child = child;
	subshell.data.group->redirs = NULL;
	status = executor_execute(&shell, &subshell);
	MU_ASSERT_INT(0, status);
	free_cmd_ast(child);

	/* ( false ) => 1 */
	child = make_false_cmd();
	subshell.data.group->child = child;
	status = executor_execute(&shell, &subshell);
	MU_ASSERT("subshell false != 0", status != 0);
	free_cmd_ast(child);

	free(subshell.data.group);
	stub_shell_cleanup(&shell);
}

static void	test_execute_subshell_isolation(void)
{
	t_shell	shell;
	t_ast	subshell;
	t_ast	*child;
	int		status;
	int		fd;
	char	buf[256];
	ssize_t	n;

	stub_shell_init(&shell);
	var_set(&shell, "PATH", "/usr/bin:/bin");

	/* Subshell with redirect: ( echo hello ) > file */
	child = calloc(1, sizeof(t_ast));
	child->type = NODE_COMMAND;
	child->data.cmd = calloc(1, sizeof(t_cmd));
	child->data.cmd->argv = calloc(3, sizeof(char *));
	child->data.cmd->argv[0] = strdup("/bin/echo");
	child->data.cmd->argv[1] = strdup("subshell_out");
	child->data.cmd->argc = 2;

	subshell.type = NODE_SUBSHELL;
	subshell.data.group = calloc(1, sizeof(t_group));
	subshell.data.group->child = child;
	{
		t_redir	*redir = calloc(1, sizeof(t_redir));
		redir->type = TOK_REDIR_OUT;
		redir->fd = -1;
		redir->target = strdup("/tmp/42sh_test_subshell");
		subshell.data.group->redirs = ft_lstnew(redir);
	}

	status = executor_execute(&shell, &subshell);
	MU_ASSERT_INT(0, status);

	fd = open("/tmp/42sh_test_subshell", O_RDONLY);
	MU_ASSERT("subshell output file exists", fd >= 0);
	n = read(fd, buf, sizeof(buf) - 1);
	buf[n] = '\0';
	MU_ASSERT_STR("subshell output", "subshell_out\n", buf);
	close(fd);
	unlink("/tmp/42sh_test_subshell");

	{
		t_redir *r = REDIR(subshell.data.group->redirs);
		free(r->target);
		free(r);
		free(subshell.data.group->redirs);
	}
	free_cmd_ast(child);
	free(subshell.data.group);
	stub_shell_cleanup(&shell);
}

static void	test_execute_block(void)
{
	t_shell	shell;
	t_ast	block;
	t_ast	*child;
	int		status;
	int		fd;
	char	buf[256];
	ssize_t	n;

	stub_shell_init(&shell);
	var_set(&shell, "PATH", "/usr/bin:/bin");

	/* { echo hello; } > file */
	child = calloc(1, sizeof(t_ast));
	child->type = NODE_COMMAND;
	child->data.cmd = calloc(1, sizeof(t_cmd));
	child->data.cmd->argv = calloc(3, sizeof(char *));
	child->data.cmd->argv[0] = strdup("/bin/echo");
	child->data.cmd->argv[1] = strdup("block_out");
	child->data.cmd->argc = 2;

	block.type = NODE_BLOCK;
	block.data.group = calloc(1, sizeof(t_group));
	block.data.group->child = child;
	{
		t_redir	*redir = calloc(1, sizeof(t_redir));
		redir->type = TOK_REDIR_OUT;
		redir->fd = -1;
		redir->target = strdup("/tmp/42sh_test_block");
		block.data.group->redirs = ft_lstnew(redir);
	}

	status = executor_execute(&shell, &block);
	MU_ASSERT_INT(0, status);

	fd = open("/tmp/42sh_test_block", O_RDONLY);
	MU_ASSERT("block output file exists", fd >= 0);
	n = read(fd, buf, sizeof(buf) - 1);
	buf[n] = '\0';
	MU_ASSERT_STR("block output", "block_out\n", buf);
	close(fd);
	unlink("/tmp/42sh_test_block");

	{
		t_redir *r = REDIR(block.data.group->redirs);
		free(r->target);
		free(r);
		free(block.data.group->redirs);
	}
	free_cmd_ast(child);
	free(block.data.group);
	stub_shell_cleanup(&shell);
}

/* ================================================================
 * 10. Background execution
 * ================================================================ */

static void	test_execute_background(void)
{
	t_shell	shell;
	t_ast	bg;
	t_ast	*child;
	int		status;
	int		tries;

	stub_shell_init(&shell);
	var_set(&shell, "PATH", "/usr/bin:/bin");

	/* cmd & => returns 0 immediately, child runs in background */
	unlink("/tmp/42sh_test_bg");
	child = calloc(1, sizeof(t_ast));
	child->type = NODE_COMMAND;
	child->data.cmd = calloc(1, sizeof(t_cmd));
	child->data.cmd->argv = calloc(4, sizeof(char *));
	child->data.cmd->argv[0] = strdup("/usr/bin/touch");
	child->data.cmd->argv[1] = strdup("/tmp/42sh_test_bg");
	child->data.cmd->argc = 2;

	bg.type = NODE_BACKGROUND;
	bg.data.group = calloc(1, sizeof(t_group));
	bg.data.group->child = child;
	bg.data.group->redirs = NULL;

	status = executor_execute(&shell, &bg);
	MU_ASSERT_INT(0, status);

	/* Wait briefly for background process to complete */
	tries = 0;
	while (access("/tmp/42sh_test_bg", F_OK) != 0 && tries < 50)
	{
		usleep(10000);
		tries++;
	}
	MU_ASSERT("background cmd created file", access("/tmp/42sh_test_bg", F_OK) == 0);
	unlink("/tmp/42sh_test_bg");

	free_cmd_ast(child);
	free(bg.data.group);
	stub_shell_cleanup(&shell);
}

/* ================================================================
 * 11. executor_execute dispatch + NULL
 * ================================================================ */

static void	test_executor_dispatch(void)
{
	t_shell	shell;
	int		status;

	stub_shell_init(&shell);
	var_set(&shell, "PATH", "/usr/bin:/bin");

	/* NULL ast => 0 */
	status = executor_execute(&shell, NULL);
	MU_ASSERT_INT(0, status);

	/* last_exit_status is updated */
	{
		t_ast *ast = make_true_cmd();
		executor_execute(&shell, ast);
		MU_ASSERT_INT(0, shell.last_exit_status);
		free_cmd_ast(ast);
	}
	{
		t_ast *ast = make_false_cmd();
		executor_execute(&shell, ast);
		MU_ASSERT("last_exit != 0 after false", shell.last_exit_status != 0);
		free_cmd_ast(ast);
	}

	stub_shell_cleanup(&shell);
}

/* ================================================================
 * 12. Complex: chained logical operators
 * ================================================================ */

static void	test_complex_logical_chains(void)
{
	t_shell	shell;
	t_ast	outer;
	t_ast	inner;
	t_ast	*cmd1;
	t_ast	*cmd2;
	t_ast	*cmd3;
	int		status;

	stub_shell_init(&shell);
	var_set(&shell, "PATH", "/usr/bin:/bin");

	inner.data.binary = calloc(1, sizeof(t_binary));
	outer.data.binary = calloc(1, sizeof(t_binary));

	/* true && true && false => false
	   Structure: (true && true) && false */
	cmd1 = make_true_cmd();
	cmd2 = make_true_cmd();
	cmd3 = make_false_cmd();
	inner.type = NODE_AND;
	inner.data.binary->left = cmd1;
	inner.data.binary->right = cmd2;
	outer.type = NODE_AND;
	outer.data.binary->left = &inner;
	outer.data.binary->right = cmd3;
	status = executor_execute(&shell, &outer);
	MU_ASSERT("true&&true&&false != 0", status != 0);
	free_cmd_ast(cmd1);
	free_cmd_ast(cmd2);
	free_cmd_ast(cmd3);

	/* false || true && false => false
	   Structure: (false || true) && false */
	cmd1 = make_false_cmd();
	cmd2 = make_true_cmd();
	cmd3 = make_false_cmd();
	inner.type = NODE_OR;
	inner.data.binary->left = cmd1;
	inner.data.binary->right = cmd2;
	outer.type = NODE_AND;
	outer.data.binary->left = &inner;
	outer.data.binary->right = cmd3;
	status = executor_execute(&shell, &outer);
	MU_ASSERT("(false||true)&&false != 0", status != 0);
	free_cmd_ast(cmd1);
	free_cmd_ast(cmd2);
	free_cmd_ast(cmd3);

	/* false || false || true => 0
	   Structure: (false || false) || true */
	cmd1 = make_false_cmd();
	cmd2 = make_false_cmd();
	cmd3 = make_true_cmd();
	inner.type = NODE_OR;
	inner.data.binary->left = cmd1;
	inner.data.binary->right = cmd2;
	outer.type = NODE_OR;
	outer.data.binary->left = &inner;
	outer.data.binary->right = cmd3;
	status = executor_execute(&shell, &outer);
	MU_ASSERT_INT(0, status);
	free_cmd_ast(cmd1);
	free_cmd_ast(cmd2);
	free_cmd_ast(cmd3);

	free(inner.data.binary);
	free(outer.data.binary);
	stub_shell_cleanup(&shell);
}

/* ================================================================
 * 13. Builtin with temporary assignments
 * TODO: uncomment when var_set/var_get_value are implemented
 * ================================================================ */
#if 0

static int	mock_builtin_getvar(t_shell *shell, int argc, char **argv)
{
	const char	*val;

	(void)argc;
	(void)argv;
	val = var_get_value(shell, "TESTVAR");
	if (val && strcmp(val, "temporary") == 0)
		return (0);
	return (1);
}

static void	test_builtin_temp_assignments(void)
{
	t_shell	shell;
	t_ast	ast;
	int		status;
	char	*assign_str;

	stub_shell_init(&shell);
	var_set(&shell, "TESTVAR", "original");
	stub_set_builtin("getvar", mock_builtin_getvar);

	/* TESTVAR=temporary getvar => builtin sees "temporary" */
	memset(&ast, 0, sizeof(ast));
	ast.type = NODE_COMMAND;
	ast.data.cmd = calloc(1, sizeof(t_cmd));
	ast.data.cmd->argv = calloc(2, sizeof(char *));
	ast.data.cmd->argv[0] = strdup("getvar");
	ast.data.cmd->argc = 1;
	assign_str = strdup("TESTVAR=temporary");
	ast.data.cmd->assignments = ft_lstnew(assign_str);

	status = executor_execute(&shell, &ast);
	MU_ASSERT_INT(0, status);

	/* After builtin, TESTVAR should be restored to "original" */
	MU_ASSERT_STR("TESTVAR restored", "original",
		var_get_value(&shell, "TESTVAR"));

	free(assign_str);
	free(ast.data.cmd->assignments);
	free(ast.data.cmd->argv[0]);
	free(ast.data.cmd->argv);
	free(ast.data.cmd);
	stub_set_builtin(NULL, NULL);
	stub_shell_cleanup(&shell);
}
#endif

/* ================================================================
 * 14. Pipeline with heredoc
 * ================================================================ */

static void	test_pipeline_with_heredoc(void)
{
	t_shell	shell;
	t_ast	pipe_node;
	t_ast	*left;
	t_ast	*right;
	int		status;
	int		fd;
	char	buf[256];
	ssize_t	n;

	stub_shell_init(&shell);
	var_set(&shell, "PATH", "/usr/bin:/bin");

	/* cat <<EOF | cat > file
	   heredoc_content: "from_heredoc\n" */
	left = calloc(1, sizeof(t_ast));
	left->type = NODE_COMMAND;
	left->data.cmd = calloc(1, sizeof(t_cmd));
	left->data.cmd->argv = calloc(2, sizeof(char *));
	left->data.cmd->argv[0] = strdup("/bin/cat");
	left->data.cmd->argc = 1;
	{
		t_redir	*redir = calloc(1, sizeof(t_redir));
		redir->type = TOK_HEREDOC;
		redir->fd = -1;
		redir->heredoc_content = strdup("from_heredoc\n");
		left->data.cmd->redirs = ft_lstnew(redir);
	}

	right = calloc(1, sizeof(t_ast));
	right->type = NODE_COMMAND;
	right->data.cmd = calloc(1, sizeof(t_cmd));
	right->data.cmd->argv = calloc(2, sizeof(char *));
	right->data.cmd->argv[0] = strdup("/bin/cat");
	right->data.cmd->argc = 1;
	{
		t_redir	*redir = calloc(1, sizeof(t_redir));
		redir->type = TOK_REDIR_OUT;
		redir->fd = -1;
		redir->target = strdup("/tmp/42sh_test_pipe_heredoc");
		right->data.cmd->redirs = ft_lstnew(redir);
	}

	pipe_node.type = NODE_PIPE;
	pipe_node.data.binary = calloc(1, sizeof(t_binary));
	pipe_node.data.binary->left = left;
	pipe_node.data.binary->right = right;

	status = executor_execute(&shell, &pipe_node);
	MU_ASSERT_INT(0, status);

	fd = open("/tmp/42sh_test_pipe_heredoc", O_RDONLY);
	MU_ASSERT("pipe+heredoc output exists", fd >= 0);
	n = read(fd, buf, sizeof(buf) - 1);
	buf[n] = '\0';
	MU_ASSERT_STR("pipe+heredoc output", "from_heredoc\n", buf);
	close(fd);
	unlink("/tmp/42sh_test_pipe_heredoc");

	/* Cleanup left */
	{
		t_redir *r = REDIR(left->data.cmd->redirs);
		free(r->heredoc_content);
		free(r);
		free(left->data.cmd->redirs);
	}
	free(left->data.cmd->argv[0]);
	free(left->data.cmd->argv);
	free(left->data.cmd);
	free(left);

	/* Cleanup right */
	{
		t_redir *r = REDIR(right->data.cmd->redirs);
		free(r->target);
		free(r);
		free(right->data.cmd->redirs);
	}
	free(right->data.cmd->argv[0]);
	free(right->data.cmd->argv);
	free(right->data.cmd);
	free(right);
	free(pipe_node.data.binary);

	stub_shell_cleanup(&shell);
}

/* ================================================================
 * 15. Sequence with mixed operators
 * ================================================================ */

static void	test_sequence_mixed(void)
{
	t_shell	shell;
	t_ast	seq;
	t_ast	and_node;
	t_ast	*cmd1;
	t_ast	*cmd2;
	t_ast	*cmd3;
	int		status;

	stub_shell_init(&shell);
	var_set(&shell, "PATH", "/usr/bin:/bin");

	and_node.data.binary = calloc(1, sizeof(t_binary));
	seq.data.binary = calloc(1, sizeof(t_binary));

	/* (true && false) ; true => 0
	   Sequence returns right side's status */
	cmd1 = make_true_cmd();
	cmd2 = make_false_cmd();
	cmd3 = make_true_cmd();
	and_node.type = NODE_AND;
	and_node.data.binary->left = cmd1;
	and_node.data.binary->right = cmd2;
	seq.type = NODE_SEQUENCE;
	seq.data.binary->left = &and_node;
	seq.data.binary->right = cmd3;
	status = executor_execute(&shell, &seq);
	MU_ASSERT_INT(0, status);
	free_cmd_ast(cmd1);
	free_cmd_ast(cmd2);
	free_cmd_ast(cmd3);

	free(and_node.data.binary);
	free(seq.data.binary);
	stub_shell_cleanup(&shell);
}

/* ================================================================
 * Master test suite entry point
 * ================================================================ */

void	test_executor_suite(void)
{
	/* 1. Utilities */
	test_get_exit_status();
	test_split_assignment();

	/* 2. Command search */
	/* TODO: uncomment when var_set/var_get_value are implemented */
	/* test_find_command(); */

	/* 3. Heredoc */
	test_heredoc();

	/* 4. Redirections */
	test_redirections_output();
	test_redirections_append();
	test_redirections_input();
	test_redirections_heredoc_in_list();
	test_redirections_bad_file();
	test_redirections_save_restore();

	/* 5. Logical operators */
	test_execute_and();
	test_execute_or();
	test_execute_sequence();

	/* 6. Simple commands */
	test_execute_external_command();
	test_execute_builtin_command();
	/* TODO: uncomment when var_set/var_get_value are implemented */
	/* test_execute_assignment_only(); */

	/* 7. Pipelines */
	test_execute_pipeline();
	test_execute_pipeline_three();
	test_pipeline_exit_status();

	/* 8. Subshell, block, background */
	test_execute_subshell();
	test_execute_subshell_isolation();
	test_execute_block();
	test_execute_background();

	/* 9. Dispatch & edge cases */
	test_executor_dispatch();

	/* 10. Complex scenarios */
	test_complex_logical_chains();
	/* TODO: uncomment when var_set/var_get_value are implemented */
	/* test_builtin_temp_assignments(); */
	test_pipeline_with_heredoc();
	test_sequence_mixed();
}

#else

void	test_executor_suite(void)
{
}

#endif
