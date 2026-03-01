#include "libft.h"

int main(void) {
	char const *s1 = "world";
	char * res;

	res = ft_strjoin(s1, " curel world...");
	printf("%s\n", res);
	return 0;
}
