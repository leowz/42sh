#include "42sh.h"
#include "expander.h"

char	*expand_word(t_shell *shell, const char *word)
{
	(void)shell;
	return (ft_strdup(word));
}

char	**expand_word_to_fields(t_shell *shell, const char *word)
{
	char	**fields;

	(void)shell;
	fields = malloc(sizeof(char *) * 2);
	if (!fields)
		return (NULL);
	fields[0] = ft_strdup(word);
	fields[1] = NULL;
	return (fields);
}

