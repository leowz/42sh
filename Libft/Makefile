# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: zweng <marvin@42.fr>                       +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2017/11/09 18:11:43 by zweng             #+#    #+#              #
#    Updated: 2026/03/18 19:50:54 by jguillem         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

CC = c99

# ----- part to change -----

NAME 			= libft.a

HEADER_PATH 	= includes
H_STD 			= $(HEADER_PATH)/libft.h

SRC_PATH 		= srcs
STD_PATH 		= $(SRC_PATH)

SRC_NAME_STD = $(notdir $(foreach D, $(STD_PATH), $(wildcard $(D)/*.c)))

OBJ_PATH =  obj
OBJ_NAME =  $(SRC_NAME_STD:.c=.o)

CPPFLAGS = -I$(HEADER_PATH) 
CFLAGS = -Wall -Wextra -Werror

# ----- part automatic -----
SRCS := $(addprefix $(STD_PATH)/,$(SRC_NAME_STD))
OBJS := $(addprefix $(OBJ_PATH)/,$(OBJ_NAME))

# ----- Colors -----
BLACK:="\033[1;30m"
RED:="\033[1;31m"
GREEN:="\033[1;32m"
CYAN:="\033[1;35m"
PURPLE:="\033[1;36m"
WHITE:="\033[1;37m"
EOC:="\033[0;0m"
#  # ==================

# ----- part rules -----

all: $(NAME)

$(NAME): $(OBJS) 
	@ar rc $(NAME) $(OBJS)
	@ranlib $(NAME)
	@printf $(GREEN)"\r$(NAME) finish\n"$(END)

$(OBJ_PATH)/%.o: $(STD_PATH)/%.c $(H_STD) | $(OBJ_PATH)
	@$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<
	@printf $(GREEN)"\rcompiling %s"$(GREEN) $<

$(OBJ_PATH)/%.o: $(ARRAY_PATH)/%.c $(H_STD) $(H_ARRAY) | $(OBJ_PATH)
	@$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<
	@printf $(GREEN)"\rcompiling %s"$(GREEN) $<

$(OBJ_PATH)/%.o: $(PRINTF_PATH)/%.c $(H_STD) $(H_PRINTF) | $(OBJ_PATH)
	@$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ -c $<
	@printf $(GREEN)"\rcompiling %s"$(GREEN) $<

$(OBJ_PATH):
	@mkdir $(OBJ_PATH) 2> /dev/null

clean:
	@rm -f $(OBJS)
	@rmdir $(OBJ_PATH) 2> /dev/null || true
	@printf $(GREEN)"$(NAME) clean\n"$(EOC)

fclean: clean
	@/bin/rm -f $(NAME)
	@printf $(GREEN)"$(NAME) fclean\n"$(EOC)

re: fclean all

norme:
	@norminette $(SRCS)
	@norminette $(HEADER_PATH)/*.h

.PHONY: clean fclean all re norme
