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
			  -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE \
			  -Wall -Wextra -Werror \
			  -MD -MP


LDFLAGS		= -L$(LIB_PATH) -lft -lreadline -ltermcap

# ----- Debug flags (only for `make debug`) -----
DBGFLAGS	= -g -DDEBUG #-fsanitize=address -fsanitize=undefined -fsanitize=leak
DBGFLAGS	+= -DFT_EXTRA_VERBOSE

# ----- Test feature flags -----
# Each flag enables one test suite.  When a suite passes permanently:
#   1. Remove #ifdef/#else/#endif in the test file.
#   2. Remove #ifdef/#endif around MU_RUN in test_runner.c.
#   3. Remove the corresponding -D flag here.
TEST_FLAGS	=
# For example: Uncomment once srcs/history/ is implemented:
TEST_FLAGS += -DTEST_EXECUTOR_ENABLED
# TEST_FLAGS	= -DTEST_HISTORY_ENABLED
TEST_FLAGS += -DTEST_LEXER_ENABLED
TEST_FLAGS += -DTEST_LIST_ENABLED
TEST_FLAGS += -DTEST_DLIST_ENABLED
TEST_FLAGS += -DTEST_BTREE_ENABLED
TEST_FLAGS += -DTEST_PARSER_ENABLED
TEST_FLAGS += -DTEST_BUILTIN_ECHO_ENABLED
TEST_FLAGS += -DTEST_BUILTIN_CD_ENABLED
TEST_FLAGS += -DTEST_EXPANDER_ENABLED
TEST_FLAGS += -DTEST_VARIABLES_ENABLED
TEST_FLAGS += -DTEST_JOB_CONTROL_ENABLED
TEST_FLAGS += -DTEST_BUILTIN_EXIT_ENABLED
TEST_FLAGS += -DTEST_BUILTIN_TYPE_ENABLED
TEST_FLAGS += -DTEST_HEREDOC_BUGS_ENABLED
TEST_FLAGS += -DTEST_BUILTIN_HASH_ENABLED

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
	@printf "\n"$(CYAN)"  $(CC) $(OBJS) $(LDFLAGS) -o $@"$(EOC)"\n"
	@printf $(GREEN)"$(NAME): OK\n"$(EOC)

# Debug build: full rebuild with ASAN + UBSan + DEBUG define
debug: fclean
	@$(MAKE) --no-print-directory \
		CFLAGS="$(CFLAGS) $(DBGFLAGS)" \
		LDFLAGS="$(LDFLAGS) $(DBGFLAGS)" \
		$(NAME)
	@printf $(GREEN)"$(NAME): debug build OK\n"$(EOC)

# Test build: compile test binary (TEST_FLAGS enables individual suites)
# -lutil: forkpty(3), used by test_builtin_exit to drive an interactive shell.
# $(NAME) is a prerequisite: several suites exec ./42sh end-to-end, so the
# shell binary must exist before the tests run (CI invokes `make test` alone).
test: $(NAME) $(LIB) $(TEST_OBJS)
	@$(CC) $(TEST_OBJS) $(LDFLAGS) -lutil -o $(TEST_NAME)
	@printf "\n"$(GREEN)"running tests...\n"$(EOC)
	@./$(TEST_NAME)

# Integration tests: run each line in tests/integration/cases.txt through
# ./42sh -c and bash --posix -c, compare stdout/stderr/exit, then re-run
# the same line through ./42sh under valgrind (bash is never instrumented).
# Use `make integration-quick` (or VALGRIND=0) to skip the valgrind pass.
integration: $(NAME)
	@printf $(GREEN)"running integration tests...\n"$(EOC)
	@bash $(TEST_PATH)/integration/run.sh

integration-quick: $(NAME)
	@printf $(GREEN)"running integration tests (no valgrind)...\n"$(EOC)
	@VALGRIND=0 bash $(TEST_PATH)/integration/run.sh

# ---- Object rules ----

# Project objects: srcs/**/*.c  ->  obj/**/*.o
$(OBJ_PATH)/%.o: $(SRC_PATH)/%.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -c $< -o $@
	@printf $(CYAN)"  $(CC) $(CFLAGS) -c $< -o $@\r"$(EOC)

# Test objects: tests/*.c  ->  obj/test/*.o  (TEST_FLAGS applied here)
$(OBJ_PATH)/test/%.o: $(TEST_PATH)/%.c
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) $(TEST_FLAGS) -c $< -o $@
	@printf $(CYAN)"  $(CC) $(CFLAGS) $(TEST_FLAGS) -c $< -o $@ [test]\r"$(EOC)

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

valgrind:
	valgrind --gen-suppressions=yes --suppressions=suppression.file --track-fds=yes --leak-check=full --show-leak-kinds=all --track-origins=yes --log-file=valgrind-out.txt ./42sh

# ---- Documentation ----
# Two Doxyfiles produce XML consumed by Sphinx/Breathe:
#   root Doxyfile  → 42sh core     → docs/core/xml
#   tests/Doxyfile → test suite    → docs/test/xml
# Sphinx (with RTD theme) builds the final HTML from docs/*.rst.

docs:
	@./scripts/gendocs.sh docs

html: docs
	@printf $(GREEN)"Building Sphinx docs...\n"$(EOC)
	@sphinx-build -b html docs docs/_build/html
	@printf $(GREEN)"HTML docs ready in docs/_build/html\n"$(EOC)

dclean:
	@printf $(RED)"Removing generated docs...\n"$(EOC)
	@rm -rf docs/_doxygen docs/_build docs/pages.json
	@printf $(RED)"Docs removed.\n"$(EOC)

serve: html
	@printf $(GREEN)"Serving docs at http://localhost:8080\n"$(EOC)
	@printf $(GREEN)"Press Ctrl+C to stop.\n"$(EOC)
	@cd docs/_build/html && python3 -m http.server 8080

# ---- Git hooks ----

install-hooks:
	@git config core.hooksPath .githooks
	@printf $(GREEN)"Git hooks installed (core.hooksPath → .githooks)\n"$(EOC)

# ---- Help ----

help:
	@printf $(WHITE)"42sh Makefile\n"$(EOC)
	@printf "\n"
	@printf "Targets:\n"
	@printf "  "$(GREEN)"all"$(EOC)"           - build $(NAME) (default)\n"
	@printf "  "$(GREEN)"debug"$(EOC)"         - build with AddressSanitizer + UBSan\n"
	@printf "  "$(GREEN)"test"$(EOC)"          - build and run the test suite\n"
	@printf "  "$(GREEN)"integration"$(EOC)"   - run integration tests (42sh vs bash --posix, +valgrind)\n"
	@printf "  "$(GREEN)"integration-quick"$(EOC)" - same, but skip the valgrind pass\n"
	@printf "  "$(GREEN)"docs"$(EOC)"          - generate Doxygen XML + man pages\n"
	@printf "  "$(GREEN)"html"$(EOC)"          - build Sphinx HTML docs (RTD theme)\n"
	@printf "  "$(GREEN)"serve"$(EOC)"         - build and serve docs at localhost:8080\n"
	@printf "  "$(RED)"dclean"$(EOC)"        - remove all generated docs\n"
	@printf "  "$(RED)"clean"$(EOC)"         - remove object files\n"
	@printf "  "$(RED)"fclean"$(EOC)"        - remove objects, binaries, and docs\n"
	@printf "  "$(CYAN)"re"$(EOC)"            - rebuild from scratch\n"
	@printf "  "$(CYAN)"install-hooks"$(EOC)" - set up Git hooks from .githooks/\n"
	@printf "  "$(CYAN)"help"$(EOC)"          - show this message\n"
	@printf "\n"
	@printf "Usage:\n"
	@printf "  make           # build 42sh\n"
	@printf "  make test      # run unit tests\n"
	@printf "  make serve     # build and serve docs locally\n"
	@printf "  make docs      # generate Doxygen XML + man pages\n"
	@printf "  make html      # build Sphinx HTML (RTD theme)\n"

-include $(DEPS)

.PHONY: all debug test integration integration-quick docs html dclean serve clean fclean re help install-hooks
