/**
 * @file test_history.c
 * @brief Test suite for the shell history module.
 * @author pulgamecanica (arosado-)
 *
 * Tests history_file_path() POSIX lookup order ($HISTFILE → $HOME/.sh_history)
 * and the history_save() / history_load() file round-trip via readline's
 * in-memory history list (history_list(), history_length, clear_history()).
 *
 * Guarded by `TEST_HISTORY_ENABLED` (set in `TEST_FLAGS` in the root
 * Makefile).  Once all assertions pass permanently, remove the #ifdef guards
 * and the `-D` flag from the Makefile.
 */

#include "minunit.h"
#include "libft.h"
#include "../includes/history.h"
#include <stdlib.h>
#include <unistd.h>
#include <readline/history.h>

#ifdef TEST_HISTORY_ENABLED

/**
 * Run all history module assertions.
 *
 * - history_file_path: $HISTFILE takes priority.
 * - history_file_path: falls back to $HOME/.sh_history when $HISTFILE unset.
 * - history_save / history_load round-trip preserves all entries.
 */
void	test_history_suite(void)
{
	char		*path;
	HIST_ENTRY	**list;

	/* $HISTFILE override */
	setenv("HISTFILE", "/tmp/42sh_histtest", 1);
	path = history_file_path();
	MU_ASSERT_STR("HISTFILE respected", "/tmp/42sh_histtest", path);
	free(path);

	/* $HOME/.sh_history fallback */
	unsetenv("HISTFILE");
	setenv("HOME", "/tmp/42test", 1);
	path = history_file_path();
	MU_ASSERT_STR("HOME fallback", "/tmp/42test/.42sh_history", path);
	free(path);

	/* save + load round-trip */
	using_history();
	clear_history();
	add_history("echo hello");
	add_history("ls -la");
	add_history("pwd");
	MU_ASSERT_INT(3, history_length);

	history_save("/tmp/42sh_hist_roundtrip");
	clear_history();
	MU_ASSERT_INT(0, history_length);

	history_load("/tmp/42sh_hist_roundtrip");
	MU_ASSERT_INT(3, history_length);

	list = history_list();
	MU_ASSERT("first entry correct", list && ft_strcmp(list[0]->line,
			"echo hello") == 0);
	MU_ASSERT("last entry correct", list && ft_strcmp(list[2]->line,
			"pwd") == 0);

	clear_history();
	unlink("/tmp/42sh_hist_roundtrip");
}

#else

void	test_history_suite(void)
{
}

#endif