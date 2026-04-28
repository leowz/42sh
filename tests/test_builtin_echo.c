/**
 * @file test_builtins_echo.c
 * @brief Unit tests for the 42sh echo builtin
 */

#ifdef TEST_BUILTIN_ECHO_ENABLED
#endif

#define BUFSIZE 256

# include "minunit.h"
# include "builtins.h"
# include <stdlib.h>
# include <string.h>
# include <unistd.h>

static int	capture_echo(t_shell *shell, int argc, char **argv,
	char *buf, size_t buf_size)
{
	int		pipefd[2];
	int		saved_stdout;
	int		ret;
	ssize_t	n = 0;
	ssize_t	total = 0;

	memset(buf, 0, buf_size);
	if (pipe(pipefd) == -1)
		return (-1);
	saved_stdout = dup(STDOUT_FILENO);
	if (saved_stdout == -1)
		return (-1);
	fflush(stdout);
	if (dup2(pipefd[1], STDOUT_FILENO) == -1)
		return (-1);
	setvbuf(stdout, NULL, _IONBF, 0);
	close(pipefd[1]);
	ret = builtin_echo(shell, argc, argv);
	fflush(stdout);
	if (dup2(saved_stdout, STDOUT_FILENO) == -1)
		return (-1);
	close(saved_stdout);
	while ((n = read(pipefd[0], buf + total, buf_size - 1 - total)) > 0)
		total += n;
	buf[total] = '\0';
	close(pipefd[0]);
	return (ret);
}

static void test_echo_return_and_status(void)
{
	t_shell shell;
	char	buf[BUFSIZE];
	char	*argv[] = {"echo", "hello"};

	MU_ASSERT_INT(0, capture_echo(&shell, 2, argv, buf, BUFSIZE));
	MU_ASSERT_INT(0, shell.last_exit_status);
}

// static void test_echo_no_args(void)
// {
// 	t_shell shell;
// 	char	buf[BUFSIZE];
// 	char	*argv[] = {"echo"};

// 	capture_echo(&shell, 1, argv, buf, BUFSIZE);
// 	MU_ASSERT_STR("no args → bare newline", "\n", buf);
// }

static void test_echo_single_word(void)
{
	t_shell shell;
	char	buf[BUFSIZE];
	char	*argv[] = {"echo", "hello"};

	capture_echo(&shell, 2, argv, buf, BUFSIZE);
	MU_ASSERT_STR("single word", "hello\n", buf);
}

static void test_echo_multiple_words(void)
{
	t_shell shell;
	char	buf[BUFSIZE];
	char	*argv[] = {"echo", "shi", "fu", "mi"};

	capture_echo(&shell, 4, argv, buf, BUFSIZE);
	MU_ASSERT_STR("words joined by single space", "shi fu mi\n", buf);
}

static void test_echo_empty_string_arg(void)
{
	t_shell shell;
	char	buf[BUFSIZE];
	char	*argv[] = {"echo", ""};

	capture_echo(&shell, 2, argv, buf, BUFSIZE);
	MU_ASSERT_STR("empty string arg → bare newline", "\n", buf);
}

static void test_echo_n_suppresses_newline(void)
{
	t_shell shell;
	char	buf[BUFSIZE];
	char	*argv[] = {"echo", "-n", "hello"};

	capture_echo(&shell, 3, argv, buf, BUFSIZE);
	MU_ASSERT_STR("-n suppresses newline", "hello", buf);
}

// static void test_echo_n_alone(void)
// {
// 	t_shell shell;
// 	char	buf[BUFSIZE];
// 	char	*argv[] = {"echo", "-n"};

// 	capture_echo(&shell, 2, argv, buf, BUFSIZE);
// 	MU_ASSERT_STR("-n alone → empty output", "", buf);
// }

static void test_echo_n_multiple_words(void)
{
	t_shell shell;
	char	buf[BUFSIZE];
	char	*argv[] = {"echo", "-n", "foo", "bar", "baz"};

	capture_echo(&shell, 5, argv, buf, BUFSIZE);
	MU_ASSERT_STR("-n with multiple words", "foo bar baz", buf);
}

static void test_echo_e_newline(void)
{
	t_shell shell;
	char	buf[BUFSIZE];
	char	*argv[] = {"echo", "-e", "foo\\nbar"};

	capture_echo(&shell, 3, argv, buf, BUFSIZE);
	MU_ASSERT_STR("-e interprets \\n", "foo\nbar\n", buf);
}

static void test_echo_e_tab(void)
{
	t_shell shell;
	char	buf[BUFSIZE];
	char	*argv[] = {"echo", "-e", "foo\\tbar"};

	capture_echo(&shell, 3, argv, buf, BUFSIZE);
	MU_ASSERT_STR("-e interprets \\t", "foo\tbar\n", buf);
}

static void test_echo_e_backslash(void)
{
	t_shell shell;
	char	buf[BUFSIZE];
	char	*argv[] = {"echo", "-e", "foo\\\\bar"};

	capture_echo(&shell, 3, argv, buf, BUFSIZE);
	MU_ASSERT_STR("-e interprets \\\\", "foo\\bar\n", buf);
}

static void test_echo_e_carriage_return(void)
{
	t_shell shell;
	char	buf[BUFSIZE];
	char	*argv[] = {"echo", "-e", "foo\\rbar"};

	capture_echo(&shell, 3, argv, buf, BUFSIZE);
	MU_ASSERT_STR("-e interprets \\r", "foo\rbar\n", buf);
}

static void test_echo_e_alert(void)
{
	t_shell shell;
	char	buf[BUFSIZE];
	char	*argv[] = {"echo", "-e", "\\a"};

	capture_echo(&shell, 3, argv, buf, BUFSIZE);
	MU_ASSERT_STR("-e interprets \\a (bell)", "\a\n", buf);
}

static void test_echo_e_backspace(void)
{
	t_shell shell;
	char	buf[BUFSIZE];
	char	*argv[] = {"echo", "-e", "\\b"};

	capture_echo(&shell, 3, argv, buf, BUFSIZE);
	MU_ASSERT_STR("-e interprets \\b (backspace)", "\b\n", buf);
}

static void test_echo_e_vertical_tab(void)
{
	t_shell shell;
	char	buf[BUFSIZE];
	char	*argv[] = {"echo", "-e", "\\v"};

	capture_echo(&shell, 3, argv, buf, BUFSIZE);
	MU_ASSERT_STR("-e interprets \\v", "\v\n", buf);
}

static void test_echo_e_formfeed(void)
{
	t_shell shell;
	char	buf[BUFSIZE];
	char	*argv[] = {"echo", "-e", "\\f"};

	capture_echo(&shell, 3, argv, buf, BUFSIZE);
	MU_ASSERT_STR("-e interprets \\f", "\f\n", buf);
}

static void test_echo_e_c_stops_output(void)
{
	t_shell shell;
	char	buf[BUFSIZE];
	char	*argv[] = {"echo", "-e", "hel\\clo"};

	capture_echo(&shell, 3, argv, buf, BUFSIZE);
	MU_ASSERT_STR("-e \\c stops output mid-string", "hel", buf);
}

static void test_echo_e_c_at_start(void)
{
	t_shell shell;
	char	buf[BUFSIZE];
	char	*argv[] = {"echo", "-e", "\\chello"};

	capture_echo(&shell, 3, argv, buf, BUFSIZE);
	MU_ASSERT_STR("-e \\c at start → empty output", "", buf);
}

static void test_echo_e_octal(void)
{
	t_shell shell;
	char	buf[BUFSIZE];
	char	*argv[] = {"echo", "-e", "B.\\0101.BA\\09"};

	capture_echo(&shell, 3, argv, buf, BUFSIZE);
	MU_ASSERT_STR("-e interprets B.A.AB9\\n", "B.A.BA9\n", buf);
}

static void	test_echo_e_octal_empty(void)
{
	t_shell shell;
	char	buf[BUFSIZE];
	char	*argv[] = {"echo", "-e", "hello\\0"};

	capture_echo(&shell, 3, argv, buf, BUFSIZE);
	MU_ASSERT_STR("-e interprets hello\\n", "hello\n", buf);
}

static void	test_echo_e_octal_too_long(void)
{
	t_shell shell;
	char	buf[BUFSIZE];
	char	*argv[] = {"echo", "-e", "B.A.B\\01014"};

	capture_echo(&shell, 3, argv, buf, BUFSIZE);
	MU_ASSERT_STR("-e interprets B.A.BA4\\n", "B.A.BA4\n", buf);
}

static void test_echo_e_hex(void)
{
	t_shell shell;
	char	buf[BUFSIZE];
	char	*argv[] = {"echo", "-e", "B.\\x41.BA\\xG"};

	capture_echo(&shell, 3, argv, buf, BUFSIZE);
	MU_ASSERT_STR("-e interprets B.A.BA\\xG", "B.A.BA\\xG\n", buf);
}

static void	test_echo_e_hex_empty(void)
{
	t_shell shell;
	char	buf[BUFSIZE];
	char	*argv[] = {"echo", "-e", "hello\\x"};

	capture_echo(&shell, 3, argv, buf, BUFSIZE);
	MU_ASSERT_STR("-e interprets hello\\x\\n", "hello\\x\n", buf);
}

static void	test_echo_e_hex_too_long(void)
{
	t_shell shell;
	char	buf[BUFSIZE];
	char	*argv[] = {"echo", "-e", "B.A.B\\x414"};

	capture_echo(&shell, 3, argv, buf, BUFSIZE);
	MU_ASSERT_STR("-e interprets B.A.BA4\\n", "B.A.BA4\n", buf);
}

static void test_echo_e_unknown_escape(void)
{
	t_shell shell;
	char	buf[BUFSIZE];
	char	*argv[] = {"echo", "-e", "\\q"};

	capture_echo(&shell, 3, argv, buf, BUFSIZE);
	MU_ASSERT_STR("-e unknown escape → literal", "\\q\n", buf);
}

static void test_echo_E_no_interpretation(void)
{
	t_shell shell;
	char	buf[BUFSIZE];
	char	*argv[] = {"echo", "-E", "foo\\nbar"};

	capture_echo(&shell, 3, argv, buf, BUFSIZE);
	MU_ASSERT_STR("-E keeps \\n literal", "foo\\nbar\n", buf);
}

static void test_echo_E_overrides_e(void)
{
	t_shell shell;
	char	buf[BUFSIZE];
	char	*argv[] = {"echo", "-e", "-E", "foo\\nbar"};

	capture_echo(&shell, 4, argv, buf, BUFSIZE);
	MU_ASSERT_STR("-e then -E → -E wins", "foo\\nbar\n", buf);
}

static void test_echo_e_overrides_E(void)
{
	t_shell shell;
	char	buf[BUFSIZE];
	char	*argv[] = {"echo", "-E", "-e", "foo\\nbar"};

	capture_echo(&shell, 4, argv, buf, BUFSIZE);
	MU_ASSERT_STR("-E then -e → -e wins", "foo\nbar\n", buf);
}

static void test_echo_e_n_combined(void)
{
	t_shell shell;
	char	buf[BUFSIZE];
	char	*argv[] = {"echo", "-e", "-n", "foo\\tbar"};

	capture_echo(&shell, 4, argv, buf, BUFSIZE);
	MU_ASSERT_STR("-e -n: escape + no newline", "foo\tbar", buf);
}

static void test_echo_n_e_combined(void)
{
	t_shell shell;

	char	buf[BUFSIZE];
	char	*argv[] = {"echo", "-n", "-e", "foo\\tbar"};

	capture_echo(&shell, 4, argv, buf, BUFSIZE);
	MU_ASSERT_STR("-n -e: same result regardless of order", "foo\tbar", buf);
}

static void test_echo_dash_alone(void)
{
	t_shell shell;

	char	buf[BUFSIZE];
	char	*argv[] = {"echo", "-"};

	capture_echo(&shell, 2, argv, buf, BUFSIZE);
	MU_ASSERT_STR("bare '-' is not a flag", "-\n", buf);
}

static void test_echo_double_dash(void)
{
	t_shell shell;

	char	buf[BUFSIZE];
	char	*argv[] = {"echo", "--", "hello"};

	capture_echo(&shell, 3, argv, buf, BUFSIZE);
	MU_ASSERT_STR("'--' printed literally", "-- hello\n", buf);
}

static void test_echo_no_escape_without_e(void)
{
	t_shell shell;

	char	buf[BUFSIZE];
	char	*argv[] = {"echo", "foo\\nbar"};

	capture_echo(&shell, 2, argv, buf, BUFSIZE);
	MU_ASSERT_STR("no -e → \\n is literal", "foo\\nbar\n", buf);
}

void	test_builtin_echo_suite(void)
{
	test_echo_return_and_status();

	// test_echo_no_args();
	test_echo_single_word();
	test_echo_multiple_words();
	test_echo_empty_string_arg();
	test_echo_n_suppresses_newline();

	// test_echo_n_alone();
	test_echo_n_multiple_words();
	test_echo_e_newline();
	test_echo_e_tab();
	test_echo_e_backslash();
	test_echo_e_carriage_return();
	test_echo_e_alert();
	test_echo_e_backspace();
	test_echo_e_vertical_tab();
	test_echo_e_formfeed();
	test_echo_e_c_stops_output();
	test_echo_e_c_at_start();
	test_echo_e_octal();
	test_echo_e_octal_empty();
	test_echo_e_octal_too_long();
	test_echo_e_hex();
	test_echo_e_hex_empty();
	test_echo_e_hex_too_long();
	test_echo_e_unknown_escape();
	test_echo_E_no_interpretation();
	test_echo_E_overrides_e();
	test_echo_e_overrides_E();
	test_echo_e_n_combined();
	test_echo_n_e_combined();
	test_echo_dash_alone();
	test_echo_double_dash();
	test_echo_no_escape_without_e();
}
