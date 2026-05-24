/**
 * @file expand_arithmetic.c
 * @brief Dollar-sign expansions: $NAME, ${NAME}, $?, $$, $0.
 * @brief Arithmetic expansions: $((expr))
 * @author jguillem
 */
#include <stdio.h>
#include "expander.h"
#include "ctype.h"

static void	skip_spaces(t_arith *a)
{
	while (a->s[a->i] && isspace(a->s[a->i]))
		a->i++;
}

static long long int	parse_factor(t_arith *a)
{
	long long int		val;
	int					sign;

	skip_spaces(a);
	sign = 1;
	if (a->s[a->i] && a->s[a->i] == '-')
	{
		sign = -1;
		a->i++;
	}
	if (a->s[a->i] && a->s[a->i] == '(')
	{
		a->i++;
		val = parse_expr(a);
		skip_spaces(a);
		if (a->s[a->i] && a->s[a->i] == ')')
			a->i++;
		else
		{
			fprintf(stderr,
					"42sh: %s: arithmetic syntax error: missing ')'\n", a->s);
			a->error = 1;
			return (0);
		}
		return (sign * val);
	}
	if (!isdigit((unsigned char)a->s[a->i]))
	{
		fprintf(stderr,
				"42sh: %s: arithmetic syntax error: operand expected (error token is \"%s\")\n",
				a->s, a->s + a->i - 1);
		a->error = 1;
		return (0);
	}
	val = 0;
	while (a->s[a->i] && isdigit(a->s[a->i]))
		val = val * 10 + (a->s[a->i++] - '0');
	return (sign *val);
}

static long long int	parse_term(t_arith *a)
{
	long long int	val;
	long long int	factor;
	char			op;

	val = parse_factor(a);
	skip_spaces(a);
	while (!a->error && a->s[a->i]
			&& (a->s[a->i] == '*' || a->s[a->i] == '/' || a->s[a->i] == '%'))
	{
		op = a->s[a->i++];
		factor = parse_factor(a);
		if (op == '*')
			val *= factor;
		else if (factor == 0)
		{
			fprintf(stderr,
					"42sh: val %c 0: division by 0 (error token is \"0\")\n", op);
			a->error = 1;
			return (0);
		}
		else if (op == '/')
			val /= factor;
		else
			val %= factor;
		skip_spaces(a);
	}
	return (val);
}

long long int	parse_expr(t_arith *a)
{
	long long int	val;
	char			op;

	val = parse_term(a);
	skip_spaces(a);
	while (!a->error && a->s[a->i] && (a->s[a->i] == '+' || a->s[a->i] == '-'))
	{
		op = a->s[a->i++];
		if (op == '+')
			val += parse_term(a);
		else
			val -= parse_term(a);
		skip_spaces(a);
	}
	return (val);
}

int	arith_eval(const char *expr, long long int *result)
{
	t_arith	a;

	a.s = expr;
	a.i = 0;
	a.error = 0;
	*result = parse_expr(&a);
	if (a.error)
		return (-1);
	skip_spaces(&a);
	if (a.s[a.i])
		return (-1);
	return (0);
}

int	expand_arithmetic(struct s_shell *shell, const char *input,
		size_t *pos, int dq, t_xbuf *out)
{
	size_t			start;
	int				depth;
	char			*raw_expr;
	char			*expanded_expr;
	long long int	result;
	char			*result_str;
	int				rc;

	*pos += 3;
	start = *pos;
	depth = 1;
	while (input[*pos] && depth > 0)
	{
		if (input[*pos] == '(' && input[*pos + 1] == '(')
		{
			depth++;
			*pos += 2;
		}
		else if (input[*pos] == ')' && input[*pos + 1] == ')')
		{
			depth--;
			*pos += 2;
		}
		else
			(*pos)++;
	}
	if (depth != 0)
		return (-1);
	raw_expr = ft_strsub(input, start, *pos - 2 - start);
	if (!raw_expr)
		return (-1);
	expanded_expr = expand_word(shell, raw_expr);
	free(raw_expr);
	if (arith_eval(expanded_expr, &result) == -1)
	{
		free(expanded_expr);
		return (-1);
	}
	free(expanded_expr);
	result_str = ft_itoa(result);
	if (!result_str)
		return (-1);
	rc = xbuf_puts(out, result_str, !dq);
	free(result_str);
	return (rc);
}
