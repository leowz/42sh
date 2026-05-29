/**
 * @file test_builtin_cd.c
 * @brief Unit tests for the 42sh cd builtin.
 */

#ifdef TEST_BUILTIN_CD_ENABLED
#endif /* TEST_BUILTIN_CD_ENABLED */

# include "minunit.h"
# include "builtins.h"
# include "variables.h"
# include "stdlib.h"
# include "sys/types.h"
# include "sys/stat.h"
# include "limits.h"
# include "string.h"

/**
 * @brief Seed a fresh shell with HOME/PWD/OLDPWD from the calling
 *        process's environ.
 * @details The cd builtin reads/writes shell->variables (not getenv);
 *          tests need the same seed values a real shell would inherit
 *          at startup.
 */
static void	cd_test_init(t_shell *shell)
{
	const char	*names[3];
	const char	*v;
	int			i;

	memset(shell, 0, sizeof(*shell));
	shell->env_dirty = 1;
	names[0] = "HOME";
	names[1] = "PWD";
	names[2] = "OLDPWD";
	i = 0;
	while (i < 3)
	{
		v = getenv(names[i]);
		if (v)
		{
			var_set(shell, names[i], v);
			var_export(shell, names[i]);
		}
		i++;
	}
}

/**
 * @param shell : the shell structure
 * @param indication : label describing the test case
 * @param str_std : expected PWD value
 * @param ret_std : expected return value
 * @param ret : actual return value
 * @brief Verify that PWD, getcwd(), and return values match expected results
 */
static void	check_test(t_shell *shell, char *indication, char *str_std, int ret_std, int ret)
{
	char	buf1[PATH_MAX];
	char	buf2[PATH_MAX];
	char	*pwd;

	pwd = var_get_value(shell, "PWD");
	MU_ASSERT_STR(indication, str_std, pwd);
	MU_ASSERT_STR("PWD vs gecwd", realpath(pwd ? pwd : "", buf1), getcwd(buf2, PATH_MAX));
	MU_ASSERT_INT(shell->last_exit_status, ret_std);
	MU_ASSERT_INT(ret_std, ret);
}

/**
 * @brief Test cd behavior when switching to root directory '/'
 */
static void	test_cd_root(void)
{
	t_shell	shell = {0};

	cd_test_init(&shell);
	int		ret;
	char	*argv[] = {"cd", "/", NULL};

	ret = builtin_cd(&shell, 2, argv);
	check_test(&shell, "root", "/", 0, ret);
}

/**
 * @brief Test cd with absolute paths,
 *	including normalization and options (-L, -P)
 */
static void	test_cd_absolute(void)
{
	t_shell	shell = {0};

	cd_test_init(&shell);
	int		ret;

	{
		char	*argv[] = {"cd", getenv("HOME"), NULL};
		ret = builtin_cd(&shell, 2, argv);
		check_test(&shell, "home", getenv("HOME"), 0, ret);
	}

	{
		char	buff[1024];
		bzero(buff, 1024);
		sprintf(buff, "%s/CDTEST/PHYSICAL/./.././../", getenv("HOME"));

		char	*argv[] = {"cd", buff, NULL};
		ret = builtin_cd(&shell, 2, argv);
		check_test(&shell, "absolute directory", getenv("HOME"), 0, ret);
	}

	{
		char	buff[1024];
		bzero(buff, 1024);
		sprintf(buff, "%s/CDTEST", getenv("HOME"));

		char	*argv[] = {"cd", buff, NULL};
		ret = builtin_cd(&shell, 2, argv);
		check_test(&shell, "absolute directory", buff, 0, ret);
	}

	{
		char	buff[1033];
		char	cdtest[1024];
		bzero(buff, 1024);
		bzero(cdtest, 1024);
		sprintf(cdtest, "%s/CDTEST", getenv("HOME"));
		sprintf(buff, "%s/SYMBOLIC", cdtest);

		{
			char	*argv[] = {"cd", cdtest, NULL};
			builtin_cd(&shell, 2, argv);
		}

		char	*argv[] = {"cd", "-L", buff, NULL};
		ret = builtin_cd(&shell, 3, argv);
		check_test(&shell, "-L option", buff, 0, ret);
	}

	{
		char	*argv[] = {"cd", NULL};
		
		ret = builtin_cd(&shell, 1, argv);
		check_test(&shell, "no path", getenv("HOME"), 0, ret);
	}

	{
		char	buff[1024];
		bzero(buff, 1024);
		sprintf(buff, "%s/CDTEST/SYMBOLIC", getenv("HOME"));
		char	physical[1024];
		bzero(physical, 1024);
		sprintf(physical, "%s/CDTEST/PHYSICAL", getenv("HOME"));

		char	*argv[] = {"cd", "-P", buff, NULL};
		ret = builtin_cd(&shell, 3, argv);
		check_test(&shell, "-P option", physical, 0, ret);
	}
}

/**
 * @brief Test cd with relative paths
 * 	and environment edge cases (HOME unset, ., ..)
 */
static void	test_cd_relative(void)
{
	{
		t_shell	shell = {0};
		int		ret;

		cd_test_init(&shell);

		{
			char	*argv[] = {"cd", NULL};

			ret = builtin_cd(&shell, 1, argv);
			check_test(&shell, "home", getenv("HOME"), 0, ret);
		}

		{
			char	*argv[] = {"cd", "CDTEST/", NULL};
			char	buff[1024];
			bzero(buff, 1024);

			sprintf(buff, "%s/CDTEST", getenv("HOME"));
			ret = builtin_cd(&shell, 2, argv);
			check_test(&shell, "relative directory", buff, 0, ret);
		}

		{
			char	*home = ft_strdup(var_get_value(&shell, "HOME"));
			char	*argv[] = {"cd", NULL};
			char	expected_pwd[4096];

			bzero(expected_pwd, 1024);
			strncpy(expected_pwd, var_get_value(&shell, "PWD"), PATH_MAX - 1);

			var_unset(&shell, "HOME");
			ret = builtin_cd(&shell, 1, argv);

			MU_ASSERT_STR("no home: PWD unchanged", expected_pwd,
				var_get_value(&shell, "PWD"));
			MU_ASSERT_INT(shell.last_exit_status, 1);
			MU_ASSERT_INT(1, ret);

			var_set(&shell, "HOME", home);
			var_export(&shell, "HOME");
			free(home);
		}

		{
			char	*argv[] = {"cd", "-L", "SYMBOLIC", NULL};
			char	buff[1024];
			bzero(buff, 1024);

			sprintf(buff, "%s/SYMBOLIC", var_get_value(&shell, "PWD"));
			ret = builtin_cd(&shell, 3, argv);
			check_test(&shell, "-L option", buff, 0, ret);
		}

		{
			char	*argv[] = {"cd", "..", NULL};
			char	buff[1024];
			bzero(buff, 1024);

			sprintf(buff, "%s/CDTEST", getenv("HOME"));
			ret = builtin_cd(&shell, 2, argv);
			check_test(&shell, "parent directory", buff, 0, ret);
		}

		{
			char	*argv[] = {"cd", ".", NULL};
			char	buff[1024];
			bzero(buff, 1024);

			sprintf(buff, "%s/CDTEST", getenv("HOME"));
			ret = builtin_cd(&shell, 2, argv);
			check_test(&shell, "current directory", buff, 0, ret);
		}

		{
			char	*argv[] = {"cd", "-LP", "SYMBOLIC", NULL};
			char	buff[1024];
			bzero(buff, 1024);

			sprintf(buff, "%s/PHYSICAL", var_get_value(&shell, "PWD"));
			ret = builtin_cd(&shell, 3, argv);
			check_test(&shell, "-P option", buff, 0, ret);
		}
	}
}

/**
 * @brief Test cd behavior with invalid or non-existent paths
 */
static void	test_cd_wrong_path(void)
{
	t_shell	shell = {0};

	cd_test_init(&shell);
	int		ret;

	{
		char	*argv1[] = {"cd", NULL};
		char	*argv2[] = {"cd", "CDTEST", NULL};

		ret = builtin_cd(&shell, 1, argv1);
		ret = builtin_cd(&shell, 2, argv2);
	}

	{
		char	*argv[] = {"cd", "NPQ", NULL};
		char	buff[1024];
		bzero(buff, 1024);

		sprintf(buff, "%s/CDTEST", getenv("HOME"));
		ret = builtin_cd(&shell, 2, argv);
		check_test(&shell, "wrong path", buff, 1, ret);
		MU_ASSERT_STR("oldpwd", var_get_value(&shell, "OLDPWD"), getenv("HOME"));
	}

	{
		char	*argv[] = {"cd", "-L", "--", "-P", NULL};
		char	buff[1024];
		bzero(buff, 1024);

		sprintf(buff, "%s/CDTEST", getenv("HOME"));
		ret = builtin_cd(&shell, 4, argv);
		check_test(&shell, "wrong path after --", buff, 1, ret);
		MU_ASSERT_STR("oldpwd", var_get_value(&shell, "OLDPWD"), getenv("HOME"));
	}
}

/**
 * @brief Test cd handling of invalid command-line options
 */
static void	test_cd_wrong_option(void)
{
	t_shell	shell = {0};

	cd_test_init(&shell);
	int		ret;

	{
		char	*argv1[] = {"cd", NULL};
		ret = builtin_cd(&shell, 1, argv1);
	}
	
	{
		char	*argv[] = {"cd", "-W", "CDTEST", NULL};

		ret = builtin_cd(&shell, 3, argv);
		check_test(&shell, "wrong simple option", getenv("HOME"), 2, ret);
	}

	{
		char	*argv[] = {"cd", "-LPW", "CDTEST", NULL};

		ret = builtin_cd(&shell, 3, argv);
		check_test(&shell, "wrong multiple options", getenv("HOME"), 2, ret);
	}

	{
		char	*argv[] = {"cd", "--Physical", "CDTEST", NULL};

		ret = builtin_cd(&shell, 3, argv);
		check_test(&shell, "wrong long option", getenv("HOME"), 2, ret);
	}
}

/**
 * @brief Test cd behavior when too many arguments are provided
 */
static void	test_cd_too_many_args(void)
{
	t_shell	shell = {0};

	cd_test_init(&shell);
	int		ret;

	{
		char	*argv[] = {"cd", NULL};
		ret = builtin_cd(&shell, 1, argv);
	}

	{
		char	*argv[] = {"cd", "path1", "path2", NULL};

		ret = builtin_cd(&shell, 3, argv);
		check_test(&shell, "too many args", getenv("HOME"), 2, ret);
	}
}

/**
 * @brief Test cd behavior when access to a directory is forbidden
 */
static void	test_cd_forbidden(void)
{
	t_shell	shell = {0};

	cd_test_init(&shell);
	int	ret;

	{
		char	*argv[] = {"cd", NULL};
		ret = builtin_cd(&shell, 1, argv);
	}

	{
		char	*argv[] = {"cd", "CDTEST/FORBIDDEN", NULL};

		ret = builtin_cd(&shell, 2, argv);
		check_test(&shell, "forbidden", getenv("HOME"), 1, ret);
	}
}

/**
 * @brief Test cd behavior with OLDPWD and '-' option
 */
static void	test_cd_oldpasswd(void)
{
	t_shell	shell = {0};

	cd_test_init(&shell);
	int		ret;

	{
		char	*argv[] = {"cd", NULL};
		ret = builtin_cd(&shell, 1, argv);
	}

	{
		char	*argv[] = {"cd", "-", NULL};

		var_unset(&shell, "OLDPWD");
		ret = builtin_cd(&shell, 2, argv);
		check_test(&shell, "no oldpwd", getenv("HOME"), 1, ret);
	}

	{
		char	*argv1[] = {"cd", "CDTEST", NULL};
		char	*argv2[] = {"cd", "-", NULL};

		ret = builtin_cd(&shell, 2, argv1);
		ret = builtin_cd(&shell, 2, argv2);
		check_test(&shell, "HOME", getenv("HOME"), 0, ret);
	}

	{
		char	*argv1[] = {"cd", "CDTEST", NULL};
		char	*argv2[] = {"cd", "PHYSICAL", NULL};
		char	*argv3[] = {"cd", "-", NULL};
		char	buff[1024];
		bzero(buff, 1024);

		sprintf(buff, "%s/CDTEST", getenv("HOME"));

		ret = builtin_cd(&shell, 2, argv1);
		ret = builtin_cd(&shell, 2, argv2);
		ret = builtin_cd(&shell, 2, argv3);
		check_test(&shell, "CDTEST", buff, 0, ret);
	}
}

/**
 * @brief Test cd behavior when navigating beyond the root directory
 */
static void	test_cd_beyond_the_root(void)
{
	t_shell	shell = {0};

	cd_test_init(&shell);
	int		ret;

	{
		char	*argv[] = {"cd", NULL};
		ret = builtin_cd(&shell, 1, argv);
	}

	{
		char	*argv[] = {"cd",
			"../../../../../../../../../../../../../../../../../../../../../../",
			NULL};

		ret = builtin_cd(&shell, 2, argv);
		check_test(&shell, "root", "/", 0, ret);
	}
}

/**
 * @brief Test cd behavior with redundant slashes in paths
 */
static void	test_cd_many_slash(void)
{
	t_shell	shell = {0};

	cd_test_init(&shell);
	int		ret;

	{
		char	*argv[] = {"cd", NULL};
		ret = builtin_cd(&shell, 1, argv);
	}

	{
		char	buff[1024];
		char	*argv[] = {"cd", "CDTEST///PHYSICAL//SUBDIR", NULL};

		sprintf(buff, "%s/CDTEST/PHYSICAL/SUBDIR", getenv("HOME"));
		ret = builtin_cd(&shell, 2, argv);
		check_test(&shell, "root", buff, 0, ret);
	}

	{
		char	*argv[] = {"cd", "///", NULL};

		ret = builtin_cd(&shell, 2, argv);
		check_test(&shell, "root", "/", 0, ret);
	}
}

/**
 * @brief Test cd behavior with excessively long paths or directories
 */
static void	test_cd_too_long(void)
{
	t_shell	shell = {0};

	cd_test_init(&shell);
	int		ret;

	{
		char	*argv[] = {"cd", NULL};
		ret = builtin_cd(&shell, 1, argv);
	}

	{
		char	directory[_POSIX_PATH_MAX + 2];
		memset(directory, 'N', _POSIX_PATH_MAX + 1);
		directory[_POSIX_PATH_MAX + 1] = '\0';

		char	*argv[] = {"cd", directory, NULL};
		ret = builtin_cd(&shell, 2, argv);
		check_test(&shell, "too long directory", getenv("HOME"), 1, ret);
	}

	{
		size_t	i = 1;
		char	path[PATH_MAX + 2];
		path[PATH_MAX + 1] = '\0';
		path[0] = '/';
		while (i < PATH_MAX - 4)
		{
			path[i++] = 'D';
			path[i++] = 'I';
			path[i++] = 'R';
			path[i++] = '/';
		}
		while (i < PATH_MAX + 1)
			path[i++] = 'E';

		char	*argv[] = {"cd", path, NULL};
		ret = builtin_cd(&shell, 2, argv);
		check_test(&shell, "too long path", getenv("HOME"), 1, ret);
	}
}

/**
 * @brief Create directory structure used for cd unit tests
 */
static void	create_test_directories(void)
{
	if (chdir(getenv("HOME")) < 0)
	{ perror("chdir"); return; }

	if (mkdir("CDTEST", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0)
	{ perror("mkdir"); return; }

	if (chdir("CDTEST") < 0)
	{ perror("chdir"); return; }

	if (mkdir("PHYSICAL", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
	{ perror("mkdir"); return; }

	if (mkdir("FORBIDDEN", 0000))
	{ perror("mkdir"); return; }

	symlink("PHYSICAL", "SYMBOLIC");
	if (chdir("PHYSICAL") < 0)
	{ perror("chdir"); return; }

	if (mkdir("SUBDIR", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
	{ perror("mkdir"); return; }

	if (chdir("../..") < 0)
	{ perror("chdir"); return; }
}

/**
 * @brief Clean up and remove directory structure created for tests
 */
static void	delete_test_directories(void)
{
	if (chdir(getenv("HOME")) < 0)
	{ perror("if (chdir"); return; }
	if (chdir("CDTEST") < 0)
	{ perror("if (chdir"); return; }
	remove("SYMBOLIC");
	if (chdir("PHYSICAL") < 0)
	{ perror("if (chdir"); return; }
	if (rmdir("SUBDIR") < 0)
	{ perror("if (rmdir"); return; }
	if (chdir("..") < 0)
	{ perror("if (chdir"); return; }
	if (rmdir("PHYSICAL") < 0)
	{ perror("if (rmdir"); return; }
	if (chmod("FORBIDDEN", S_IRWXU) < 0)
		perror("chmod");
	if (rmdir("FORBIDDEN") < 0)
	{ perror("if (rmdir"); return; }
	if (chdir("..") < 0)
	{ perror("if (chdir"); return; }
	if (rmdir("CDTEST") < 0)
	{ perror("rmdir"); return; }
}

/**
 * @brief Run the full test suite for the cd builtin
 */
void	test_builtin_cd_suite(void)
{
	char	saved_cwd[PATH_MAX];

	if (!getcwd(saved_cwd, sizeof(saved_cwd)))
		saved_cwd[0] = '\0';
	create_test_directories();

	test_cd_root();
	test_cd_absolute();
	test_cd_relative();
	test_cd_wrong_path();
	test_cd_wrong_option();
	test_cd_too_many_args();
	test_cd_oldpasswd();
	test_cd_forbidden();
	test_cd_beyond_the_root();
	test_cd_many_slash();
	test_cd_too_long();

	delete_test_directories();
	if (saved_cwd[0] && chdir(saved_cwd) != 0)
		perror("chdir");
}
