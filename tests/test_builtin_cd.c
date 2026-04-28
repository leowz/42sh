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

static void	check_test(t_shell *shell, char *indication, char *str_std, int ret_std, int ret)
{
	MU_ASSERT_STR(indication, str_std, getenv("PWD"));
	MU_ASSERT_INT(shell->last_exit_status, ret_std);
	MU_ASSERT_INT(ret_std, ret);
}

static void	test_cd_root(void)
{
	t_shell	shell;
	int		ret;
	char	*argv[] = {"cd", "/"};

	ret = builtin_cd(&shell, 2, argv);
	check_test(&shell, "root", "/", 0, ret);
}

static void	test_cd_absolute(void)
{
	t_shell	shell;
	int		ret;
	char	*argv[] = {"cd", getenv("HOME")};

	ret = builtin_cd(&shell, 2, argv);
	check_test(&shell, "home", getenv("HOME"), 0, ret);
}

static void	test_cd_relative(void)
{
	char	*cwd_bak = getcwd(NULL, 0);
	chdir(getenv("HOME"));

	{
		t_shell	shell;
		int		ret;

		{
			char	*argv[] = {"cd", "CDTEST"};
			char	buff[1024];

			sprintf(buff, "%s/CDTEST", getenv("HOME"));
			ret = builtin_cd(&shell, 2, argv);
			check_test(&shell, "relative directory", buff, 0, ret);
		}

		{
			char	*argv[] = {"cd", "-L", "SYMBOLIC"};
			char	buff[1024];

			sprintf(buff, "%s/SYMBOLIC", getenv("PWD"));
			ret = builtin_cd(&shell, 3, argv);
			check_test(&shell, "-L option", buff, 0, ret);
		}

		{
			char	*argv[] = {"cd", ".."};
			char	buff[1024];

			sprintf(buff, "%s/CDTEST", getenv("HOME"));
			ret = builtin_cd(&shell, 2, argv);
			check_test(&shell, "parent directory", buff, 0, ret);
		}

		{
			char	*argv[] = {"cd", "."};
			char	buff[1024];

			sprintf(buff, "%s/CDTEST", getenv("HOME"));
			ret = builtin_cd(&shell, 2, argv);
			check_test(&shell, "current directory", buff, 0, ret);
		}

		{
			char	*argv[] = {"cd", "-P", "SYMBOLIC"};
			char	buff[1024];

			sprintf(buff, "%s/PHYSICAL", getenv("PWD"));
			ret = builtin_cd(&shell, 3, argv);
			check_test(&shell, "-P option", buff, 0, ret);
		}
	}

	chdir(getenv(cwd_bak));
	free(cwd_bak);
}

static void	create_test_directories(void)
{
	chdir(getenv("HOME"));
	mkdir("CDTEST", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	chdir("CDTEST");
	mkdir("PHYSICAL", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	symlink("PHYSICAL", "SYMBOLIC");
	chdir("..");
}

static void	delete_test_directories(void)
{
	chdir(getenv("HOME"));
	chdir("CDTEST");
	remove("SYMBOLIC");
	rmdir("PHYSICAL");
	chdir("..");
	rmdir("CDTEST");
}

void	test_builtin_cd_suite(void)
{
	create_test_directories();

	test_cd_root();
	test_cd_absolute();
	test_cd_relative();

	delete_test_directories();
}
