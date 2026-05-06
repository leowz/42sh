/**
 * @file test_variables.c
 * @brief Tests for the shell variables module.
 * @author pulgamecanica
 *
 * Covers:
 *   - var_set / var_get / var_get_value: insert, lookup, overwrite,
 *     identifier validation.
 *   - var_unset: removal of head and middle nodes, missing-name no-op.
 *   - var_export: marks existing var, creates a NULL-valued export,
 *     refuses invalid identifiers.
 *   - var_get_environ: rebuild on env_dirty, NULL value rendered as
 *     "NAME=", unexported variables excluded.
 *   - var_init_from_environ: every entry becomes an exported variable.
 */

#ifdef TEST_VARIABLES_ENABLED

# include "minunit.h"
# include "../includes/42sh.h"
# include "../includes/variables.h"
# include <stdlib.h>
# include <string.h>

extern void	stub_shell_init(t_shell *shell);
extern void	stub_shell_cleanup(t_shell *shell);

/* ===== set / get / overwrite ========================================== */

static void	test_var_set_and_get(void)
{
	t_shell	shell;

	stub_shell_init(&shell);
	MU_ASSERT_INT(0, var_set(&shell, "FOO", "bar"));
	MU_ASSERT_STR("FOO=bar", "bar", var_get_value(&shell, "FOO"));
	MU_ASSERT("get returns node", var_get(&shell, "FOO") != NULL);
	MU_ASSERT("missing var", var_get(&shell, "MISSING") == NULL);
	MU_ASSERT("missing value", var_get_value(&shell, "MISSING") == NULL);
	stub_shell_cleanup(&shell);
}

static void	test_var_overwrite(void)
{
	t_shell	shell;

	stub_shell_init(&shell);
	var_set(&shell, "X", "first");
	var_set(&shell, "X", "second");
	MU_ASSERT_STR("overwritten", "second", var_get_value(&shell, "X"));
	var_set(&shell, "X", NULL);
	MU_ASSERT("NULL value reset", var_get_value(&shell, "X") == NULL);
	MU_ASSERT("var still exists", var_get(&shell, "X") != NULL);
	stub_shell_cleanup(&shell);
}

static void	test_var_invalid_identifier(void)
{
	t_shell	shell;

	stub_shell_init(&shell);
	MU_ASSERT_INT(1, var_set(&shell, "1BAD", "x"));
	MU_ASSERT_INT(1, var_set(&shell, "WITH-DASH", "x"));
	MU_ASSERT_INT(1, var_set(&shell, "", "x"));
	MU_ASSERT_INT(1, var_set(&shell, NULL, "x"));
	MU_ASSERT("none stored",
		var_get(&shell, "1BAD") == NULL
		&& var_get(&shell, "WITH-DASH") == NULL);
	MU_ASSERT_INT(0, var_set(&shell, "GOOD_1", "ok"));
	MU_ASSERT_INT(0, var_set(&shell, "_underscore", "ok"));
	stub_shell_cleanup(&shell);
}

/* ===== unset ========================================================== */

static void	test_var_unset_head(void)
{
	t_shell	shell;

	stub_shell_init(&shell);
	var_set(&shell, "A", "1");
	var_set(&shell, "B", "2");
	MU_ASSERT_INT(0, var_unset(&shell, "B"));
	MU_ASSERT("B gone", var_get(&shell, "B") == NULL);
	MU_ASSERT("A kept", var_get_value(&shell, "A") != NULL);
	stub_shell_cleanup(&shell);
}

static void	test_var_unset_missing(void)
{
	t_shell	shell;

	stub_shell_init(&shell);
	MU_ASSERT_INT(0, var_unset(&shell, "NOPE"));
	stub_shell_cleanup(&shell);
}

static void	test_var_unset_middle(void)
{
	t_shell	shell;

	stub_shell_init(&shell);
	var_set(&shell, "A", "1");
	var_set(&shell, "B", "2");
	var_set(&shell, "C", "3");
	MU_ASSERT_INT(0, var_unset(&shell, "B"));
	MU_ASSERT("B gone", var_get(&shell, "B") == NULL);
	MU_ASSERT_STR("A kept", "1", var_get_value(&shell, "A"));
	MU_ASSERT_STR("C kept", "3", var_get_value(&shell, "C"));
	stub_shell_cleanup(&shell);
}

/* ===== export + env =================================================== */

static int	env_contains(char **env, const char *entry)
{
	int	i;

	if (!env)
		return (0);
	i = 0;
	while (env[i])
	{
		if (ft_strequ(env[i], entry))
			return (1);
		i++;
	}
	return (0);
}

static void	test_var_export(void)
{
	t_shell	shell;
	char	**env;

	stub_shell_init(&shell);
	var_set(&shell, "EXPORTED", "yes");
	var_set(&shell, "PRIVATE", "no");
	MU_ASSERT_INT(0, var_export(&shell, "EXPORTED"));
	env = var_get_environ(&shell);
	MU_ASSERT("env not NULL", env != NULL);
	MU_ASSERT("EXPORTED present", env_contains(env, "EXPORTED=yes"));
	MU_ASSERT("PRIVATE absent", !env_contains(env, "PRIVATE=no"));
	stub_shell_cleanup(&shell);
}

static void	test_var_export_creates(void)
{
	t_shell	shell;
	char	**env;

	stub_shell_init(&shell);
	MU_ASSERT_INT(0, var_export(&shell, "FUTURE"));
	env = var_get_environ(&shell);
	MU_ASSERT("FUTURE rendered with empty value",
		env_contains(env, "FUTURE="));
	MU_ASSERT_INT(1, var_export(&shell, "1bad"));
	stub_shell_cleanup(&shell);
}

static void	test_env_dirty_rebuild(void)
{
	t_shell	shell;
	char	**env_a;
	char	**env_b;

	stub_shell_init(&shell);
	var_set(&shell, "X", "1");
	var_export(&shell, "X");
	env_a = var_get_environ(&shell);
	MU_ASSERT_INT(0, shell.env_dirty);
	var_set(&shell, "X", "2");
	MU_ASSERT_INT(1, shell.env_dirty);
	env_b = var_get_environ(&shell);
	MU_ASSERT("X=2 after rebuild", env_contains(env_b, "X=2"));
	MU_ASSERT("X=1 gone after rebuild", !env_contains(env_b, "X=1"));
	(void)env_a;
	stub_shell_cleanup(&shell);
}

/* ===== init from environ ============================================== */

static void	test_var_init_from_environ(void)
{
	t_shell		shell;
	char		*envp[3];
	char		**env;
	t_var		*var;

	stub_shell_init(&shell);
	envp[0] = ft_strdup("PATH=/bin:/usr/bin");
	envp[1] = ft_strdup("HOME=/home/user");
	envp[2] = NULL;
	var_init_from_environ(&shell, envp);
	MU_ASSERT_STR("PATH loaded", "/bin:/usr/bin",
		var_get_value(&shell, "PATH"));
	MU_ASSERT_STR("HOME loaded", "/home/user",
		var_get_value(&shell, "HOME"));
	var = var_get(&shell, "PATH");
	MU_ASSERT("PATH exported", var && var->exported);
	env = var_get_environ(&shell);
	MU_ASSERT("env contains PATH",
		env_contains(env, "PATH=/bin:/usr/bin"));
	MU_ASSERT("env contains HOME",
		env_contains(env, "HOME=/home/user"));
	free(envp[0]);
	free(envp[1]);
	stub_shell_cleanup(&shell);
}

static void	test_var_init_skips_malformed(void)
{
	t_shell	shell;
	char	*envp[3];

	stub_shell_init(&shell);
	envp[0] = ft_strdup("=novalue");
	envp[1] = ft_strdup("OK=fine");
	envp[2] = NULL;
	var_init_from_environ(&shell, envp);
	MU_ASSERT("malformed skipped", var_get(&shell, "") == NULL);
	MU_ASSERT_STR("OK loaded", "fine", var_get_value(&shell, "OK"));
	free(envp[0]);
	free(envp[1]);
	stub_shell_cleanup(&shell);
}

/* ===== Suite registration ============================================= */

void	test_variables_suite(void)
{
	test_var_set_and_get();
	test_var_overwrite();
	test_var_invalid_identifier();
	test_var_unset_head();
	test_var_unset_missing();
	test_var_unset_middle();
	test_var_export();
	test_var_export_creates();
	test_env_dirty_rebuild();
	test_var_init_from_environ();
	test_var_init_skips_malformed();
}

#else

void	test_variables_suite(void)
{
}

#endif
