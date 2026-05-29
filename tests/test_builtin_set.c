/**
 * @file test_builtin_set.c
 * @brief Unit tests for the 42sh set builtin.
 */

#include "minunit.h"
#include "builtins.h"
#include "variables.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief Initialize a fresh shell for testing
 */
static void	set_test_init(t_shell *shell)
{
	memset(shell, 0, sizeof(*shell));
	shell->env_dirty = 1;
}

/**
 * @brief Test set with no arguments (display all variables)
 */
static void	test_set_no_args(void)
{
	t_shell	shell = {0};
	int		ret;
	char	*argv[] = {"set", NULL};

	set_test_init(&shell);
	var_set(&shell, "TEST_VAR1", "value1");
	var_set(&shell, "TEST_VAR2", "value2");

	ret = builtin_set(&shell, 1, argv);
	MU_ASSERT_INT(0, ret);
}

/**
 * @brief Test set with a single variable assignment
 */
static void	test_set_single_assignment(void)
{
	t_shell	shell = {0};
	int		ret;
	char	*argv[] = {"set", "MYVAR=hello", NULL};
	char	*value;

	set_test_init(&shell);
	ret = builtin_set(&shell, 2, argv);
	value = var_get_value(&shell, "MYVAR");

	MU_ASSERT_INT(0, ret);
	MU_ASSERT_STR("should be hello","hello", value);
	MU_ASSERT_INT(0, shell.last_exit_status);
}

/**
 * @brief Test set with multiple variable assignments
 */
static void	test_set_multiple_assignments(void)
{
	t_shell	shell = {0};
	int		ret;
	char	*argv[] = {"set", "VAR1=value1", "VAR2=value2", "VAR3=value3", NULL};

	set_test_init(&shell);
	ret = builtin_set(&shell, 4, argv);

	MU_ASSERT_INT(0, ret);
	MU_ASSERT_STR("should be value1","value1", var_get_value(&shell, "VAR1"));
	MU_ASSERT_STR("should be value2","value2", var_get_value(&shell, "VAR2"));
	MU_ASSERT_STR("should be value3","value3", var_get_value(&shell, "VAR3"));
}

/**
 * @brief Test set with empty value assignment
 */
static void	test_set_empty_value(void)
{
	t_shell	shell = {0};
	int		ret;
	char	*argv[] = {"set", "EMPTY_VAR=", NULL};
	char	*value;

	set_test_init(&shell);
	ret = builtin_set(&shell, 2, argv);
	value = var_get_value(&shell, "EMPTY_VAR");

	MU_ASSERT_INT(0, ret);
	MU_ASSERT_STR("should be empty","", value);
}

/**
 * @brief Test set overwriting existing variable
 */
static void	test_set_overwrite(void)
{
	t_shell	shell = {0};
	int		ret;
	char	*argv[] = {"set", "VAR=newvalue", NULL};
	char	*value;

	set_test_init(&shell);
	var_set(&shell, "VAR", "oldvalue");
	ret = builtin_set(&shell, 2, argv);
	value = var_get_value(&shell, "VAR");

	MU_ASSERT_INT(0, ret);
	MU_ASSERT_STR("should be newvalue","newvalue", value);
}

/**
 * @brief Test set with value containing special characters
 */
static void	test_set_special_chars(void)
{
	t_shell	shell = {0};
	int		ret;
	char	*argv[] = {"set", "PATH=/usr/bin:/bin", "DESC=hello world!", NULL};

	set_test_init(&shell);
	ret = builtin_set(&shell, 3, argv);

	MU_ASSERT_INT(0, ret);
	MU_ASSERT_STR("should be /usr/bin:/bin","/usr/bin:/bin", var_get_value(&shell, "PATH"));
	MU_ASSERT_STR("should be hello world!","hello world!", var_get_value(&shell, "DESC"));
}

/**
 * @brief Test set with invalid variable name (starting with digit)
 */
static void	test_set_invalid_name(void)
{
	t_shell	shell = {0};
	int		ret;
	char	*argv[] = {"set", "2INVALID=value", NULL};

	set_test_init(&shell);
	ret = builtin_set(&shell, 2, argv);

	MU_ASSERT_INT(1, ret);
	MU_ASSERT_INT(1, shell.last_exit_status);
}

/**
 * @brief Test set with NULL shell
 */
static void	test_set_null_shell(void)
{
	int		ret;
	char	*argv[] = {"set", "VAR=value", NULL};

	ret = builtin_set(NULL, 2, argv);
	MU_ASSERT_INT(1, ret);
}

/**
 * @brief Test set with NULL argv
 */
static void	test_set_null_argv(void)
{
	t_shell	shell = {0};
	int		ret;

	set_test_init(&shell);
	ret = builtin_set(&shell, 2, NULL);
	MU_ASSERT_INT(1, ret);
}

/**
 * @brief Run the full test suite for the set builtin
 */
void	test_builtin_set_suite(void)
{
	test_set_no_args();
	test_set_single_assignment();
	test_set_multiple_assignments();
	test_set_empty_value();
	test_set_overwrite();
	test_set_special_chars();
	test_set_invalid_name();
	test_set_null_shell();
	test_set_null_argv();
}
