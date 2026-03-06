/**
 * @file minunit.h
 * @brief Minimal header-only unit-test framework for 42sh.
 * @author pulgamecanica (arosado-)
 *
 * Provides shared pass/fail counters and assertion macros used across all
 * test suites.  No external libraries are required beyond libc.
 *
 * ## Design
 *
 * !(assets/tests_meme.jpeg)[assets/tests_meme.jpeg]
 *
 * `g_mu_passed` and `g_mu_failed` are declared `extern` here and defined
 * exactly once in test_runner.c so that every translation unit accumulates
 * results into the same global totals.
 *
 * ## Workflow
 *
 * Modules start life guarded by `#ifdef TEST_ (MODULE) _ENABLED`.  Once an
 * implementation is complete and all assertions pass the guards are removed
 * and the corresponding `-D` flag is deleted from the Makefile.  The test
 * then compiles and runs unconditionally forever.
 *
 * ## Quick-start — test file
 *
 * ```c
 * void test_something(void) {
 *     MU_ASSERT("truth holds", 1 == 1);
 *     MU_ASSERT_INT(42, compute_answer());
 *     MU_ASSERT_STR("label", "expected", get_string());
 * }
 * ```
 *
 * ## Quick-start — test_runner.c
 *
 * ```c
 * int main(void) {
 *     MU_RUN(test_something);
 *     MU_SUMMARY();
 * }
 * ```
 */

#ifndef MINUNIT_H
# define MINUNIT_H

# include <stdio.h>
# include <string.h>

/**
 * Running total of assertions that passed.
 *
 * Defined in test_runner.c; all test translation units share this counter
 * through the `extern` declaration in this header.
 */
extern int	g_mu_passed;

/**
 * Running total of assertions that failed.
 *
 * A non-zero value causes MU_SUMMARY() to return exit code 1 so that
 * `make` can detect and report test failures.
 */
extern int	g_mu_failed;

/**
 * Generic boolean assertion.
 *
 * Evaluates *expr*. Prints a green **PASS** line on success, or a red
 * **FAIL** line including the source location on failure.
 * Do not call directly — use the *MU_ASSERT* macro.
 *
 * @param msg   Human-readable description shown in the output line.
 * @param expr  Non-zero = pass, zero = fail.
 *
 * Example:
 * 
 * ```c
 * MU_ASSERT("pointer is not null", ptr != NULL);
 * ```
 * 
 */
static inline void	mu_assert(const char *file, int line,
						const char *msg, int expr)
{
	if (!expr)
	{
		printf("  \033[1;31mFAIL\033[0m %s:%d — %s\n", file, line, msg);
		g_mu_failed++;
	}
	else
	{
		printf("  \033[1;32mPASS\033[0m %s\n", msg);
		g_mu_passed++;
	}
}

# define MU_ASSERT(msg, expr) \
	mu_assert(__FILE__, __LINE__, (msg), (int)!!(expr))

/**
 * Integer equality assertion.
 *
 * Compares *expected* and *actual* as `int` values.  On failure the
 * output includes both the expected and the actual values.
 * Do not call directly — use the *MU_ASSERT_INT* macro.
 *
 * @param expected The value the expression should produce.
 * @param actual   The expression under test.
 *
 * Example:
 * 
 * ```c
 * MU_ASSERT_INT(3, (int)ft_lstsize(list));
 * ```
 * 
 */
static inline void	mu_assert_int(const char *file, int line,
						int expected, int actual)
{
	if (expected != actual)
	{
		printf("  \033[1;31mFAIL\033[0m %s:%d — expected %d, got %d\n",
			file, line, expected, actual);
		g_mu_failed++;
	}
	else
	{
		printf("  \033[1;32mPASS\033[0m int == %d\n", expected);
		g_mu_passed++;
	}
}

# define MU_ASSERT_INT(expected, actual) \
	mu_assert_int(__FILE__, __LINE__, (int)(expected), (int)(actual))

/**
 * String equality assertion (NULL-safe).
 *
 * Compares *expected* and *actual* with strcmp(3).  A NULL *actual* is
 * treated as a failure and printed as `"(null)"` in the output line.
 * Do not call directly — use the *MU_ASSERT_STR* macro.
 *
 * @param msg      Human-readable label shown in the output line.
 * @param expected The expected C string (must not be NULL).
 * @param actual   The actual C string returned by the code under test.
 *
 * Example:
 * 
 * ```c
 * MU_ASSERT_STR("newest entry", "pwd", history_prev(&h));
 * ```
 * 
 */
static inline void	mu_assert_str(const char *file, int line,
						const char *msg, const char *expected,
						const char *actual)
{
	if (!actual || strcmp(expected, actual) != 0)
	{
		printf("  \033[1;31mFAIL\033[0m %s:%d — %s: want \"%s\" got \"%s\"\n",
			file, line, msg, expected, actual ? actual : "(null)");
		g_mu_failed++;
	}
	else
	{
		printf("  \033[1;32mPASS\033[0m %s: \"%s\"\n", msg, actual);
		g_mu_passed++;
	}
}

# define MU_ASSERT_STR(msg, expected, actual) \
	mu_assert_str(__FILE__, __LINE__, (msg), (expected), (actual))

/**
 * Execute a test suite function and announce its name.
 *
 * Prints a cyan header line with the name of *fn* then calls `fn()`.
 * Typically used in `main()` of test_runner.c.
 * Do not call directly — use the *MU_RUN* macro.
 *
 * @param name  Stringified name of the test suite function.
 * @param fn    Function pointer to the test suite.
 *
 * Example:
 * 
 * ```c
 * MU_RUN(test_list_suite);
 * ```
 * 
 */
static inline void	mu_run(const char *name, void (*fn)(void))
{
	printf("\n\033[1;36m>>> %s\033[0m\n", name);
	fn();
}

# define MU_RUN(fn) mu_run(#fn, fn)

/**
 * Print the final pass/fail totals and return an exit code.
 *
 * Must be called as the last statement in `main()` via the *MU_SUMMARY*
 * macro.  Returns `0` if all tests passed or `1` if any test failed, so
 * that `make` can propagate the result.
 *
 * Example:
 * 
 * ```c
 * int main(void) {
 *     MU_RUN(test_list_suite);
 *     MU_SUMMARY();
 * }
 * ```
 */
static inline int	mu_summary(void)
{
	printf("\n\033[1;37m--- %d passed, %d failed ---\033[0m\n",
		g_mu_passed, g_mu_failed);
	return (g_mu_failed > 0 ? 1 : 0);
}

# define MU_SUMMARY() return mu_summary()

#endif
