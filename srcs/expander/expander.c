/**
 * @file expander.c
 * @brief Word expansion functions for the 42sh shell.
 *
 * @author pulgamecanica wengzhang
*/

#include "42sh.h"
#include "expander.h"
#include "ast.h"

/* TODO P3: implement word expansion ($VAR, quotes, field splitting, globbing) */

char	*expand_word(t_shell *shell, const char *word)
{
	(void)shell;
	if (!word)
		return (NULL);
	return (ft_strdup(word));
}

char	**expand_word_to_fields(t_shell *shell, const char *word)
{
	char	**fields;

	(void)shell;
	if (!word)
		return (NULL);
	fields = malloc(sizeof(char *) * 2);
	if (!fields)
		return (NULL);
	fields[0] = ft_strdup(word);
	fields[1] = NULL;
	return (fields);
}

int	expand_command(t_shell *shell, t_cmd *cmd)
{
	(void)shell;
	(void)cmd;
	return (0);
}
