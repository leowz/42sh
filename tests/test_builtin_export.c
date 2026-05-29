/**
 * @file test_builtin_export.c
 * @brief Unit tests for the 42sh export builtin.
 */

#include "minunit.h"
#include "builtins.h"
#include "variables.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * @brief Initialize a fresh shell for testing
 */
static void	export_test_init(t_shell *shell)
{
	memset(shell, 0, sizeof(*shell));
	shell->env_dirty = 1;
}

/**
 * @brief Run builtin_export capturing one fd (1 = stdout, 2 = stderr).
 * @return The builtin's return value; `buf` receives the captured output.
 */
static int	capture_export_fd(t_shell *shell, int argc, char **argv,
		char *buf, size_t buf_size, int which_fd)
{
	int		pipefd[2];
	int		saved;
	int		ret;
	ssize_t	n;
	ssize_t	total;

	total = 0;
	memset(buf, 0, buf_size);
	if (pipe(pipefd) == -1)
		return (-1);
	saved = dup(which_fd);
	dup2(pipefd[1], which_fd);
	close(pipefd[1]);
	ret = builtin_export(shell, argc, argv);
	dup2(saved, which_fd);
	close(saved);
	while ((n = read(pipefd[0], buf + total, buf_size - 1 - total)) > 0)
		total += n;
	buf[total] = '\0';
	close(pipefd[0]);
	return (ret);
}

/**
 * @brief Test export with no arguments (display exported variables)
 */
static void	test_export_no_args(void)
{
	t_shell	shell = {0};
	int		ret;
	char	*argv[] = {"export", NULL};

	export_test_init(&shell);
	var_set(&shell, "VAR1", "value1");
	var_export(&shell, "VAR1");
	var_set(&shell, "VAR2", "value2");

	ret = builtin_export(&shell, 1, argv);
	MU_ASSERT_INT(0, ret);
}

/**
 * @brief Test export with existing variable name only
 */
static void	test_export_existing_var(void)
{
	t_shell	shell = {0};
	int		ret;
	char	*argv[] = {"export", "MYVAR", NULL};
	t_var	*var;

	export_test_init(&shell);
	var_set(&shell, "MYVAR", "value");
	ret = builtin_export(&shell, 2, argv);
	var = var_get(&shell, "MYVAR");

	MU_ASSERT_INT(0, ret);
	MU_ASSERT_INT(1, var->exported);
}

/**
 * @brief Test export with assignment (set and export)
 */
static void	test_export_assignment(void)
{
	t_shell	shell = {0};
	int		ret;
	char	*argv[] = {"export", "NEWVAR=newvalue", NULL};
	t_var	*var;
	char	*value;

	export_test_init(&shell);
	ret = builtin_export(&shell, 2, argv);
	var = var_get(&shell, "NEWVAR");
	value = var_get_value(&shell, "NEWVAR");

	MU_ASSERT_INT(0, ret);
	MU_ASSERT_STR("should be newvalue","newvalue", value);
	MU_ASSERT_INT(1, var->exported);
}

/**
 * @brief Test export with multiple assignments
 */
static void	test_export_multiple_assignments(void)
{
	t_shell	shell = {0};
	int		ret;
	char	*argv[] = {"export", "VAR1=val1", "VAR2=val2", "VAR3=val3", NULL};

	export_test_init(&shell);
	ret = builtin_export(&shell, 4, argv);

	MU_ASSERT_INT(0, ret);
	MU_ASSERT_INT(1, var_get(&shell, "VAR1")->exported);
	MU_ASSERT_INT(1, var_get(&shell, "VAR2")->exported);
	MU_ASSERT_INT(1, var_get(&shell, "VAR3")->exported);
}

/**
 * @brief Test export with mixed assignments and variable names
 */
static void	test_export_mixed(void)
{
	t_shell	shell = {0};
	int		ret;
	char	*argv[] = {"export", "EXISTING=newval", "PATH", NULL};

	export_test_init(&shell);
	var_set(&shell, "PATH", "/usr/bin");
	ret = builtin_export(&shell, 3, argv);

	MU_ASSERT_INT(0, ret);
	MU_ASSERT_INT(1, var_get(&shell, "EXISTING")->exported);
	MU_ASSERT_INT(1, var_get(&shell, "PATH")->exported);
}

/**
 * @brief Test export with empty value assignment
 */
static void	test_export_empty_value(void)
{
	t_shell	shell = {0};
	int		ret;
	char	*argv[] = {"export", "EMPTY_VAR=", NULL};
	t_var	*var;

	export_test_init(&shell);
	ret = builtin_export(&shell, 2, argv);
	var = var_get(&shell, "EMPTY_VAR");

	MU_ASSERT_INT(0, ret);
	MU_ASSERT_STR("should be empty","", var_get_value(&shell, "EMPTY_VAR"));
	MU_ASSERT_INT(1, var->exported);
}

/**
 * @brief Test export with invalid identifier (starting with digit)
 */
static void	test_export_invalid_digit(void)
{
	t_shell	shell = {0};
	int		ret;
	char	*argv[] = {"export", "2INVALID", NULL};

	export_test_init(&shell);
	ret = builtin_export(&shell, 2, argv);

	MU_ASSERT_INT(1, ret);
}

/**
 * @brief Test export with invalid assignment (digit start)
 */
static void	test_export_invalid_assign(void)
{
	t_shell	shell = {0};
	int		ret;
	char	*argv[] = {"export", "2INVALID=value", NULL};

	export_test_init(&shell);
	ret = builtin_export(&shell, 2, argv);

	MU_ASSERT_INT(1, ret);
}

/**
 * @brief Test export handling of equals in value
 */
static void	test_export_equals_in_value(void)
{
	t_shell	shell = {0};
	int		ret;
	char	*argv[] = {"export", "CONFIG=key=value", NULL};

	export_test_init(&shell);
	ret = builtin_export(&shell, 2, argv);

	MU_ASSERT_INT(0, ret);
	MU_ASSERT_STR("should be key=value","key=value", var_get_value(&shell, "CONFIG"));
}

/**
 * @brief Test export with special characters
 */
static void	test_export_special_chars(void)
{
	t_shell	shell = {0};
	int		ret;
	char	*argv[] = {"export", "PATH=/usr/bin:/bin", "_PRIVATE=secret", NULL};

	export_test_init(&shell);
	ret = builtin_export(&shell, 3, argv);

	MU_ASSERT_INT(0, ret);
	MU_ASSERT_STR("should be /usr/bin:/bin","/usr/bin:/bin", var_get_value(&shell, "PATH"));
	MU_ASSERT_STR("should be secret","secret", var_get_value(&shell, "_PRIVATE"));
}

/**
 * @brief Test export marks non-exported variable as exported
 */
static void	test_export_marks_as_exported(void)
{
	t_shell	shell = {0};
	int		ret;
	char	*argv[] = {"export", "UNEXPORTED", NULL};
	t_var	*var;

	export_test_init(&shell);
	var_set(&shell, "UNEXPORTED", "value");
	var = var_get(&shell, "UNEXPORTED");
	MU_ASSERT_INT(0, var->exported);

	ret = builtin_export(&shell, 2, argv);
	var = var_get(&shell, "UNEXPORTED");
	MU_ASSERT_INT(0, ret);
	MU_ASSERT_INT(1, var->exported);
}

/**
 * @brief Test export with NULL shell
 */
static void	test_export_null_shell(void)
{
	int		ret;
	char	*argv[] = {"export", "VAR=value", NULL};

	ret = builtin_export(NULL, 2, argv);
	MU_ASSERT_INT(1, ret);
}

/**
 * @brief Test export with NULL argv
 */
static void	test_export_null_argv(void)
{
	t_shell	shell = {0};
	int		ret;

	export_test_init(&shell);
	ret = builtin_export(&shell, 2, NULL);
	MU_ASSERT_INT(1, ret);
}

/**
 * @brief Test `export -p` lists exported variables (subject: export [-p]).
 */
static void	test_export_p_lists_exported(void)
{
	t_shell	shell = {0};
	char	buf[1024];
	int		ret;
	char	*argv[] = {"export", "-p", NULL};

	export_test_init(&shell);
	var_set(&shell, "EXP_P_VAR", "pval");
	var_export(&shell, "EXP_P_VAR");
	ret = capture_export_fd(&shell, 2, argv, buf, sizeof(buf), 1);
	MU_ASSERT_INT(0, ret);
	MU_ASSERT("export -p lists exported variables",
		strstr(buf, "export EXP_P_VAR=pval") != NULL);
}

/**
 * @brief Test the invalid-name error echoes the rejected token.
 */
static void	test_export_invalid_name_echoes_token(void)
{
	t_shell	shell = {0};
	char	buf[256];
	char	*argv[] = {"export", "2BADNAME", NULL};

	export_test_init(&shell);
	capture_export_fd(&shell, 2, argv, buf, sizeof(buf), 2);
	MU_ASSERT("export error names the rejected token",
		strstr(buf, "2BADNAME") != NULL);
}

/**
 * @brief Test the invalid-assignment error echoes the rejected token.
 */
static void	test_export_invalid_assign_echoes_token(void)
{
	t_shell	shell = {0};
	char	buf[256];
	char	*argv[] = {"export", "1BAD=value", NULL};

	export_test_init(&shell);
	capture_export_fd(&shell, 2, argv, buf, sizeof(buf), 2);
	MU_ASSERT("export assignment error names the rejected token",
		strstr(buf, "1BAD") != NULL);
}

/**
 * @brief Run the full test suite for the export builtin
 */
void	test_builtin_export_suite(void)
{
	test_export_no_args();
	test_export_existing_var();
	test_export_assignment();
	test_export_multiple_assignments();
	test_export_mixed();
	test_export_empty_value();
	test_export_invalid_digit();
	test_export_invalid_assign();
	test_export_equals_in_value();
	test_export_special_chars();
	test_export_marks_as_exported();
	test_export_null_shell();
	test_export_null_argv();
	test_export_p_lists_exported();
	test_export_invalid_name_echoes_token();
	test_export_invalid_assign_echoes_token();
}
