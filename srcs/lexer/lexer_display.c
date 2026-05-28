/**
 * @file lexer_display.c
 * @brief Display and JSON serialization of token lists
 * @author pulgamecanica
 */

#ifdef FT_EXTRA_VERBOSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include "lexer.h"

#define TOKENIZATION_DIR_DEFAULT "viz/tokenizations"

static const char	*get_tokenization_dir(void)
{
	const char	*env;

	env = getenv("SH42_VIZ_DIR");
	if (env)
		return (env);
	return (TOKENIZATION_DIR_DEFAULT);
}

static const char	*tok_type_str(t_token_type type)
{
	static const char	*names[] = {
		"TOK_WORD",
		"TOK_PIPE",
		"TOK_AND",
		"TOK_OR",
		"TOK_SEMICOLON",
		"TOK_AMPERSAND",
		"TOK_NEWLINE",
		"TOK_REDIR_IN",
		"TOK_REDIR_OUT",
		"TOK_REDIR_APPEND",
		"TOK_HEREDOC",
		"TOK_HEREDOC_STRIP",
		"TOK_REDIR_DUP_IN",
		"TOK_REDIR_DUP_OUT",
		"TOK_LPAREN",
		"TOK_RPAREN",
		"TOK_ARITH_OPEN",
		"TOK_ARITH_CLOSE",
		"TOK_EOF",
		"TOK_ERROR"
	};

	if (type >= 0 && (size_t)type < sizeof(names) / sizeof(names[0]))
		return (names[type]);
	return ("UNKNOWN");
}

void	lexer_display(t_list *tokens, const char *input)
{
	t_list	*node;
	t_token	*tok;
	int		i;

	printf("\033[1;36m┌─ Tokenization ────────────────────────────────\033[0m\n");
	printf("\033[1;36m│\033[0m Input : \033[1;33m%s\033[0m\n", input);
	printf("\033[1;36m├────────────────────────────────────────────────\033[0m\n");
	node = tokens;
	i = 0;
	while (node)
	{
		tok = TOK(node);
		printf("\033[1;36m│\033[0m [%2d]  %-20s  value=\033[1;32m%-20s\033[0m",
			i, tok_type_str(tok->type), tok->value ? tok->value : "(null)");
		if (tok->io_number != -1)
			printf("  io=%d", tok->io_number);
		printf("\n");
		node = node->next;
		i++;
	}
	printf("\033[1;36m└────────────────────────────────────────────────\033[0m\n");
}

/* ─── JSON helpers ──────────────────────────────────────────────── */

/**
 * @brief Write a JSON-safe escaped string to fp (no surrounding quotes).
 */
static void	write_json_str(FILE *fp, const char *s)
{
	if (!s)
	{
		fprintf(fp, "null");
		return ;
	}
	fputc('"', fp);
	while (*s)
	{
		if (*s == '"')
			fputs("\\\"", fp);
		else if (*s == '\\')
			fputs("\\\\", fp);
		else if (*s == '\n')
			fputs("\\n", fp);
		else if (*s == '\t')
			fputs("\\t", fp);
		else
			fputc(*s, fp);
		s++;
	}
	fputc('"', fp);
}

/**
 * @brief Append this tokenization entry to tokenizations/manifest.json.
 * @details The manifest is a JSON array of relative filenames so the
 *          static viewer can discover all dumps without a server.
 */
static void	update_manifest(const char *json_filename)
{
	FILE		*fp;
	char		manifest_path[256];
	char		tmp_path[256];
	char		line[512];
	char		*p;
	size_t		len;

	snprintf(manifest_path, sizeof(manifest_path), "%s/manifest.json",
		get_tokenization_dir());
	snprintf(tmp_path, sizeof(tmp_path), "%s/manifest.json.tmp",
		get_tokenization_dir());

	fp = fopen(manifest_path, "r");
	FILE *tmp = fopen(tmp_path, "w");
	if (!tmp)
	{
		if (fp)
			fclose(fp);
		return ;
	}
	fprintf(tmp, "[\n");
	if (fp)
	{
		while (fgets(line, sizeof(line), fp))
		{
			/* Strip leading whitespace */
			p = line;
			while (*p == ' ' || *p == '\t')
				p++;
			/* Skip structural characters */
			if (*p == '[' || *p == ']' || *p == '\0' || *p == '\n')
				continue ;
			/* Strip trailing whitespace/newline */
			len = strlen(p);
			while (len > 0 && (p[len - 1] == '\n' || p[len - 1] == '\r'
					|| p[len - 1] == ' '))
				p[--len] = '\0';
			if (len == 0)
				continue ;
			/* Remove trailing comma */
			if (p[len - 1] == ',')
				p[--len] = '\0';
			/* Skip duplicate */
			{
				char	quoted[256];
				snprintf(quoted, sizeof(quoted), "\"%s\"", json_filename);
				if (strcmp(p, quoted) == 0)
					continue ;
			}
			fprintf(tmp, "  %s,\n", p);
		}
		fclose(fp);
	}
	fprintf(tmp, "  \"%s\"\n]\n", json_filename);
	fclose(tmp);
	rename(tmp_path, manifest_path);
}

/**
 * @details Creates tokenizations/ if missing, writes a timestamped file,
 *                   and updates tokenizations/manifest.json for the static viewer.
 */
char	*lexer_to_json(t_list *tokens, const char *input)
{
	char		filename[64];
	char		filepath[128];
	FILE		*fp;
	t_list		*node;
	t_token		*tok;
	int			first;
	struct timespec ts;

	mkdir(get_tokenization_dir(), 0755);
	clock_gettime(CLOCK_REALTIME, &ts);
	snprintf(filename, sizeof(filename), "tok_%ld_%06ld.json",
		(long)ts.tv_sec, ts.tv_nsec / 1000);
	snprintf(filepath, sizeof(filepath), "%s/%s", get_tokenization_dir(), filename);

	fp = fopen(filepath, "w");
	if (!fp)
	{
		perror("lexer_to_json: fopen");
		return (NULL);
	}
	fprintf(fp, "{\n");
	fprintf(fp, "  \"input\": ");
	write_json_str(fp, input);
	fprintf(fp, ",\n");
	fprintf(fp, "  \"tokens\": [\n");

	node = tokens;
	first = 1;
	while (node)
	{
		tok = TOK(node);
		if (!first)
			fprintf(fp, ",\n");
		first = 0;
		fprintf(fp, "    {\n");
		fprintf(fp, "      \"type\": \"%s\",\n", tok_type_str(tok->type));
		fprintf(fp, "      \"value\": ");
		write_json_str(fp, tok->value);
		fprintf(fp, ",\n");
		fprintf(fp, "      \"io_number\": %d\n", tok->io_number);
		fprintf(fp, "    }");
		node = node->next;
	}
	fprintf(fp, "\n  ]\n}\n");
	fclose(fp);

	update_manifest(filename);

	if (isatty(STDOUT_FILENO))
	{
		fprintf(stdout, "\033]1337;42sh;tok;{\"input\":");
		write_json_str(stdout, input);
		fprintf(stdout, ",\"tokens\":[");
		node = tokens;
		first = 1;
		while (node)
		{
			tok = TOK(node);
			if (!first)
				fprintf(stdout, ",");
			first = 0;
			fprintf(stdout, "{\"type\":\"%s\",\"value\":", tok_type_str(tok->type));
			write_json_str(stdout, tok->value);
			fprintf(stdout, ",\"io_number\":%d}", tok->io_number);
			node = node->next;
		}
		fprintf(stdout, "]}\007");
		fflush(stdout);
	}

	return (strdup(filepath));
}

#endif /* FT_EXTRA_VERBOSE */
