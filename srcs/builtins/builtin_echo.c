/**
 * @file builtin_echo.c
 * @brief Implementation of the echo builtin command for the 42sh shell.
 * @author jguillem
 */

#include "42sh.h"
#include "builtins.h"

/**
 * @param token : the token to analyze
 * @param nl : pointer to int to know the trailing newline status
 * @param escape : pointer to int to know the backslash interpretation status
 * @brief check if the option is -e, -E, -N or a combination thereof
 * @return 1 | 0
 */
static int	is_valid_option(char *token, int *nl, int *escape)
{
	int	i = 1;
	int	nl_bak = *nl;
	int	esc_bak = *escape;

	if (!token)
		return (0);
	if (token[0] != '-')
		return (0);
	if (token[1] == '\0')
		return (0);
	while (token[i])
	{
		switch (token[i])
		{
			case 'n' :
				*nl = 1;
				break;
			case 'e' :
				*escape = 1;
				break;
			case 'E' :
				*escape = 0;
				break;
			default :
				*nl = nl_bak;
				*escape = esc_bak;
				return (0);
		}
		i++;
	}
	return (1);
}

/**
 * @param c : symbol to check
 * @param symbols : list of valid symbols
 * @brief check if the symbol c own to the symbols list
 * @return 0 | 1
 */
static int	is_valid_symbol(char c, char symbols[])
{
	char	maj;
	int		i;

	i = -1;
	maj = toupper(c);
	while (symbols[++i])
	{
		if (symbols[i] == maj)
			return (1);
	}
	return (0);
}

/**
 * @param scout : the pointer of pointer on the number
 * @param base : the input base
 * @brief : display the char corresponding to the base number given
 */
static void	display_base(char **scout, int base)
{
	int		i = 0;
	int		lim;
	int		res;
	int		valid;
	char	symbols[17];

	if (base == 16)
	{
		char	tmp[17] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 0};
		memcpy(symbols, tmp, 17);
		lim = 2;
	}
	else if (base == 8)
	{
		char	tmp[9] = {'0', '1', '2', '3', '4', '5', '6', '7', 0};
		memcpy(symbols, tmp, 9);
		lim = 3;
	}
	else
		return;
	res = 0;
	while (i < lim && **scout && (valid = is_valid_symbol(**scout, symbols)))
	{
		res *= base;
		res += **scout - '0';
		(*scout)++;
		i++;
	}
	if (i != 0)
		printf("%c", res);
	else if (i == 0 && base == 16)
		printf("\\x");
}

/**
 * @param token : char *
 * @param last : flag to know if a trailing space is necessary
 * @param escape : flag to know if there is a backslash interpretation
 * @brief display the token
 * @return 0 | 1
 */
static int	display_token(char *token, int last, int escape)
{
	char	*scout;
	char	*word;

	if (!escape && token)
		printf("%s", token);
	else if (token)
	{
		scout = token;
		while (*scout)
		{
			while (*scout && *scout != '\\')
				scout++;
			word = strndup(token, scout - token);
			printf("%s", word);
			free(word);
			if (*scout)
			{
				scout++;
				switch (*scout)
				{
					case '\\' :
						scout++;
						printf("\\");
						break;
					case 'a' :
						scout++;
						printf("\a");
						break;
					case 'b' :
						scout++;
						printf("\b");
						break;
					case 'c' :
						return (0);
					case 'e' :
						scout++;
						printf("\e");
						break;
					case 'f' :
						scout++;
						printf("\f");
						break;
					case 'n' :
						scout++;
						printf("\n");
						break;
					case 'r' :
						scout++;
						printf("\r");
						break;
					case 't' :
						scout++;
						printf("\t");
						break;
					case 'v' :
						scout++;
						printf("\v");
						break;
					case '0' :
						scout++;
						display_base(&scout, 8);
						break;
					case 'x' :
						scout++;
						display_base(&scout, 16);
						break;
					default :
						printf("\\");
				}
				token = scout;
			}
		}
	}
	if (last)
		printf(" ");
	return (1);
}

int	builtin_echo(struct s_shell *shell, int argc, char **argv)
{
	int	i = 1;
	int	nl = 0;
	int	escape = 0;
	int	run = 1;

	while (is_valid_option(argv[i], &nl, &escape))
		i++;
	while (run && i < argc - 1)
		run = display_token(argv[i++], 1, escape);
	if (run)
	{
		run = display_token(argv[i], 0, escape);
		if (run && nl == 0)
			printf("\n");
	}
	shell->last_exit_status = 0;
	return (0);
}
