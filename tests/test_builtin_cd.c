/**
 * @file test_builtin_cd.c
 * @brief Unit tests for the 42sh cd builtin.
 */

#ifdef TEST_BUILTIN_CD_ENABLED
#endif /* TEST_BUILTIN_CD_ENABLED */

# include "minunit.h"
# include "builtins.h"
# include "stdlib.h"
# include "sys/types.h"
# include "sys/stat.h"
# include "limits.h"

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

	MU_ASSERT_STR(indication, str_std, getenv("PWD"));
	MU_ASSERT_STR("PWD vs gecwd", realpath(getenv("PWD"), buf1), getcwd(buf2, PATH_MAX));
	MU_ASSERT_INT(shell->last_exit_status, ret_std);
	MU_ASSERT_INT(ret_std, ret);
}

/**
 * @brief Test cd behavior when switching to root directory '/'
 */
static void	test_cd_root(void)
{
	t_shell	shell = {0};
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
			char	*home = getenv("HOME");
			char	*argv[] = {"cd", NULL};
			char	expected_pwd[4096];
			bzero(expected_pwd, 1024);

			strncpy(expected_pwd, getenv("PWD"), PATH_MAX - 1);

			unsetenv("HOME");
			ret = builtin_cd(&shell, 1, argv);

			MU_ASSERT_STR("no home: PWD unchanged", expected_pwd, getenv("PWD"));
			MU_ASSERT_INT(shell.last_exit_status, 1);
			MU_ASSERT_INT(1, ret);

			setenv("HOME", home, 1);
		}

		{
			char	*argv[] = {"cd", "-L", "SYMBOLIC", NULL};
			char	buff[1024];
			bzero(buff, 1024);

			sprintf(buff, "%s/SYMBOLIC", getenv("PWD"));
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

			sprintf(buff, "%s/PHYSICAL", getenv("PWD"));
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
		MU_ASSERT_STR("oldpwd", getenv("OLDPWD"), getenv("HOME"));
	}

	{
		char	*argv[] = {"cd", "-L", "--", "-P", NULL};
		char	buff[1024];
		bzero(buff, 1024);

		sprintf(buff, "%s/CDTEST", getenv("HOME"));
		ret = builtin_cd(&shell, 4, argv);
		check_test(&shell, "wrong path after --", buff, 1, ret);
		MU_ASSERT_STR("oldpwd", getenv("OLDPWD"), getenv("HOME"));
	}
}

/**
 * @brief Test cd handling of invalid command-line options
 */
static void	test_cd_wrong_option(void)
{
	t_shell	shell = {0};
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
	int		ret;

	{
		char	*argv[] = {"cd", NULL};
		ret = builtin_cd(&shell, 1, argv);
	}

	{
		char	*argv[] = {"cd", "-", NULL};

		unsetenv("OLDPWD");
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
 * @brief Test cd behavior when directory argument is NULL or invalid
 */
static void	test_cd_null_directory(void)
{
	t_shell	shell = {0};
	int		ret;

	{
		char	*argv[] = {"cd", NULL};
		ret = builtin_cd(&shell, 1, argv);
	}

	{
		char	*argv[] = {"cd", NULL};
		
		ret = builtin_cd(&shell, 2, argv);
		check_test(&shell, "null directory", getenv("HOME"), 1, ret);
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
	test_cd_null_directory();

	delete_test_directories();
}
