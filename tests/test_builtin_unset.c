/**
 * @file test_builtin_unset.c
 * @brief Unit tests for the 42sh unset builtin.
 */

#include "minunit.h"
#include "builtins.h"
#include "variables.h"
#include <stdlib.h>
#include <string.h>

/**
 * @brief Initialize a fresh shell for testing
 */
static void	unset_test_init(t_shell *shell)
{
	memset(shell, 0, sizeof(*shell));
	shell->env_dirty = 1;
}

/**
 * @brief Test unset with no arguments (missing operand)
 */
static void	test_unset_no_args(void)
{
	t_shell	shell = {0};
	int		ret;
	char	*argv[] = {"unset", NULL};

	unset_test_init(&shell);
	ret = builtin_unset(&shell, 1, argv);

	MU_ASSERT_INT(1, ret);
	MU_ASSERT_INT(1, shell.last_exit_status);
}

/**
 * @brief Test unset with a single variable
 */
static void	test_unset_single_var(void)
{
	t_shell	shell = {0};
	int		ret;
	char	*argv[] = {"unset", "MYVAR", NULL};
	char	*value;

	unset_test_init(&shell);
	var_set(&shell, "MYVAR", "value");
	ret = builtin_unset(&shell, 2, argv);
	value = var_get_value(&shell, "MYVAR");

	MU_ASSERT_INT(0, ret);
	MU_ASSERT_INT(0, value != NULL);
}

/**
 * @brief Test unset with multiple variables
 */
static void	test_unset_multiple_vars(void)
{
	t_shell	shell = {0};
	int		ret;
	char	*argv[] = {"unset", "VAR1", "VAR2", "VAR3", NULL};

	unset_test_init(&shell);
	var_set(&shell, "VAR1", "value1");
	var_set(&shell, "VAR2", "value2");
	var_set(&shell, "VAR3", "value3");

	ret = builtin_unset(&shell, 4, argv);

	MU_ASSERT_INT(0, ret);
	MU_ASSERT_INT(0, var_get_value(&shell, "VAR1") != NULL);
	MU_ASSERT_INT(0, var_get_value(&shell, "VAR2") != NULL);
	MU_ASSERT_INT(0, var_get_value(&shell, "VAR3") != NULL);
}

/**
 * @brief Test unset with non-existent variable (should succeed silently)
 */
static void	test_unset_nonexistent(void)
{
	t_shell	shell = {0};
	int		ret;
	char	*argv[] = {"unset", "DOES_NOT_EXIST", NULL};

	unset_test_init(&shell);
	ret = builtin_unset(&shell, 2, argv);

	MU_ASSERT_INT(0, ret);
	MU_ASSERT_INT(0, shell.last_exit_status);
}

/**
 * @brief Test unset with invalid identifier (starting with digit)
 */
static void	test_unset_invalid_digit(void)
{
	t_shell	shell = {0};
	int		ret;
	char	*argv[] = {"unset", "2INVALID", NULL};

	unset_test_init(&shell);
	ret = builtin_unset(&shell, 2, argv);

	MU_ASSERT_INT(1, ret);
	MU_ASSERT_INT(1, shell.last_exit_status);
}

/**
 * @brief Test unset with invalid identifier (containing special character)
 */
static void	test_unset_invalid_special(void)
{
	t_shell	shell = {0};
	int		ret;
	char	*argv[] = {"unset", "VAR-NAME", NULL};

	unset_test_init(&shell);
	ret = builtin_unset(&shell, 2, argv);

	MU_ASSERT_INT(1, ret);
}

/**
 * @brief Test unset with mixed valid and invalid identifiers
 */
static void	test_unset_mixed_valid_invalid(void)
{
	t_shell	shell = {0};
	int		ret;
	char	*argv[] = {"unset", "VALID_VAR", "2INVALID", NULL};

	unset_test_init(&shell);
	var_set(&shell, "VALID_VAR", "value");
	ret = builtin_unset(&shell, 3, argv);

	MU_ASSERT_INT(1, ret);
}

/**
 * @brief Test unset with empty string identifier
 */
static void	test_unset_empty_identifier(void)
{
	t_shell	shell = {0};
	int		ret;
	char	*argv[] = {"unset", "", NULL};

	unset_test_init(&shell);
	ret = builtin_unset(&shell, 2, argv);

	MU_ASSERT_INT(1, ret);
}

/**
 * @brief Test unset with underscore identifier
 */
static void	test_unset_underscore(void)
{
	t_shell	shell = {0};
	int		ret;
	char	*argv[] = {"unset", "_VAR", NULL};

	unset_test_init(&shell);
	var_set(&shell, "_VAR", "value");
	ret = builtin_unset(&shell, 2, argv);

	MU_ASSERT_INT(0, ret);
	MU_ASSERT_INT(0, var_get_value(&shell, "_VAR") != NULL);
}

/**
 * @brief Test unset with NULL shell
 */
static void	test_unset_null_shell(void)
{
	int		ret;
	char	*argv[] = {"unset", "VAR", NULL};

	ret = builtin_unset(NULL, 2, argv);
	MU_ASSERT_INT(1, ret);
}

/**
 * @brief Test unset with NULL argv
 */
static void	test_unset_null_argv(void)
{
	t_shell	shell = {0};
	int		ret;

	unset_test_init(&shell);
	ret = builtin_unset(&shell, 2, NULL);
	MU_ASSERT_INT(1, ret);
}

/**
 * @brief Run the full test suite for the unset builtin
 */
void	test_builtin_unset_suite(void)
{
	test_unset_no_args();
	test_unset_single_var();
	test_unset_multiple_vars();
	test_unset_nonexistent();
	test_unset_invalid_digit();
	test_unset_invalid_special();
	test_unset_mixed_valid_invalid();
	test_unset_empty_identifier();
	test_unset_underscore();
	test_unset_null_shell();
	test_unset_null_argv();
}
