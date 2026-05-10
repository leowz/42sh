/**
 * @file ast_display.c
 * @brief Display and JSON serialization of AST trees
 * @author pulgamecanica
 */

#ifdef FT_EXTRA_VERBOSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include "parser.h"

#define AST_DIR "viz/ast"

static const char	*node_type_str(t_node_type type)
{
	static const char	*names[] = {
		"NODE_COMMAND",
		"NODE_PIPE",
		"NODE_AND",
		"NODE_OR",
		"NODE_SEQUENCE",
		"NODE_SUBSHELL",
		"NODE_BLOCK",
		"NODE_BACKGROUND"
	};

	if (type >= 0 && (size_t)type < sizeof(names) / sizeof(names[0]))
		return (names[type]);
	return ("UNKNOWN");
}

static const char	*redir_type_str(t_token_type type)
{
	if (type == TOK_REDIR_IN)
		return ("<");
	if (type == TOK_REDIR_OUT)
		return (">");
	if (type == TOK_REDIR_APPEND)
		return (">>");
	if (type == TOK_HEREDOC)
		return ("<<");
	if (type == TOK_HEREDOC_STRIP)
		return ("<<-");
	if (type == TOK_REDIR_DUP_IN)
		return ("<&");
	if (type == TOK_REDIR_DUP_OUT)
		return (">&");
	return ("?");
}

static void	display_indent(int depth)
{
	int	i;

	i = 0;
	while (i < depth)
	{
		if (i == depth - 1)
			printf("\033[1;36m│  ├─ \033[0m");
		else
			printf("\033[1;36m│  \033[0m");
		i++;
	}
}

static void	display_redirs(t_list *redirs, int depth)
{
	t_list	*node;

	node = redirs;
	while (node)
	{
		display_indent(depth);
		printf("\033[1;35mredir\033[0m %s", redir_type_str(REDIR(node)->type));
		if (REDIR(node)->fd != -1)
			printf(" fd=%d", REDIR(node)->fd);
		printf(" \033[1;32m%s\033[0m\n", REDIR(node)->target);
		node = node->next;
	}
}

static void	ast_display_node(t_ast *node, int depth)
{
	int	i;

	if (!node)
		return ;
	display_indent(depth);
	printf("\033[1;33m%s\033[0m", node_type_str(node->type));
	if (node->type == NODE_COMMAND)
	{
		printf("  argv=[");
		i = 0;
		while (node->data.cmd->argv && node->data.cmd->argv[i])
		{
			if (i > 0)
				printf(", ");
			printf("\033[1;32m%s\033[0m", node->data.cmd->argv[i]);
			i++;
		}
		printf("]\tassignments=[");
		i = 0;
		for (t_list *it = node->data.cmd->assignments; it != NULL; i++)
		{
			if (i > 0)
				printf(", ");
			printf("\033[1;32m%s\033[0m", (char*)TOK(it));
			it = it->next;
		}
		printf("]\n");
		display_redirs(node->data.cmd->redirs, depth + 1);
	}
	else if (node->type == NODE_PIPE || node->type == NODE_AND
		|| node->type == NODE_OR || node->type == NODE_SEQUENCE)
	{
		printf("\n");
		ast_display_node(node->data.binary->left, depth + 1);
		ast_display_node(node->data.binary->right, depth + 1);
	}
	else
	{
		printf("\n");
		ast_display_node(node->data.group->child, depth + 1);
		display_redirs(node->data.group->redirs, depth + 1);
	}
}

void	ast_display(t_ast *ast, const char *input)
{
	printf("\033[1;36m┌─ AST ─────────────────────────────────────────\033[0m\n");
	printf("\033[1;36m│\033[0m Input : \033[1;33m%s\033[0m\n", input);
	printf("\033[1;36m├────────────────────────────────────────────────\033[0m\n");
	ast_display_node(ast, 0);
	printf("\033[1;36m└────────────────────────────────────────────────\033[0m\n");
}

static void	json_str(FILE *fp, const char *s)
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

static void	json_indent(FILE *fp, int depth)
{
	int	i;

	i = 0;
	while (i++ < depth)
		fprintf(fp, "  ");
}

static void	json_redirs(FILE *fp, t_list *redirs, int depth)
{
	t_list	*node;
	int		first;

	fprintf(fp, "[\n");
	node = redirs;
	first = 1;
	while (node)
	{
		if (!first)
			fprintf(fp, ",\n");
		first = 0;
		json_indent(fp, depth);
		fprintf(fp, "{\n");
		json_indent(fp, depth + 1);
		fprintf(fp, "\"type\": \"%s\",\n", redir_type_str(REDIR(node)->type));
		json_indent(fp, depth + 1);
		fprintf(fp, "\"fd\": %d,\n", REDIR(node)->fd);
		json_indent(fp, depth + 1);
		fprintf(fp, "\"target\": ");
		json_str(fp, REDIR(node)->target);
		fprintf(fp, ",\n");
		json_indent(fp, depth + 1);
		fprintf(fp, "\"heredoc_delim\": ");
		json_str(fp, REDIR(node)->heredoc_delim);
		fprintf(fp, ",\n");
		json_indent(fp, depth + 1);
		fprintf(fp, "\"heredoc_quoted\": %d\n",
			REDIR(node)->heredoc_quoted);
		json_indent(fp, depth);
		fprintf(fp, "}");
		node = node->next;
	}
	fprintf(fp, "\n");
	json_indent(fp, depth - 1);
	fprintf(fp, "]");
}

static void	json_assignments(FILE *fp, t_list *assignments, int depth)
{
	t_list	*node;
	int		first;

	fprintf(fp, "[");
	node = assignments;
	first = 1;
	while (node)
	{
		if (!first)
			fprintf(fp, ", ");
		first = 0;
		json_str(fp, (char *)node->content);
		node = node->next;
	}
	(void)depth;
	fprintf(fp, "]");
}

static void	json_argv(FILE *fp, char **argv)
{
	int	i;

	fprintf(fp, "[");
	i = 0;
	while (argv && argv[i])
	{
		if (i > 0)
			fprintf(fp, ", ");
		json_str(fp, argv[i]);
		i++;
	}
	fprintf(fp, "]");
}

static void	ast_to_json_node(FILE *fp, t_ast *node, int depth)
{
	if (!node)
	{
		fprintf(fp, "null");
		return ;
	}
	fprintf(fp, "{\n");
	json_indent(fp, depth + 1);
	fprintf(fp, "\"type\": \"%s\"", node_type_str(node->type));
	if (node->type == NODE_COMMAND)
	{
		fprintf(fp, ",\n");
		json_indent(fp, depth + 1);
		fprintf(fp, "\"argv\": ");
		json_argv(fp, node->data.cmd->argv);
		fprintf(fp, ",\n");
		json_indent(fp, depth + 1);
		fprintf(fp, "\"argc\": %d,\n", node->data.cmd->argc);
		json_indent(fp, depth + 1);
		fprintf(fp, "\"assignments\": ");
		json_assignments(fp, node->data.cmd->assignments, depth + 2);
		fprintf(fp, ",\n");
		json_indent(fp, depth + 1);
		fprintf(fp, "\"redirs\": ");
		json_redirs(fp, node->data.cmd->redirs, depth + 2);
	}
	else if (node->type == NODE_PIPE || node->type == NODE_AND
		|| node->type == NODE_OR || node->type == NODE_SEQUENCE)
	{
		fprintf(fp, ",\n");
		json_indent(fp, depth + 1);
		fprintf(fp, "\"left\": ");
		ast_to_json_node(fp, node->data.binary->left, depth + 1);
		fprintf(fp, ",\n");
		json_indent(fp, depth + 1);
		fprintf(fp, "\"right\": ");
		ast_to_json_node(fp, node->data.binary->right, depth + 1);
	}
	else
	{
		fprintf(fp, ",\n");
		json_indent(fp, depth + 1);
		fprintf(fp, "\"child\": ");
		ast_to_json_node(fp, node->data.group->child, depth + 1);
		fprintf(fp, ",\n");
		json_indent(fp, depth + 1);
		fprintf(fp, "\"redirs\": ");
		json_redirs(fp, node->data.group->redirs, depth + 2);
	}
	fprintf(fp, "\n");
	json_indent(fp, depth);
	fprintf(fp, "}");
}

static void	update_manifest(const char *json_filename)
{
	FILE		*fp;
	const char	*manifest_path;
	const char	*tmp_path;
	char		line[512];
	char		*p;
	size_t		len;

	manifest_path = AST_DIR "/manifest.json";
	tmp_path = AST_DIR "/manifest.json.tmp";
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
			p = line;
			while (*p == ' ' || *p == '\t')
				p++;
			if (*p == '[' || *p == ']' || *p == '\0' || *p == '\n')
				continue ;
			len = strlen(p);
			while (len > 0 && (p[len - 1] == '\n' || p[len - 1] == '\r'
					|| p[len - 1] == ' '))
				p[--len] = '\0';
			if (len == 0)
				continue ;
			if (p[len - 1] == ',')
				p[--len] = '\0';
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
 * @details Creates viz/ast/ if missing, writes a timestamped file,
 *                   and updates viz/ast/manifest.json for the static viewer.
 *                   The tok_file field links this AST dump to its tokenization file.
 */
char	*ast_to_json(t_ast *ast, const char *input, const char *tok_file)
{
	char			filename[64];
	char			filepath[128];
	FILE			*fp;
	struct timespec	ts;

	mkdir(AST_DIR, 0755);
	clock_gettime(CLOCK_REALTIME, &ts);
	snprintf(filename, sizeof(filename), "ast_%ld_%06ld.json",
		(long)ts.tv_sec, ts.tv_nsec / 1000);
	snprintf(filepath, sizeof(filepath), "%s/%s", AST_DIR, filename);
	fp = fopen(filepath, "w");
	if (!fp)
	{
		perror("ast_to_json: fopen");
		return (NULL);
	}
	fprintf(fp, "{\n");
	fprintf(fp, "  \"input\": ");
	json_str(fp, input);
	fprintf(fp, ",\n");
	fprintf(fp, "  \"tok_file\": ");
	json_str(fp, tok_file);
	fprintf(fp, ",\n");
	fprintf(fp, "  \"tree\": ");
	ast_to_json_node(fp, ast, 1);
	fprintf(fp, "\n}\n");
	fclose(fp);
	update_manifest(filename);
	return (strdup(filepath));
}

#endif /* FT_EXTRA_VERBOSE */
