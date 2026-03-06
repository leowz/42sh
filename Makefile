# ----- Toolchain -----
CC			= c99
NAME		= 42sh
TEST_NAME	= 42sh_test

# ----- Paths -----
LIB_PATH	= Libft
LIB			= $(LIB_PATH)/libft.a
HEADER_PATH	= includes $(LIB_PATH)/includes
SRC_PATH	= srcs
OBJ_PATH	= obj
TEST_PATH	= tests

# ----- Base flags -----
CFLAGS		= $(foreach D, $(HEADER_PATH), -I$(D)) \
			  -D_POSIX_C_SOURCE=200809L \
			  -Wall -Wextra -Werror \
			  -MD -MP # -std=c99

LDFLAGS		= -L$(LIB_PATH) -lft -lreadline -ltermcap

# ----- Debug flags (only for `make debug`) -----
DBGFLAGS	= -g -fsanitize=address -fsanitize=undefined -fsanitize=leak -DDEBUG

# ----- Test feature flags -----
# Each flag enables one test suite.  When a suite passes permanently:
#   1. Remove #ifdef/#else/#endif in the test file.
#   2. Remove #ifdef/#endif around MU_RUN in test_runner.c.
#   3. Remove the corresponding -D flag here.
TEST_FLAGS	=
# For example: Uncomment once srcs/history/ is implemented:
TEST_FLAGS	= -DTEST_HISTORY_ENABLED
TEST_FLAGS += -DTEST_LEXER_ENABLED
TEST_FLAGS += -DTEST_LIST_ENABLED
TEST_FLAGS += -DTEST_DLIST_ENABLED

# ----- Source discovery (recursive) -----
SRCS		= $(shell find $(SRC_PATH) -name '*.c')
OBJS		= $(patsubst $(SRC_PATH)/%.c, $(OBJ_PATH)/%.o, $(SRCS))
DEPS		= $(OBJS:.o=.d)

# Test sources: all .c under tests/ + project sources except main.c
TEST_SRCS	= $(shell find $(TEST_PATH) -name '*.c' 2>/dev/null)
PROJ_NO_MAIN= $(filter-out $(SRC_PATH)/main.c, $(SRCS))
TEST_OBJS	= $(patsubst $(TEST_PATH)/%.c, $(OBJ_PATH)/test/%.o, $(TEST_SRCS)) \
			  $(patsubst $(SRC_PATH)/%.c,  $(OBJ_PATH)/%.o,      $(PROJ_NO_MAIN))

# ----- Colors -----
GREEN		:= "\033[1;32m"
RED			:= "\033[1;31m"
CYAN		:= "\033[1;36m"
WHITE		:= "\033[1;37m"
EOC			:= "\033[0;0m"

# ==================
# Rules
# ==================

all: $(NAME)

$(NAME): $(LIB) $(OBJS)
	@$(CC) $(OBJS) $(LDFLAGS) -o $@
	@printf $(GREEN)"$(NAME): OK\n"$(EOC)

# Debug build: full rebuild with ASAN + UBSan + DEBUG define
debug: fclean
	@$(MAKE) --no-print-directory \
		CFLAGS="$(CFLAGS) $(DBGFLAGS)" \
		LDFLAGS="$(LDFLAGS) $(DBGFLAGS)" \
		$(NAME)
	@printf $(GREEN)"$(NAME): debug build OK\n"$(EOC)

# Test build: compile test binary (TEST_FLAGS enables individual suites)
test: $(LIB) $(TEST_OBJS)
	@$(CC) $(TEST_OBJS) $(LDFLAGS) -o $(TEST_NAME)
	@printf $(GREEN)"running tests...\n"$(EOC)
	@./$(TEST_NAME)

# ---- Object rules ----

# Project objects: srcs/**/*.c  ->  obj/**/*.o
$(OBJ_PATH)/%.o: $(SRC_PATH)/%.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c $< -o $@
	@printf $(CYAN)"  CC  $<\n"$(EOC)

# Test objects: tests/*.c  ->  obj/test/*.o  (TEST_FLAGS applied here)
$(OBJ_PATH)/test/%.o: $(TEST_PATH)/%.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) $(TEST_FLAGS) -c $< -o $@
	@printf $(CYAN)"  CC  $< [test]\n"$(EOC)

# ---- Library ----

$(LIB):
	@$(MAKE) -C $(LIB_PATH) --no-print-directory

# ---- Housekeeping ----

clean:
	@rm -rf $(OBJ_PATH)
	@printf $(GREEN)"$(NAME): objects removed\n"$(EOC)
	@$(MAKE) -C $(LIB_PATH) clean --no-print-directory

fclean: clean dclean
	@rm -f $(NAME) $(TEST_NAME)
	@printf $(GREEN)"$(NAME): binaries removed\n"$(EOC)
	@$(MAKE) -C $(LIB_PATH) fclean --no-print-directory

re: fclean all

# ---- Documentation ----
# Two Doxyfiles:
#   root Doxyfile  → 42sh core     → docs/core/
#   tests/Doxyfile → test suite    → docs/test/
# docs/index.html is the static viewer (version-controlled, never removed).
# All generation logic lives in scripts/gendocs.sh.

docs:
	@./scripts/gendocs.sh docs

html: docs
	@./scripts/gendocs.sh html

dclean:
	@printf $(RED)"Removing generated docs (preserving docs/index.html)...\n"$(EOC)
	@rm -rf docs/core docs/test docs/pages.json
	@printf $(RED)"Docs removed.\n"$(EOC)

serve: html
	@printf $(GREEN)"Serving docs at http://localhost:8080\n"$(EOC)
	@printf $(GREEN)"Press Ctrl+C to stop.\n"$(EOC)
	@cd docs && python3 -m http.server 8080

# ---- Help ----

help:
	@printf $(WHITE)"42sh Makefile\n"$(EOC)
	@printf "\n"
	@printf "Targets:\n"
	@printf "  "$(GREEN)"all"$(EOC)"     — build $(NAME) (default)\n"
	@printf "  "$(GREEN)"debug"$(EOC)"   — build with AddressSanitizer + UBSan\n"
	@printf "  "$(GREEN)"test"$(EOC)"    — build and run the test suite\n"
	@printf "  "$(GREEN)"docs"$(EOC)"    — generate man pages (core + tests)\n"
	@printf "  "$(GREEN)"html"$(EOC)"    — convert to HTML + build docs/pages.json\n"
	@printf "  "$(GREEN)"serve"$(EOC)"   — build HTML and serve at localhost:8080\n"
	@printf "  "$(RED)"dclean"$(EOC)"  — remove docs/core, docs/test, docs/pages.json\n"
	@printf "  "$(RED)"clean"$(EOC)"   — remove object files\n"
	@printf "  "$(RED)"fclean"$(EOC)"  — remove objects, binaries, and docs\n"
	@printf "  "$(CYAN)"re"$(EOC)"      — rebuild from scratch\n"
	@printf "  "$(CYAN)"norme"$(EOC)"   — run norminette\n"
	@printf "  "$(CYAN)"help"$(EOC)"    — show this message\n"
	@printf "\n"
	@printf "Usage:\n"
	@printf "  make           # build 42sh\n"
	@printf "  make test      # run unit tests\n"
	@printf "  make serve     # build docs and open viewer\n"
	@printf "  make docs      # generate man pages only\n"
	@printf "  make html      # build HTML docs + pages.json\n"

-include $(DEPS)

.PHONY: all debug test docs html dclean serve clean fclean re norme help
