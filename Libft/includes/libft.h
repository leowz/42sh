/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   libft.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: zweng <marvin@42.fr>                       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2017/11/09 11:44:47 by zweng             #+#    #+#             */
/*   Updated: 2026/04/28 15:36:56 by jguillem         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LIBFT_H
# define LIBFT_H

# include <unistd.h>
# include <stdlib.h>
# include <string.h>

# define BUFF_SIZE 	(320)
# define MAX_FD 	(2048)
# define FUN_SUCS 	(1)
# define FUN_FAIL	(0)
# define TRUE	 	(1)
# define FALSE		(0)
# define EXT_SUCS	(0)
# define EXT_FAIL	(1)
# define FUN_EXT(a)	(a == 0 ? 1 : 0)
# define ABS(a,b)	(a > b ? a : b)

typedef unsigned char bool;

typedef struct s_list
{
	void			*content;
	struct s_list	*next;
}					t_list;

typedef struct s_dlist
{
	void			*content;
	struct s_dlist	*prev;
	struct s_dlist	*next;
}					t_dlist;

typedef struct s_btree
{
	struct s_btree	*left;
	struct s_btree	*right;
	void			*item;
}	t_btree;

typedef struct s_hash_entry
{
	char	*key;
	void	*value;
}	t_hash_entry;

typedef struct s_hash
{
	t_list	**buckets;
	size_t	bucket_count;
	size_t	size;
}	t_hash;

typedef struct s_fd
{
	int				open_flag;
	size_t			bytes_read;
	char			*buf;
	size_t			buf_size;
	int				ret_flag;
}					t_fd;

char				*ft_strcat(char *s1, const char *s2);
void				*ft_memset(void *b, int c, size_t len);
void				ft_bzero(void *s, size_t n);
void				*ft_memcpy(void *dst, const void *src, size_t n);
void				*ft_memccpy(void *dst, const void *src, int c, size_t n);
void				*ft_memmove(void *dst, const void *src, size_t len);
void				*ft_memchr(const void *s, int c, size_t n);
int					ft_memcmp(const void *s1, const void *s2, size_t n);
size_t				ft_strlen(const char *s);
char				*ft_strdup(const char *s1);
char				*ft_strcpy(char *dst, const char *src);
char				*ft_strncpy(char *dst, const char *src, size_t len);
char				*ft_strcat(char *s1, const char *s2);
char				*ft_strncat(char *s1, const char *s2, size_t n);
size_t				ft_strlcat(char *dst, const char *src, size_t size);
char				*ft_strchr(const char *s, int c);
char				*ft_strrchr(const char *s, int c);
char				*ft_strstr(const char *haystack, const char *needle);
char				*ft_strnstr(const char *haystack, const char *needle,
						size_t len);
int					ft_strcmp(const char *s1, const char *s2);
int					ft_strncmp(const char *s1, const char *s2, size_t n);
int					ft_atoi(const char *str);
int					ft_isalpha(int c);
int					ft_isdigit(int c);
int					ft_isalnum(int c);
int					ft_isascii(int c);
int					ft_isprint(int c);
int					ft_isupper(int c);
int					ft_islower(int c);
int					ft_toupper(int c);
int					ft_tolower(int c);
void				*ft_memalloc(size_t size);
void				ft_memdel(void **ap);
char				*ft_strnew(size_t size);
void				ft_strdel(char **as);
void				ft_strclr(char *s);
void				ft_striter(char *s, void (*f)(char *));
void				ft_striteri(char *s, void (*f)(unsigned int, char *));
char				*ft_strmap(char const *s, char (*f)(char));
char				*ft_strmapi(char const *s, char (*f)(unsigned int, char));
int					ft_strequ(char const *s1, char const *s2);
int					ft_strnequ(char const *s1, char const *s2, size_t n);
char				*ft_strsub(char const *s, unsigned int start, size_t len);
char				*ft_strjoin(char const *s1, char const *s2);
char				*ft_strtrim(char const *s);
char				**ft_strsplit(char const *s, char c);
char				*ft_itoa_base(long long int n, int base);
char				*ft_itoa(int n);
void				ft_putchar(char c);
void				ft_putstr(char const *s);
void				ft_putendl(char const *s);
void				ft_putnbr(int n);
void				ft_putnbr_base(int nbr, char *base);
void				ft_putchar_fd(char c, int fd);
void				ft_putstr_fd(char const *str, int fd);
void				ft_putendl_fd(char const *s, int fd);
void				ft_putnbr_fd(int n, int fd);
char				*ft_strrev(char const *s);
char				*ft_strlastchrp(char const *s);
t_list				*ft_lstnew(void *content);
void				ft_lstdelone(t_list **alst, void (*del)(void *));
void				ft_lstdel(t_list **alst, void (*del)(void *));
void				ft_lstadd(t_list **alst, t_list *new);
void				ft_lstiter(t_list *lst, void (*f)(t_list *elem));
t_list				*ft_lstmap(t_list *lst, t_list *(*f)(t_list *elem));
void				ft_lstappend(t_list **alst, t_list *node);
t_list				*ft_lstlast(t_list *lst);
void				ft_lstdellast(t_list **alst, void (*del)(void *));
void				ft_lstprint(t_list *alst);
int					ft_abs(int n);
size_t				ft_lstsize(t_list *lst);
int					get_next_line(const int fd, char **line);
char				*ft_ternary(int x, const char *s1, const char *s2);
int					ft_ternary_int(int x, const int n1, const int n2);
t_dlist				*ft_dlstnew(void *content);
void				ft_dlstadd_front(t_dlist **head, t_dlist *node);
t_dlist				*ft_dlstadd_back(t_dlist **head, t_dlist *node);
void				ft_dlstdelone(t_dlist *node, void (*del)(void *));
void				ft_dlstclear(t_dlist **head, void (*del)(void *));
size_t				ft_dlstsize(t_dlist *head);
t_btree				*btree_create_node(void *item);
void				btree_insert_data(t_btree **root, void *item,
						int (*cmpf)(void *, void *));
void				btree_clear(t_btree **root, void (*del)(void *));
unsigned long		ft_strhash(const char *s);
t_hash				*ft_hash_new(size_t bucket_count);
void				ft_hash_destroy(t_hash *h, void (*del)(void *));
void				ft_hash_clear(t_hash *h, void (*del)(void *));
size_t				ft_hash_size(const t_hash *h);
void				*ft_hash_get(const t_hash *h, const char *key);
int					ft_hash_set(t_hash *h, const char *key, void *value,
						void **old_out);
int					ft_hash_delete(t_hash *h, const char *key, void **old_out);
void				ft_hash_iter(const t_hash *h,
						void (*f)(const char *, void *, void *),
						void *userdata);
t_list				*hash_lookup_node(const t_hash *h, const char *key,
						size_t *bucket_idx, t_list **prev_out);
#endif
