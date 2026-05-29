/**
 * @file test_arithmetic.c
 * @brief Comprehensive test suite for the arithmetic expansion module.
 * @author jguillem
 *
 * Tests cover:
 *   - arith_eval: basic operations, operator precedence, parentheses,
 *     unary minus, nested parentheses, whitespace handling, division
 *     by zero, trailing garbage detection.
 *   - expand_arith (via expand_word): simple $((expr)), variable
 *     substitution inside $((...)), nested $((...$(...) ...)),
 *     arithmetic inside double quotes, chained expressions in one word,
 *     edge cases (zero, negative result, large numbers).
 *   - expand_command: assignment with arithmetic value (X=$((...))),
 *     argv with arithmetic word, redir target with arithmetic.
 *
 * Uses the shared stub_shell_init / stub_shell_cleanup helpers from
 * test_stubs.c and the inject_var helper pattern from test_expander.c.
 */

#ifdef TEST_ARITH_ENABLED

# include "minunit.h"
# include "../includes/42sh.h"
# include "../includes/expander.h"
# include "../includes/ast.h"
# include <stdlib.h>
# include <string.h>

extern void	stub_shell_init(t_shell *shell);
extern void	stub_shell_cleanup(t_shell *shell);

static void	inject_var(t_shell *shell, const char *name, const char *value)
{
	t_var	*var;
	t_list	*node;

	var = malloc(sizeof(t_var));
	var->name = ft_strdup(name);
	var->value = ft_strdup(value);
	var->exported = 0;
	var->readonly = 0;
	node = ft_lstnew(var);
	ft_lstadd(&shell->variables, node);
}

/* ===== arith_eval: basic arithmetic ===================================== */

static void	test_arith_addition(void)
{
	long long int	result;

	MU_ASSERT_INT(0, arith_eval("1 + 2", &result));
	MU_ASSERT_INT(3, (int)result);
	MU_ASSERT_INT(0, arith_eval("0 + 0", &result));
	MU_ASSERT_INT(0, (int)result);
	MU_ASSERT_INT(0, arith_eval("100 + 200", &result));
	MU_ASSERT_INT(300, (int)result);
}

static void	test_arith_subtraction(void)
{
	long long int	result;

	MU_ASSERT_INT(0, arith_eval("5 - 3", &result));
	MU_ASSERT_INT(2, (int)result);
	MU_ASSERT_INT(0, arith_eval("3 - 5", &result));
	MU_ASSERT_INT(-2, (int)result);
	MU_ASSERT_INT(0, arith_eval("0 - 0", &result));
	MU_ASSERT_INT(0, (int)result);
}

static void	test_arith_multiplication(void)
{
	long long int	result;

	MU_ASSERT_INT(0, arith_eval("3 * 4", &result));
	MU_ASSERT_INT(12, (int)result);
	MU_ASSERT_INT(0, arith_eval("0 * 99", &result));
	MU_ASSERT_INT(0, (int)result);
	MU_ASSERT_INT(0, arith_eval("7 * 7", &result));
	MU_ASSERT_INT(49, (int)result);
}

static void	test_arith_division(void)
{
	long long int	result;

	MU_ASSERT_INT(0, arith_eval("10 / 2", &result));
	MU_ASSERT_INT(5, (int)result);
	MU_ASSERT_INT(0, arith_eval("7 / 2", &result));
	MU_ASSERT_INT(3, (int)result);
	MU_ASSERT_INT(0, arith_eval("1 / 1", &result));
	MU_ASSERT_INT(1, (int)result);
}

static void	test_arith_modulo(void)
{
	long long int	result;

	MU_ASSERT_INT(0, arith_eval("10 % 3", &result));
	MU_ASSERT_INT(1, (int)result);
	MU_ASSERT_INT(0, arith_eval("9 % 3", &result));
	MU_ASSERT_INT(0, (int)result);
	MU_ASSERT_INT(0, arith_eval("7 % 4", &result));
	MU_ASSERT_INT(3, (int)result);
}

/* ===== arith_eval: operator precedence ================================== */

static void	test_arith_precedence_mul_over_add(void)
{
	long long int	result;

	MU_ASSERT_INT(0, arith_eval("2 + 3 * 4", &result));
	MU_ASSERT_INT(14, (int)result);
}

static void	test_arith_precedence_div_over_sub(void)
{
	long long int	result;

	MU_ASSERT_INT(0, arith_eval("10 - 6 / 2", &result));
	MU_ASSERT_INT(7, (int)result);
}

static void	test_arith_precedence_mixed(void)
{
	long long int	result;

	MU_ASSERT_INT(0, arith_eval("1 + 2 * 3 - 4 / 2", &result));
	MU_ASSERT_INT(5, (int)result);
}

static void	test_arith_precedence_left_associative(void)
{
	long long int	result;

	MU_ASSERT_INT(0, arith_eval("10 - 3 - 2", &result));
	MU_ASSERT_INT(5, (int)result);
	MU_ASSERT_INT(0, arith_eval("24 / 4 / 2", &result));
	MU_ASSERT_INT(3, (int)result);
}

/* ===== arith_eval: parentheses ========================================== */

static void	test_arith_parens_override_precedence(void)
{
	long long int	result;

	MU_ASSERT_INT(0, arith_eval("(2 + 3) * 4", &result));
	MU_ASSERT_INT(20, (int)result);
}

static void	test_arith_parens_nested(void)
{
	long long int	result;

	MU_ASSERT_INT(0, arith_eval("((2 + 3) * (1 + 1))", &result));
	MU_ASSERT_INT(10, (int)result);
}

static void	test_arith_parens_redundant(void)
{
	long long int	result;

	MU_ASSERT_INT(0, arith_eval("(42)", &result));
	MU_ASSERT_INT(42, (int)result);
	MU_ASSERT_INT(0, arith_eval("((7))", &result));
	MU_ASSERT_INT(7, (int)result);
}

/* ===== arith_eval: unary minus ========================================== */

static void	test_arith_unary_minus(void)
{
	long long int	result;

	MU_ASSERT_INT(0, arith_eval("-5", &result));
	MU_ASSERT_INT(-5, (int)result);
	MU_ASSERT_INT(0, arith_eval("-5 + 3", &result));
	MU_ASSERT_INT(-2, (int)result);
	MU_ASSERT_INT(0, arith_eval("10 + -3", &result));
	MU_ASSERT_INT(7, (int)result);
	MU_ASSERT_INT(0, arith_eval("-(2 + 3)", &result));
	MU_ASSERT_INT(-5, (int)result);
}

/* ===== arith_eval: whitespace =========================================== */

static void	test_arith_whitespace(void)
{
	long long int	result;

	MU_ASSERT_INT(0, arith_eval("  3  +  4  ", &result));
	MU_ASSERT_INT(7, (int)result);
	MU_ASSERT_INT(0, arith_eval("\t3\t*\t2\t", &result));
	MU_ASSERT_INT(6, (int)result);
	MU_ASSERT_INT(0, arith_eval("3+4", &result));
	MU_ASSERT_INT(7, (int)result);
}

/* ===== arith_eval: edge cases and errors ================================ */

static void	test_arith_zero(void)
{
	long long int	result;

	MU_ASSERT_INT(0, arith_eval("0", &result));
	MU_ASSERT_INT(0, (int)result);
	MU_ASSERT_INT(0, arith_eval("0 + 0 * 0", &result));
	MU_ASSERT_INT(0, (int)result);
}

static void	test_arith_large_numbers(void)
{
	long long int	result;

	MU_ASSERT_INT(0, arith_eval("1000000 * 1000", &result));
	MU_ASSERT("1 billion", result == 1000000000L);
}

static void	test_arith_division_by_zero(void)
{
	long long int	result;

	MU_ASSERT("div by zero is error", arith_eval("1 / 0", &result) == -1);
	MU_ASSERT("mod by zero is error", arith_eval("1 % 0", &result) == -1);
}

static void	test_arith_trailing_garbage(void)
{
	long long int	result;

	MU_ASSERT("trailing word", arith_eval("3 + 4 bad", &result) == -1);
	MU_ASSERT("trailing op",   arith_eval("3 +", &result) == -1);
}

/* ===== expand_word: $((expr)) via expand_word =========================== */

static void	test_expand_arith_simple(void)
{
	t_shell	shell = {0};
	char	*out;

	stub_shell_init(&shell);
	out = expand_word(&shell, "$((3 + 4))");
	MU_ASSERT_STR("simple arith", "7", out);
	free(out);
	stub_shell_cleanup(&shell);
}

static void	test_expand_arith_precedence(void)
{
	t_shell	shell = {0};
	char	*out;

	stub_shell_init(&shell);
	out = expand_word(&shell, "$((2 + 3 * 4))");
	MU_ASSERT_STR("precedence in expand", "14", out);
	free(out);
	stub_shell_cleanup(&shell);
}

static void	test_expand_arith_negative_result(void)
{
	t_shell	shell = {0};
	char	*out;

	stub_shell_init(&shell);
	out = expand_word(&shell, "$((3 - 10))");
	MU_ASSERT_STR("negative result", "-7", out);
	free(out);
	stub_shell_cleanup(&shell);
}

static void	test_expand_arith_with_variable(void)
{
	t_shell	shell = {0};
	char	*out;

	stub_shell_init(&shell);
	inject_var(&shell, "X", "5");
	out = expand_word(&shell, "$(($X + 3))");
	MU_ASSERT_STR("var in arith", "8", out);
	free(out);
	stub_shell_cleanup(&shell);
}

static void	test_expand_arith_two_variables(void)
{
	t_shell	shell = {0};
	char	*out;

	stub_shell_init(&shell);
	inject_var(&shell, "A", "10");
	inject_var(&shell, "B", "4");
	out = expand_word(&shell, "$(($A * $B))");
	MU_ASSERT_STR("two vars in arith", "40", out);
	free(out);
	stub_shell_cleanup(&shell);
}

static void	test_expand_arith_in_double_quotes(void)
{
	t_shell	shell = {0};
	char	*out;

	stub_shell_init(&shell);
	out = expand_word(&shell, "\"result: $((6 / 2))\"");
	MU_ASSERT_STR("arith in dq", "result: 3", out);
	free(out);
	stub_shell_cleanup(&shell);
}

static void	test_expand_arith_concatenated(void)
{
	t_shell	shell = {0};
	char	*out;

	stub_shell_init(&shell);
	out = expand_word(&shell, "val=$((2 + 2))x");
	MU_ASSERT_STR("arith concat", "val=4x", out);
	free(out);
	stub_shell_cleanup(&shell);
}

static void	test_expand_arith_two_expansions_in_one_word(void)
{
	t_shell	shell = {0};
	char	*out;

	stub_shell_init(&shell);
	out = expand_word(&shell, "$((1 + 1))-$((2 * 3))");
	MU_ASSERT_STR("two arith in word", "2-6", out);
	free(out);
	stub_shell_cleanup(&shell);
}

static void	test_expand_arith_zero_result(void)
{
	t_shell	shell = {0};
	char	*out;

	stub_shell_init(&shell);
	out = expand_word(&shell, "$((0))");
	MU_ASSERT_STR("zero result", "0", out);
	free(out);
	stub_shell_cleanup(&shell);
}

static void	test_expand_arith_parentheses(void)
{
	t_shell	shell = {0};
	char	*out;

	stub_shell_init(&shell);
	out = expand_word(&shell, "$(((2 + 3) * 4))");
	MU_ASSERT_STR("parens in arith", "20", out);
	free(out);
	stub_shell_cleanup(&shell);
}

/* ===== expand_command: assignment with arithmetic ======================= */

static void	test_expand_command_arith_assignment(void)
{
	t_shell	shell = {0};
	t_cmd	*cmd;

	stub_shell_init(&shell);
	cmd = malloc(sizeof(t_cmd));
	cmd->argv = NULL;
	cmd->argc = 0;
	cmd->redirs = NULL;
	cmd->assignments = ft_lstnew(ft_strdup("X=$((1 + 2))"));
	MU_ASSERT_INT(0, expand_command(&shell, cmd));
	MU_ASSERT_STR("arith assignment",
			"X=3",
			(char *)cmd->assignments->content);
	ast_free(ast_new_command(cmd));
	stub_shell_cleanup(&shell);
}

static void	test_expand_command_arith_assignment_with_var(void)
{
	t_shell	shell = {0};
	t_cmd	*cmd;

	stub_shell_init(&shell);
	inject_var(&shell, "N", "7");
	cmd = malloc(sizeof(t_cmd));
	cmd->argv = NULL;
	cmd->argc = 0;
	cmd->redirs = NULL;
	cmd->assignments = ft_lstnew(ft_strdup("Y=$(($N * 2))"));
	MU_ASSERT_INT(0, expand_command(&shell, cmd));
	MU_ASSERT_STR("arith assignment with var",
			"Y=14",
			(char *)cmd->assignments->content);
	ast_free(ast_new_command(cmd));
	stub_shell_cleanup(&shell);
}

/* ===== expand_command: argv with arithmetic ============================= */

static void	test_expand_command_arith_argv(void)
{
	t_shell	shell = {0};
	t_cmd	*cmd;

	stub_shell_init(&shell);
	cmd = malloc(sizeof(t_cmd));
	cmd->argv = malloc(sizeof(char *) * 3);
	cmd->argv[0] = ft_strdup("echo");
	cmd->argv[1] = ft_strdup("$((4 * 3 + 2))");
	cmd->argv[2] = NULL;
	cmd->argc = 2;
	cmd->assignments = NULL;
	cmd->redirs = NULL;
	MU_ASSERT_INT(0, expand_command(&shell, cmd));
	MU_ASSERT_INT(2, cmd->argc);
	MU_ASSERT_STR("echo", "echo", cmd->argv[0]);
	MU_ASSERT_STR("arith argv", "14", cmd->argv[1]);
	MU_ASSERT("argv NULL terminated", cmd->argv[2] == NULL);
	ast_free(ast_new_command(cmd));
	stub_shell_cleanup(&shell);
}

/* ===== Suite registration ============================================== */

void	test_arithmetic_suite(void)
{
	test_arith_addition();
	test_arith_subtraction();
	test_arith_multiplication();
	test_arith_division();
	test_arith_modulo();
	test_arith_precedence_mul_over_add();
	test_arith_precedence_div_over_sub();
	test_arith_precedence_mixed();
	test_arith_precedence_left_associative();
	test_arith_parens_override_precedence();
	test_arith_parens_nested();
	test_arith_parens_redundant();
	test_arith_unary_minus();
	test_arith_whitespace();
	test_arith_zero();
	test_arith_large_numbers();
	test_arith_division_by_zero();
	test_arith_trailing_garbage();
	test_expand_arith_simple();
	test_expand_arith_precedence();
	test_expand_arith_negative_result();
	test_expand_arith_with_variable();
	test_expand_arith_two_variables();
	test_expand_arith_in_double_quotes();
	test_expand_arith_concatenated();
	test_expand_arith_two_expansions_in_one_word();
	test_expand_arith_zero_result();
	test_expand_arith_parentheses();
	test_expand_command_arith_assignment();
	test_expand_command_arith_assignment_with_var();
	test_expand_command_arith_argv();
}

#else

void	test_arithmetic_suite(void)
{
}

#endif
