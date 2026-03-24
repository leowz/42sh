/**
 * @file test_runner.c
 * @brief Entry point for the 42sh unit test suite.
 * @author pulgamecanica (arosado-)
 *
 * Run with: `make test`
 *
 * ## How to Graduate a Test
 *
 * - 1. Implement the module under `srcs/MODULE/`.
 * - 2. Add `-DTEST_MODULE_ENABLED` to `TEST_FLAGS` in the root Makefile.
 * - 3. Verify all MU_ASSERT lines pass with `make test`.
 * - 4. When permanently green:\n
 *    Delete `#ifdef` / `#else` / `#endif` in `tests/test_module.c`.\n
 *    Delete `#ifdef` / `#endif` around MU_RUN() below.\n
 * 		Remove `-DTEST_MODULE_ENABLED` from the root Makefile.\n
 *
 * The test then compiles and runs unconditionally forever.
 */

#include "minunit.h"

/* Shared pass/fail counters — extern in minunit.h, defined once here. */
int	g_mu_passed = 0;
int	g_mu_failed = 0;

/* Forward declarations — one per test file. */
void	test_dlist_suite(void);
void	test_list_suite(void);
void	test_history_suite(void);
void	test_lexer_suite(void);
void	test_btree_suite(void);
void	test_parser_suite(void);

int	main(void)
{
	printf("\033[1;35m=== 42sh Unit Tests ===\033[0m\n");

	MU_RUN(test_dlist_suite);
	MU_RUN(test_list_suite);
	MU_RUN(test_history_suite);
	MU_RUN(test_lexer_suite);
	MU_RUN(test_btree_suite);
	MU_RUN(test_parser_suite);

	MU_SUMMARY();
}
