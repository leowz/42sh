/**
 * @file test_builtin_hash.c
 * @brief Unit tests for the hash builtin command.
 * @author pulgamecanica
 */

#ifdef TEST_BUILTIN_HASH_ENABLED
#endif

#define BUFSIZE 1024

#include "minunit.h"
#include "builtins.h"
#include "executor.h"
#include "variables.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

static int	capture_hash_stdout(t_shell *shell, int argc, char **argv,
	char *buf, size_t buf_size)
{
	int		pipefd[2];
	int		saved_stdout;
	int		ret;
	ssize_t	n;
	ssize_t	total;

	total = 0;
	memset(buf, 0, buf_size);
	if (pipe(pipefd) == -1)
		return (-1);
	saved_stdout = dup(STDOUT_FILENO);
	if (saved_stdout == -1)
		return (-1);
	fflush(stdout);
	if (dup2(pipefd[1], STDOUT_FILENO) == -1)
		return (-1);
	setvbuf(stdout, NULL, _IONBF, 0);
	close(pipefd[1]);
	ret = builtin_hash(shell, argc, argv);
	fflush(stdout);
	if (dup2(saved_stdout, STDOUT_FILENO) == -1)
		return (-1);
	close(saved_stdout);
	while ((n = read(pipefd[0], buf + total, buf_size - 1 - total)) > 0)
		total += n;
	buf[total] = '\0';
	close(pipefd[0]);
	return (ret);
}

static int	capture_hash_stderr(t_shell *shell, int argc, char **argv,
	char *buf, size_t buf_size)
{
	int		pipefd[2];
	int		saved_stderr;
	int		ret;
	ssize_t	n;
	ssize_t	total;

	total = 0;
	memset(buf, 0, buf_size);
	if (pipe(pipefd) == -1)
		return (-1);
	saved_stderr = dup(STDERR_FILENO);
	if (saved_stderr == -1)
		return (-1);
	if (dup2(pipefd[1], STDERR_FILENO) == -1)
		return (-1);
	close(pipefd[1]);
	ret = builtin_hash(shell, argc, argv);
	if (dup2(saved_stderr, STDERR_FILENO) == -1)
		return (-1);
	close(saved_stderr);
	while ((n = read(pipefd[0], buf + total, buf_size - 1 - total)) > 0)
		total += n;
	buf[total] = '\0';
	close(pipefd[0]);
	return (ret);
}

static void	free_shell_vars(t_shell *shell)
{
	t_list	*cur;
	t_list	*next;
	t_var	*v;

	cur = shell->variables;
	while (cur)
	{
		next = cur->next;
		v = LST_VAR(cur);
		free(v->name);
		free(v->value);
		free(v);
		free(cur);
		cur = next;
	}
	shell->variables = NULL;
}

static void	teardown_shell(t_shell *shell)
{
	cmd_hash_destroy(shell);
	free_shell_vars(shell);
}

/* --- hash (no args): empty table --- */

static void	test_hash_empty_table(void)
{
	t_shell	shell = {0};
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	char *argv[] = {"hash"};
	ret = capture_hash_stdout(&shell, 1, argv, buf, BUFSIZE);
	MU_ASSERT_INT(0, ret);
	MU_ASSERT_STR("empty table message",
		"hash: hash table empty\n", buf);
	teardown_shell(&shell);
}

/* --- hash -r: clear (no-op on empty) --- */

static void	test_hash_clear_empty(void)
{
	t_shell	shell = {0};
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	char *argv[] = {"hash", "-r"};
	ret = capture_hash_stdout(&shell, 2, argv, buf, BUFSIZE);
	MU_ASSERT_INT(0, ret);
	MU_ASSERT_STR("clear produces no output", "", buf);
	teardown_shell(&shell);
}

/* --- hash -p path name: preset entry --- */

static void	test_hash_preset(void)
{
	t_shell	shell = {0};
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	char *argv[] = {"hash", "-p", "/usr/bin/ls", "ls"};
	ret = capture_hash_stdout(&shell, 4, argv, buf, BUFSIZE);
	MU_ASSERT_INT(0, ret);
	MU_ASSERT_STR("preset produces no output", "", buf);
	MU_ASSERT("entry was cached",
		cmd_hash_get(&shell, "ls") != NULL);
	MU_ASSERT_STR("cached path is correct",
		"/usr/bin/ls", cmd_hash_get(&shell, "ls")->path);
	MU_ASSERT_INT(0, (int)cmd_hash_get(&shell, "ls")->hits);
	teardown_shell(&shell);
}

/* --- hash -t name: print cached path --- */

static void	test_hash_type_single(void)
{
	t_shell	shell = {0};
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	cmd_hash_set(&shell, "ls", "/usr/bin/ls");
	char *argv[] = {"hash", "-t", "ls"};
	ret = capture_hash_stdout(&shell, 3, argv, buf, BUFSIZE);
	MU_ASSERT_INT(0, ret);
	MU_ASSERT_STR("type single shows path",
		"/usr/bin/ls\n", buf);
	teardown_shell(&shell);
}

/* --- hash -t name1 name2: multi-name shows "name\tpath" --- */

static void	test_hash_type_multi(void)
{
	t_shell	shell = {0};
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	cmd_hash_set(&shell, "ls", "/usr/bin/ls");
	cmd_hash_set(&shell, "cat", "/usr/bin/cat");
	char *argv[] = {"hash", "-t", "ls", "cat"};
	ret = capture_hash_stdout(&shell, 4, argv, buf, BUFSIZE);
	MU_ASSERT_INT(0, ret);
	MU_ASSERT("multi type shows name+path for ls",
		strstr(buf, "ls\t/usr/bin/ls\n") != NULL);
	MU_ASSERT("multi type shows name+path for cat",
		strstr(buf, "cat\t/usr/bin/cat\n") != NULL);
	teardown_shell(&shell);
}

/* --- hash -t with unknown name: error --- */

static void	test_hash_type_not_found(void)
{
	t_shell	shell = {0};
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	char *argv[] = {"hash", "-t", "nonexistent_xyz"};
	ret = capture_hash_stderr(&shell, 3, argv, buf, BUFSIZE);
	MU_ASSERT_INT(1, ret);
	MU_ASSERT("not found error on stderr",
		strstr(buf, "nonexistent_xyz: not found") != NULL);
	teardown_shell(&shell);
}

/* --- hash -t with no args: usage error --- */

static void	test_hash_type_no_args(void)
{
	t_shell	shell = {0};
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	char *argv[] = {"hash", "-t"};
	ret = capture_hash_stderr(&shell, 2, argv, buf, BUFSIZE);
	MU_ASSERT_INT(2, ret);
	MU_ASSERT("requires argument error",
		strstr(buf, "option requires an argument") != NULL);
	teardown_shell(&shell);
}

/* --- hash -d name: delete entry --- */

static void	test_hash_delete(void)
{
	t_shell	shell = {0};
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	cmd_hash_set(&shell, "ls", "/usr/bin/ls");
	MU_ASSERT("entry exists before delete",
		cmd_hash_get(&shell, "ls") != NULL);
	char *argv[] = {"hash", "-d", "ls"};
	ret = capture_hash_stdout(&shell, 3, argv, buf, BUFSIZE);
	MU_ASSERT_INT(0, ret);
	MU_ASSERT("entry removed after delete",
		cmd_hash_get(&shell, "ls") == NULL);
	teardown_shell(&shell);
}

/* --- hash -d nonexistent: error --- */

static void	test_hash_delete_not_found(void)
{
	t_shell	shell = {0};
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	char *argv[] = {"hash", "-d", "nonexistent_xyz"};
	ret = capture_hash_stderr(&shell, 3, argv, buf, BUFSIZE);
	MU_ASSERT_INT(1, ret);
	MU_ASSERT("delete not found error",
		strstr(buf, "nonexistent_xyz: not found") != NULL);
	teardown_shell(&shell);
}

/* --- hash -d with no args: usage error --- */

static void	test_hash_delete_no_args(void)
{
	t_shell	shell = {0};
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	char *argv[] = {"hash", "-d"};
	ret = capture_hash_stderr(&shell, 2, argv, buf, BUFSIZE);
	MU_ASSERT_INT(2, ret);
	MU_ASSERT("requires argument error",
		strstr(buf, "option requires an argument") != NULL);
	teardown_shell(&shell);
}

/* --- hash -r clears populated table --- */

static void	test_hash_clear_populated(void)
{
	t_shell	shell = {0};
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	cmd_hash_set(&shell, "ls", "/usr/bin/ls");
	cmd_hash_set(&shell, "cat", "/usr/bin/cat");
	char *argv[] = {"hash", "-r"};
	ret = capture_hash_stdout(&shell, 2, argv, buf, BUFSIZE);
	MU_ASSERT_INT(0, ret);
	MU_ASSERT("ls removed after clear",
		cmd_hash_get(&shell, "ls") == NULL);
	MU_ASSERT("cat removed after clear",
		cmd_hash_get(&shell, "cat") == NULL);
	teardown_shell(&shell);
}

/* --- hash (no args) with entries: prints table --- */

static void	test_hash_list_entries(void)
{
	t_shell	shell = {0};
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	cmd_hash_set(&shell, "ls", "/usr/bin/ls");
	char *argv[] = {"hash"};
	ret = capture_hash_stdout(&shell, 1, argv, buf, BUFSIZE);
	MU_ASSERT_INT(0, ret);
	MU_ASSERT("listing contains header",
		strstr(buf, "hits\tcommand\n") != NULL);
	MU_ASSERT("listing contains path",
		strstr(buf, "/usr/bin/ls") != NULL);
	teardown_shell(&shell);
}

/* --- invalid option --- */

static void	test_hash_invalid_option(void)
{
	t_shell	shell = {0};
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	char *argv[] = {"hash", "-x"};
	ret = capture_hash_stderr(&shell, 2, argv, buf, BUFSIZE);
	MU_ASSERT_INT(2, ret);
	MU_ASSERT("invalid option error",
		strstr(buf, "invalid option") != NULL);
	teardown_shell(&shell);
}

/* --- mutually exclusive flags --- */

static void	test_hash_mutual_exclusion(void)
{
	t_shell	shell = {0};
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	char *argv[] = {"hash", "-dt", "ls"};
	ret = capture_hash_stderr(&shell, 3, argv, buf, BUFSIZE);
	MU_ASSERT_INT(2, ret);
	MU_ASSERT("mutually exclusive error",
		strstr(buf, "mutually exclusive") != NULL);
	teardown_shell(&shell);
}

/* --- hash -p wrong arg count --- */

static void	test_hash_preset_wrong_argc(void)
{
	t_shell	shell = {0};
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	char *argv[] = {"hash", "-p", "/usr/bin/ls"};
	ret = capture_hash_stderr(&shell, 3, argv, buf, BUFSIZE);
	MU_ASSERT_INT(2, ret);
	MU_ASSERT("preset expects path name",
		strstr(buf, "expects exactly") != NULL);
	teardown_shell(&shell);
}

/* --- hash -p preserves hits on update --- */

static void	test_hash_preset_preserves_hits(void)
{
	t_shell			shell = {0};
	char			buf[BUFSIZE];
	int				ret;
	t_cmd_hash_value	*v;

	memset(&shell, 0, sizeof(shell));
	cmd_hash_set(&shell, "mybin", "/old/path/mybin");
	v = cmd_hash_get(&shell, "mybin");
	v->hits = 42;
	char *argv[] = {"hash", "-p", "/new/path/mybin", "mybin"};
	ret = capture_hash_stdout(&shell, 4, argv, buf, BUFSIZE);
	MU_ASSERT_INT(0, ret);
	v = cmd_hash_get(&shell, "mybin");
	MU_ASSERT("entry still exists", v != NULL);
	MU_ASSERT_STR("path updated", "/new/path/mybin", v->path);
	MU_ASSERT_INT(42, (int)v->hits);
	teardown_shell(&shell);
}

/* --- hash name: resolve via PATH --- */

static void	test_hash_resolve_name(void)
{
	t_shell	shell = {0};
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	var_set(&shell, "PATH", "/usr/bin:/bin");
	char *argv[] = {"hash", "ls"};
	ret = capture_hash_stdout(&shell, 2, argv, buf, BUFSIZE);
	MU_ASSERT_INT(0, ret);
	MU_ASSERT_STR("resolve produces no output", "", buf);
	MU_ASSERT("ls is now cached",
		cmd_hash_get(&shell, "ls") != NULL);
	teardown_shell(&shell);
}

/* --- hash name (not found): error --- */

static void	test_hash_resolve_not_found(void)
{
	t_shell	shell = {0};
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	var_set(&shell, "PATH", "/usr/bin:/bin");
	char *argv[] = {"hash", "nonexistent_xyz_42sh"};
	ret = capture_hash_stderr(&shell, 2, argv, buf, BUFSIZE);
	MU_ASSERT_INT(1, ret);
	MU_ASSERT("not found error",
		strstr(buf, "nonexistent_xyz_42sh: not found") != NULL);
	teardown_shell(&shell);
}

/* --- hash -- stops option parsing --- */

static void	test_hash_double_dash(void)
{
	t_shell	shell = {0};
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	var_set(&shell, "PATH", "/usr/bin:/bin");
	char *argv[] = {"hash", "--", "ls"};
	ret = capture_hash_stdout(&shell, 3, argv, buf, BUFSIZE);
	MU_ASSERT_INT(0, ret);
	MU_ASSERT("ls cached via -- stop",
		cmd_hash_get(&shell, "ls") != NULL);
	teardown_shell(&shell);
}

/* --- hash -d multiple names --- */

static void	test_hash_delete_multiple(void)
{
	t_shell	shell = {0};
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	cmd_hash_set(&shell, "ls", "/usr/bin/ls");
	cmd_hash_set(&shell, "cat", "/usr/bin/cat");
	cmd_hash_set(&shell, "grep", "/usr/bin/grep");
	char *argv[] = {"hash", "-d", "ls", "cat"};
	ret = capture_hash_stdout(&shell, 4, argv, buf, BUFSIZE);
	MU_ASSERT_INT(0, ret);
	MU_ASSERT("ls deleted", cmd_hash_get(&shell, "ls") == NULL);
	MU_ASSERT("cat deleted", cmd_hash_get(&shell, "cat") == NULL);
	MU_ASSERT("grep still present",
		cmd_hash_get(&shell, "grep") != NULL);
	teardown_shell(&shell);
}

/* --- hash -r then names: clear then resolve --- */

static void	test_hash_clear_then_resolve(void)
{
	t_shell	shell = {0};
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	var_set(&shell, "PATH", "/usr/bin:/bin");
	cmd_hash_set(&shell, "old_cached", "/stale/old_cached");
	char *argv[] = {"hash", "-r", "ls"};
	ret = capture_hash_stdout(&shell, 3, argv, buf, BUFSIZE);
	MU_ASSERT_INT(0, ret);
	MU_ASSERT("old entry cleared",
		cmd_hash_get(&shell, "old_cached") == NULL);
	MU_ASSERT("ls resolved after clear",
		cmd_hash_get(&shell, "ls") != NULL);
	teardown_shell(&shell);
}

/* --- combined flags: -rt, -rd, etc. are rejected --- */

static void	test_hash_combined_flags_rejected(void)
{
	t_shell	shell = {0};
	char	buf[BUFSIZE];
	int		ret;

	memset(&shell, 0, sizeof(shell));
	char *argv[] = {"hash", "-rt"};
	ret = capture_hash_stderr(&shell, 2, argv, buf, BUFSIZE);
	MU_ASSERT_INT(2, ret);
	MU_ASSERT("combined flags rejected",
		strstr(buf, "mutually exclusive") != NULL);
	teardown_shell(&shell);
}

void	test_builtin_hash_suite(void)
{
	test_hash_empty_table();
	test_hash_clear_empty();
	test_hash_preset();
	test_hash_type_single();
	test_hash_type_multi();
	test_hash_type_not_found();
	test_hash_type_no_args();
	test_hash_delete();
	test_hash_delete_not_found();
	test_hash_delete_no_args();
	test_hash_clear_populated();
	test_hash_list_entries();
	test_hash_invalid_option();
	test_hash_mutual_exclusion();
	test_hash_preset_wrong_argc();
	test_hash_preset_preserves_hits();
	test_hash_resolve_name();
	test_hash_resolve_not_found();
	test_hash_double_dash();
	test_hash_delete_multiple();
	test_hash_clear_then_resolve();
	test_hash_combined_flags_rejected();
}
