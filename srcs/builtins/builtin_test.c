// filepath: /home/josh/Desktop/42sh/srcs/builtins/builtin_test.c
/**
 * @file builtin_test.c
 * @brief Implementation of the test builtin command for the 42sh shell.
 * @author pulgamecanica
 */

#include "42sh.h"
#include "builtins.h"
#include <sys/stat.h>
#include <unistd.h>



static int	test_file_exists(const char *file, int flag)
{
    struct stat	buf;

    if (flag == 'L' || flag == 'h')
    {
        if (lstat(file, &buf) < 0)
            return (0);
        return (S_ISLNK(buf.st_mode));
    }
    if (stat(file, &buf) < 0)
        return (0);
    if (flag == 'e')
        return (1);
    if (flag == 'b')
        return (S_ISBLK(buf.st_mode));
    if (flag == 'c')
        return (S_ISCHR(buf.st_mode));
    if (flag == 'd')
        return (S_ISDIR(buf.st_mode));
    if (flag == 'f')
        return (S_ISREG(buf.st_mode));
    if (flag == 'r')
        return (access(file, R_OK) == 0);
    if (flag == 'w')
        return (access(file, W_OK) == 0);
    if (flag == 'x')
        return (access(file, X_OK) == 0);
    if (flag == 's')
        return (buf.st_size > 0);
    return (0);
}

static int	test_string(const char *str, int op)
{
    if (op == 'z')
        return (str == NULL || *str == '\0');
    if (op == 'n')
        return (str != NULL && *str != '\0');
    return (0);
}

static int	test_string_compare(const char *s1, const char *s2, int op)
{
    if (op == '=')
        return (ft_strcmp(s1, s2) == 0);
    if (op == '!')
        return (ft_strcmp(s1, s2) != 0);
    if (op == '<')
        return (ft_strcmp(s1, s2) < 0);
    if (op == '>')
        return (ft_strcmp(s1, s2) > 0);
    return (0);
}

static int	test_numeric(const char *s1, const char *s2, const char *op)
{
    long	n1;
    long	n2;
    char	*endptr;

    n1 = strtol(s1, &endptr, 10);
    if (*endptr != '\0')
        return (0);
    n2 = strtol(s2, &endptr, 10);
    if (*endptr != '\0')
        return (0);
    if (ft_strcmp(op, "-eq") == 0)
        return (n1 == n2);
    if (ft_strcmp(op, "-ne") == 0)
        return (n1 != n2);
    if (ft_strcmp(op, "-lt") == 0)
        return (n1 < n2);
    if (ft_strcmp(op, "-le") == 0)
        return (n1 <= n2);
    if (ft_strcmp(op, "-gt") == 0)
        return (n1 > n2);
    if (ft_strcmp(op, "-ge") == 0)
        return (n1 >= n2);
    return (0);
}

static int	evaluate_test(t_test_context *ctx)
{
    char	*arg;
    char	*op;

    if (ctx->pos >= ctx->argc)
        return (0);
    arg = ctx->argv[ctx->pos++];
    
    /* Handle unary flags: -e, -z, -n, -f, -d, -b, -c, -r, -w, -x, -s */
    if (arg[0] == '-' && arg[1] != '\0' && arg[1] != '-')
    {
        if (arg[1] == 'z' && arg[2] == '\0')
        {
            if (ctx->pos >= ctx->argc)
                return (0);
            return (test_string(ctx->argv[ctx->pos++], 'z'));
        }
        if (arg[1] == 'n' && arg[2] == '\0')
        {
            if (ctx->pos >= ctx->argc)
                return (0);
            return (test_string(ctx->argv[ctx->pos++], 'n'));
        }
        if (arg[2] == '\0')
        {
            if (ctx->pos >= ctx->argc)
                return (0);
            return (test_file_exists(ctx->argv[ctx->pos++], arg[1]));
        }
    }
    
    /* Check if next argument is a binary operator */
    if (ctx->pos < ctx->argc)
    {
        op = ctx->argv[ctx->pos];
        
        /* Numeric comparisons */
        if (ft_strncmp(op, "-eq", 3) == 0 || ft_strncmp(op, "-ne", 3) == 0 ||
            ft_strncmp(op, "-lt", 3) == 0 || ft_strncmp(op, "-le", 3) == 0 ||
            ft_strncmp(op, "-gt", 3) == 0 || ft_strncmp(op, "-ge", 3) == 0)
        {
            if (ctx->pos + 1 >= ctx->argc)
                return (0);
            ctx->pos += 2;
            return (test_numeric(arg, ctx->argv[ctx->pos - 1], op));
        }
        
        /* String equality comparisons */
        if (ft_strcmp(op, "=") == 0 || ft_strcmp(op, "!=") == 0)
        {
            if (ctx->pos + 1 >= ctx->argc)
                return (0);
            ctx->pos += 2;
            return (test_string_compare(arg, ctx->argv[ctx->pos - 1], op[0]));
        }
        
        /* String lexicographic comparisons */
        if (ft_strcmp(op, "<") == 0 || ft_strcmp(op, ">") == 0)
        {
            if (ctx->pos + 1 >= ctx->argc)
                return (0);
            ctx->pos += 2;
            return (test_string_compare(arg, ctx->argv[ctx->pos - 1], op[0]));
        }
    }
    
    return (0);
}

int	builtin_test(struct s_shell *shell, int argc, char **argv)
{
    t_test_context	ctx;

    if (argc < 2)
    {
        shell->last_exit_status = 1;
        return (1);
    }
    ctx.argc = argc;
    ctx.argv = argv;
    ctx.pos = 1;
    shell->last_exit_status = !evaluate_test(&ctx) ? 1 : 0;
    return (shell->last_exit_status);
}