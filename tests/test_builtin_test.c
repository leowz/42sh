// filepath: /home/josh/Desktop/42sh/tests/test_builtin_test.c
/**
 * @file test_builtin_test.c
 * @brief Test suite for the test builtin command.
 * @author pulgamecanica
 *
 * Tests cover:
 *   - File tests: -e, -f, -d, -b, -c, -r, -w, -x, -s
 *   - String tests: -z, -n
 *   - String comparisons: =, !=, <, >
 *   - Numeric comparisons: -eq, -ne, -lt, -le, -gt, -ge
 *   - Error cases: missing arguments, invalid operators
 */


# include "minunit.h"
# include "../includes/42sh.h"
# include "../includes/builtins.h"
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
# include <fcntl.h>
# include <sys/stat.h>
# include <sys/types.h>

extern void	stub_shell_init(t_shell *shell);
extern void	stub_shell_cleanup(t_shell *shell);

static char	*create_temp_file(const char *content);
static char	*create_temp_dir(void);
static void	cleanup_temp_file(char *path);
static void	cleanup_temp_dir(char *path);

/* ===== Helper: Create temporary test files ============================= */
/* ===== Comparison tests with bash ===================================== */
// 
static int	bash_test(const char *cmd)
{
    int	status;
// 
    status = system(cmd);
    return (WIFEXITED(status) ? WEXITSTATUS(status) : -1);
}
// 
static void	test_compare_bash_file_exists(void)
{
    t_shell	shell;
    char	*file;
    char	*argv[] = {"test", "-e", NULL, NULL};
    char	bash_cmd[256];
    int		shell_result;
    int		bash_result;
// 
    stub_shell_init(&shell);
    file = create_temp_file("content");
    if (!file)
        return ;
    argv[2] = file;
    shell_result = builtin_test(&shell, 3, argv);
    snprintf(bash_cmd, sizeof(bash_cmd), "test -e %s", file);
    bash_result = bash_test(bash_cmd);
    MU_ASSERT("shell and bash -e match", shell_result == bash_result);
    cleanup_temp_file(file);
    stub_shell_cleanup(&shell);
}
// 
static void	test_compare_bash_file_regular(void)
{
    t_shell	shell;
    char	*file;
    char	*argv[] = {"test", "-f", NULL, NULL};
    char	bash_cmd[256];
    int		shell_result;
    int		bash_result;
// 
    stub_shell_init(&shell);
    file = create_temp_file("test");
    if (!file)
        return ;
    argv[2] = file;
    shell_result = builtin_test(&shell, 3, argv);
    snprintf(bash_cmd, sizeof(bash_cmd), "test -f %s", file);
    bash_result = bash_test(bash_cmd);
    MU_ASSERT("shell and bash -f match", shell_result == bash_result);
    cleanup_temp_file(file);
    stub_shell_cleanup(&shell);
}
// 
static void	test_compare_bash_file_directory(void)
{
    t_shell	shell;
    char	*dir;
    char	*argv[] = {"test", "-d", NULL, NULL};
    char	bash_cmd[256];
    int		shell_result;
    int		bash_result;
// 
    stub_shell_init(&shell);
    dir = create_temp_dir();
    if (!dir)
        return ;
    argv[2] = dir;
    shell_result = builtin_test(&shell, 3, argv);
    snprintf(bash_cmd, sizeof(bash_cmd), "test -d %s", dir);
    bash_result = bash_test(bash_cmd);
    MU_ASSERT("shell and bash -d match", shell_result == bash_result);
    cleanup_temp_dir(dir);
    stub_shell_cleanup(&shell);
}
// 
static void	test_compare_bash_string_equal(void)
{
    t_shell	shell;
    char	*argv[] = {"test", "hello", "=", "hello", NULL};
    char	bash_cmd[256];
    int		shell_result;
    int		bash_result;
// 
    stub_shell_init(&shell);
    shell_result = builtin_test(&shell, 4, argv);
    snprintf(bash_cmd, sizeof(bash_cmd), "test 'hello' = 'hello'");
    bash_result = bash_test(bash_cmd);
    MU_ASSERT("shell and bash string = match", shell_result == bash_result);
    stub_shell_cleanup(&shell);
}
// 
static void	test_compare_bash_numeric_equal(void)
{
    t_shell	shell;
    char	*argv[] = {"test", "42", "-eq", "42", NULL};
    int		shell_result;
    int		bash_result;
// 
    stub_shell_init(&shell);
    shell_result = builtin_test(&shell, 4, argv);
    bash_result = bash_test("test 42 -eq 42");
    MU_ASSERT("shell and bash -eq match", shell_result == bash_result);
    stub_shell_cleanup(&shell);
}
// 
static void	test_compare_bash_numeric_less_than(void)
{
    t_shell	shell;
    char	*argv[] = {"test", "10", "-lt", "20", NULL};
    int		shell_result;
    int		bash_result;
// 
    stub_shell_init(&shell);
    shell_result = builtin_test(&shell, 4, argv);
    bash_result = bash_test("test 10 -lt 20");
    MU_ASSERT("shell and bash -lt match", shell_result == bash_result);
    stub_shell_cleanup(&shell);
}
// 
static void	test_compare_bash_string_not_empty(void)
{
    t_shell	shell;
    char	*argv[] = {"test", "-n", "hello", NULL};
    int		shell_result;
    int		bash_result;
// 
    stub_shell_init(&shell);
    shell_result = builtin_test(&shell, 3, argv);
    bash_result = bash_test("test -n 'hello'");
    MU_ASSERT("shell and bash -n match", shell_result == bash_result);
    stub_shell_cleanup(&shell);
}
// 
static void	test_compare_bash_string_empty(void)
{
    t_shell	shell;
    char	*argv[] = {"test", "-z", "", NULL};
    int		shell_result;
    int		bash_result;
// 
    stub_shell_init(&shell);
    shell_result = builtin_test(&shell, 3, argv);
    bash_result = bash_test("test -z ''");
    MU_ASSERT("shell and bash -z match", shell_result == bash_result);
    stub_shell_cleanup(&shell);
}
static char	*create_temp_file(const char *content)
{
    char	*path;
    int		fd;
    int		len;

    path = malloc(64);
    strcpy(path, "/tmp/42sh_test_XXXXXX");
    fd = mkstemp(path);
    if (fd < 0)
    {
        free(path);
        return (NULL);
    }
    if (content)
    {
        len = strlen(content);
        write(fd, content, len);
    }
    close(fd);
    return (path);
}

static char	*create_temp_dir(void)
{
    char	*path;

    path = malloc(64);
    strcpy(path, "/tmp/42sh_test_dir_XXXXXX");
    if (!mkdtemp(path))
    {
        free(path);
        return (NULL);
    }
    return (path);
}

static void	cleanup_temp_file(char *path)
{
    if (path)
    {
        unlink(path);
        free(path);
    }
}

static void	cleanup_temp_dir(char *path)
{
    if (path)
    {
        rmdir(path);
        free(path);
    }
}

/* ===== File existence tests (-e) ======================================= */

static void	test_file_exists_true(void)
{
    t_shell	shell;
    char	*file;
    char	*argv[] = {"test", "-e", NULL, NULL};
    int		result;

    stub_shell_init(&shell);
    file = create_temp_file("content");
    if (!file)
        return ;
    argv[2] = file;
    result = builtin_test(&shell, 3, argv);
    MU_ASSERT("file exists returns 0", result == 0);
    MU_ASSERT("exit status 0", shell.last_exit_status == 0);
    cleanup_temp_file(file);
    stub_shell_cleanup(&shell);
}

static void	test_file_exists_false(void)
{
    t_shell	shell;
    char	*argv[] = {"test", "-e", "/nonexistent/path/file", NULL};
    int		result;

    stub_shell_init(&shell);
    result = builtin_test(&shell, 3, argv);
    MU_ASSERT("nonexistent file returns 1", result == 1);
    MU_ASSERT("exit status 1", shell.last_exit_status == 1);
    stub_shell_cleanup(&shell);
}

/* ===== File type tests (-f, -d) ======================================== */

static void	test_file_is_regular(void)
{
    t_shell	shell;
    char	*file;
    char	*argv[] = {"test", "-f", NULL, NULL};
    int		result;

    stub_shell_init(&shell);
    file = create_temp_file("test data");
    if (!file)
        return ;
    argv[2] = file;
    result = builtin_test(&shell, 3, argv);
    MU_ASSERT("regular file test true", result == 0);
    cleanup_temp_file(file);
    stub_shell_cleanup(&shell);
}

static void	test_file_is_directory(void)
{
    t_shell	shell;
    char	*dir;
    char	*argv[] = {"test", "-d", NULL, NULL};
    int		result;

    stub_shell_init(&shell);
    dir = create_temp_dir();
    if (!dir)
        return ;
    argv[2] = dir;
    result = builtin_test(&shell, 3, argv);
    MU_ASSERT("directory test true", result == 0);
    cleanup_temp_dir(dir);
    stub_shell_cleanup(&shell);
}

static void	test_directory_is_not_regular(void)
{
    t_shell	shell;
    char	*dir;
    char	*argv[] = {"test", "-f", NULL, NULL};
    int		result;

    stub_shell_init(&shell);
    dir = create_temp_dir();
    if (!dir)
        return ;
    argv[2] = dir;
    result = builtin_test(&shell, 3, argv);
    MU_ASSERT("directory is not regular file", result == 1);
    cleanup_temp_dir(dir);
    stub_shell_cleanup(&shell);
}

/* ===== File permissions tests (-r, -w, -x) ============================ */

static void	test_file_readable(void)
{
    t_shell	shell;
    char	*file;
    char	*argv[] = {"test", "-r", NULL, NULL};
    int		result;

    stub_shell_init(&shell);
    file = create_temp_file("readable");
    if (!file)
        return ;
    chmod(file, S_IRUSR);
    argv[2] = file;
    result = builtin_test(&shell, 3, argv);
    MU_ASSERT("readable file test true", result == 0);
    cleanup_temp_file(file);
    stub_shell_cleanup(&shell);
}

static void	test_file_writable(void)
{
    t_shell	shell;
    char	*file;
    char	*argv[] = {"test", "-w", NULL, NULL};
    int		result;

    stub_shell_init(&shell);
    file = create_temp_file("writable");
    if (!file)
        return ;
    chmod(file, S_IWUSR);
    argv[2] = file;
    result = builtin_test(&shell, 3, argv);
    MU_ASSERT("writable file test true", result == 0);
    cleanup_temp_file(file);
    stub_shell_cleanup(&shell);
}

/* ===== File size test (-s) ============================================= */

static void	test_file_has_size(void)
{
    t_shell	shell;
    char	*file;
    char	*argv[] = {"test", "-s", NULL, NULL};
    int		result;

    stub_shell_init(&shell);
    file = create_temp_file("content");
    if (!file)
        return ;
    argv[2] = file;
    result = builtin_test(&shell, 3, argv);
    MU_ASSERT("file with size returns 0", result == 0);
    cleanup_temp_file(file);
    stub_shell_cleanup(&shell);
}

static void	test_empty_file_no_size(void)
{
    t_shell	shell;
    char	*file;
    char	*argv[] = {"test", "-s", NULL, NULL};
    int		result;

    stub_shell_init(&shell);
    file = create_temp_file("");
    if (!file)
        return ;
    argv[2] = file;
    result = builtin_test(&shell, 3, argv);
    MU_ASSERT("empty file size test returns 1", result == 1);
    cleanup_temp_file(file);
    stub_shell_cleanup(&shell);
}

/* ===== String tests (-z, -n) ========================================== */

static void	test_string_zero_empty(void)
{
    t_shell	shell;
    char	*argv[] = {"test", "-z", "", NULL};
    int		result;

    stub_shell_init(&shell);
    result = builtin_test(&shell, 3, argv);
    MU_ASSERT("empty string is zero true", result == 0);
    stub_shell_cleanup(&shell);
}

static void	test_string_zero_nonempty(void)
{
    t_shell	shell;
    char	*argv[] = {"test", "-z", "hello", NULL};
    int		result;

    stub_shell_init(&shell);
    result = builtin_test(&shell, 3, argv);
    MU_ASSERT("nonempty string is zero false", result == 1);
    stub_shell_cleanup(&shell);
}

static void	test_string_n_nonempty(void)
{
    t_shell	shell;
    char	*argv[] = {"test", "-n", "hello", NULL};
    int		result;

    stub_shell_init(&shell);
    result = builtin_test(&shell, 3, argv);
    MU_ASSERT("nonempty string is n true", result == 0);
    stub_shell_cleanup(&shell);
}

static void	test_string_n_empty(void)
{
    t_shell	shell;
    char	*argv[] = {"test", "-n", "", NULL};
    int		result;

    stub_shell_init(&shell);
    result = builtin_test(&shell, 3, argv);
    MU_ASSERT("empty string is n false", result == 1);
    stub_shell_cleanup(&shell);
}

/* ===== String comparisons (=, !=) ===================================== */

static void	test_string_equal(void)
{
    t_shell	shell;
    char	*argv[] = {"test", "hello", "=", "hello", NULL};
    int		result;

    stub_shell_init(&shell);
    result = builtin_test(&shell, 4, argv);
    MU_ASSERT("equal strings true", result == 0);
    stub_shell_cleanup(&shell);
}

static void	test_string_not_equal_op(void)
{
    t_shell	shell;
    char	*argv[] = {"test", "hello", "!=", "world", NULL};
    int		result;

    stub_shell_init(&shell);
    result = builtin_test(&shell, 4, argv);
    MU_ASSERT("not equal strings true", result == 0);
    stub_shell_cleanup(&shell);
}

static void	test_string_equal_fails(void)
{
    t_shell	shell;
    char	*argv[] = {"test", "hello", "=", "world", NULL};
    int		result;

    stub_shell_init(&shell);
    result = builtin_test(&shell, 4, argv);
    MU_ASSERT("not equal strings false", result == 1);
    stub_shell_cleanup(&shell);
}

/* ===== Lexicographic comparisons (<, >) =============================== */

static void	test_string_less_than(void)
{
    t_shell	shell;
    char	*argv[] = {"test", "abc", "<", "def", NULL};
    int		result;

    stub_shell_init(&shell);
    result = builtin_test(&shell, 4, argv);
    MU_ASSERT("abc < def true", result == 0);
    stub_shell_cleanup(&shell);
}

static void	test_string_greater_than(void)
{
    t_shell	shell;
    char	*argv[] = {"test", "xyz", ">", "abc", NULL};
    int		result;

    stub_shell_init(&shell);
    result = builtin_test(&shell, 4, argv);
    MU_ASSERT("xyz > abc true", result == 0);
    stub_shell_cleanup(&shell);
}

static void	test_string_not_less_than(void)
{
    t_shell	shell;
    char	*argv[] = {"test", "xyz", "<", "abc", NULL};
    int		result;

    stub_shell_init(&shell);
    result = builtin_test(&shell, 4, argv);
    MU_ASSERT("xyz < abc false", result == 1);
    stub_shell_cleanup(&shell);
}

/* ===== Numeric comparisons (-eq, -ne, -lt, etc.) ====================== */

static void	test_numeric_equal(void)
{
    t_shell	shell;
    char	*argv[] = {"test", "42", "-eq", "42", NULL};
    int		result;

    stub_shell_init(&shell);
    result = builtin_test(&shell, 4, argv);
    MU_ASSERT("42 -eq 42 true", result == 0);
    stub_shell_cleanup(&shell);
}

static void	test_numeric_not_equal(void)
{
    t_shell	shell;
    char	*argv[] = {"test", "42", "-ne", "13", NULL};
    int		result;

    stub_shell_init(&shell);
    result = builtin_test(&shell, 4, argv);
    MU_ASSERT("42 -ne 13 true", result == 0);
    stub_shell_cleanup(&shell);
}

static void	test_numeric_less_than(void)
{
    t_shell	shell;
    char	*argv[] = {"test", "10", "-lt", "20", NULL};
    int		result;

    stub_shell_init(&shell);
    result = builtin_test(&shell, 4, argv);
    MU_ASSERT("10 -lt 20 true", result == 0);
    stub_shell_cleanup(&shell);
}

static void	test_numeric_less_or_equal(void)
{
    t_shell	shell;
    char	*argv[] = {"test", "20", "-le", "20", NULL};
    int		result;

    stub_shell_init(&shell);
    result = builtin_test(&shell, 4, argv);
    MU_ASSERT("20 -le 20 true", result == 0);
    stub_shell_cleanup(&shell);
}

static void	test_numeric_greater_than(void)
{
    t_shell	shell;
    char	*argv[] = {"test", "30", "-gt", "20", NULL};
    int		result;

    stub_shell_init(&shell);
    result = builtin_test(&shell, 4, argv);
    MU_ASSERT("30 -gt 20 true", result == 0);
    stub_shell_cleanup(&shell);
}

static void	test_numeric_greater_or_equal(void)
{
    t_shell	shell;
    char	*argv[] = {"test", "20", "-ge", "20", NULL};
    int		result;

    stub_shell_init(&shell);
    result = builtin_test(&shell, 4, argv);
    MU_ASSERT("20 -ge 20 true", result == 0);
    stub_shell_cleanup(&shell);
}

static void	test_numeric_invalid_argument(void)
{
    t_shell	shell;
    char	*argv[] = {"test", "abc", "-eq", "42", NULL};
    int		result;

    stub_shell_init(&shell);
    result = builtin_test(&shell, 4, argv);
    MU_ASSERT("non-numeric argument returns 1", result == 1);
    stub_shell_cleanup(&shell);
}

/* ===== Error cases: missing arguments ================================= */

static void	test_no_arguments(void)
{
    t_shell	shell;
    char	*argv[] = {"test", NULL};
    int		result;

    stub_shell_init(&shell);
    result = builtin_test(&shell, 1, argv);
    MU_ASSERT("no arguments returns 1", result == 1);
    MU_ASSERT("exit status 1", shell.last_exit_status == 1);
    stub_shell_cleanup(&shell);
}

static void	test_missing_argument_for_flag(void)
{
    t_shell	shell;
    char	*argv[] = {"test", "-f", NULL};
    int		result;

    stub_shell_init(&shell);
    result = builtin_test(&shell, 2, argv);
    MU_ASSERT("missing file arg returns 1", result == 1);
    stub_shell_cleanup(&shell);
}

static void	test_missing_right_operand(void)
{
    t_shell	shell;
    char	*argv[] = {"test", "string", "=", NULL};
    int		result;

    stub_shell_init(&shell);
    result = builtin_test(&shell, 3, argv);
    MU_ASSERT("missing right operand returns 1", result == 1);
    stub_shell_cleanup(&shell);
}

/* ===== Suite registration ============================================== */

void	test_builtin_test_suite(void)
{
    test_file_exists_true();
    test_file_exists_false();
    test_file_is_regular();
    test_file_is_directory();
    test_directory_is_not_regular();
    test_file_readable();
    test_file_writable();
    test_file_has_size();
    test_empty_file_no_size();
    test_string_zero_empty();
    test_string_zero_nonempty();
    test_string_n_nonempty();
    test_string_n_empty();
    test_string_equal();
    test_string_not_equal_op();
    test_string_equal_fails();
    test_string_less_than();
    test_string_greater_than();
    test_string_not_less_than();
    test_numeric_equal();
    test_numeric_not_equal();
    test_numeric_less_than();
    test_numeric_less_or_equal();
    test_numeric_greater_than();
    test_numeric_greater_or_equal();
    test_numeric_invalid_argument();
    test_no_arguments();
    test_missing_argument_for_flag();
    test_missing_right_operand();
    test_compare_bash_file_directory();
    test_compare_bash_file_regular();
    test_compare_bash_file_exists();
    test_compare_bash_string_equal();
    test_compare_bash_numeric_equal();
    test_compare_bash_numeric_less_than();
    test_compare_bash_string_not_empty();
    test_compare_bash_string_empty();
}

