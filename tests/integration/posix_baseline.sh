#!/usr/bin/env bash
# ============================================================================
# posix_baseline.sh -- POSIX shell baseline behaviour
# ============================================================================
# A wide-coverage replay of the basic POSIX shell behaviours the project
# subject mandates: command resolution, redirections, pipelines, control
# operators, the must-have built-ins (echo, exit, cd, pwd, type, set, unset,
# export), shell variables, and one-shot environment assignments.
#
# Every case runs the same script through 42sh and `bash --posix` and
# compares:
#   - exit status,
#   - stdout (byte-exact),
#   - stderr presence (both produced a diagnostic, or neither did) -- exact
#     stderr wording is not compared since each shell legitimately uses its
#     own name.
#
# Cases that create files run in their own scratch directory so each one is
# isolated. The interactive items (heredoc with the `> ` continuation
# prompt, job-control PTY interaction, terminal signals) are NOT here --
# they belong to the PTY harness and the signal suite.
#
# Usage:   bash tests/integration/posix_baseline.sh
# Exit:    0 if 42sh matches bash on every case, 1 otherwise.
# ============================================================================

set -u

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SH="${SHELL_BIN:-$ROOT/42sh}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

if [ -t 1 ]; then
	G=$'\033[1;32m'; R=$'\033[1;31m'; C=$'\033[1;36m'; Z=$'\033[0m'
else
	G=; R=; C=; Z=
fi

[ -x "$SH" ] || { echo "posix_baseline.sh: shell not built: $SH" >&2; exit 2; }
command -v bash >/dev/null 2>&1 || { echo "posix_baseline.sh: no bash" >&2; exit 2; }

total=0; pass=0; fail=0; failed=""

# b_case LABEL SCRIPT -- run SCRIPT under 42sh and bash --posix in a fresh
# scratch directory; compare stdout, exit status, and stderr presence.
b_case() {
	local label="$1" script="$2" brc src wd report ok
	total=$((total + 1))
	wd="$TMP/case$total"
	rm -rf "$wd"; mkdir -p "$wd"
	(cd "$wd" && bash --posix -c "$script") >"$TMP/b.out" 2>"$TMP/b.err"
	brc=$?
	rm -rf "$wd"; mkdir -p "$wd"
	(cd "$wd" && "$SH" -c "$script") >"$TMP/s.out" 2>"$TMP/s.err"
	src=$?
	ok=1; report=""
	if [ "$brc" != "$src" ]; then
		ok=0
		report+="    exit:   bash=$brc 42sh=$src"$'\n'
	fi
	if ! cmp -s "$TMP/b.out" "$TMP/s.out"; then
		ok=0
		report+="    stdout differs (- bash, + 42sh):"$'\n'
		report+="$(diff -u "$TMP/b.out" "$TMP/s.out" | sed 's/^/      /')"$'\n'
	fi
	if [ -s "$TMP/b.err" ] && [ ! -s "$TMP/s.err" ]; then
		ok=0
		report+="    bash produced a diagnostic, 42sh produced none"$'\n'
	elif [ ! -s "$TMP/b.err" ] && [ -s "$TMP/s.err" ]; then
		ok=0
		report+="    42sh produced an unexpected diagnostic"$'\n'
	fi
	if [ "$ok" = "1" ]; then
		pass=$((pass + 1))
		printf "  %sPASS%s  %s\n" "$G" "$Z" "$label"
	else
		fail=$((fail + 1))
		failed+="[$label] "
		printf "  %sFAIL%s  %s\n%s" "$R" "$Z" "$label" "$report"
	fi
}

# b_match LABEL REGEX SCRIPT -- run SCRIPT in 42sh, check that stdout matches
# REGEX (extended regex). Used for cases whose output is intentionally random
# (e.g. urandom-fed pipelines) so a byte-exact compare against bash is wrong;
# the contract is "the shape of the output, not its bytes".
b_match() {
	local label="$1" regex="$2" script="$3" got
	total=$((total + 1))
	got="$("$SH" -c "$script" 2>/dev/null)"
	if printf '%s' "$got" | grep -Eq "$regex"; then
		pass=$((pass + 1))
		printf "  %sPASS%s  %s  (got: %s)\n" "$G" "$Z" "$label" "$got"
	else
		fail=$((fail + 1))
		failed+="[$label] "
		printf "  %sFAIL%s  %s  (got: [%s], wanted regex: %s)\n" \
			"$R" "$Z" "$label" "$got" "$regex"
	fi
}

# b_stdin LABEL EXPECTED INPUT -- feed INPUT on stdin (not via -c), compare
# stdout against EXPECTED (literal). For cases that need the REPL loop, such
# as a sequence of lines where state from one line affects the next.
b_stdin() {
	local label="$1" want="$2" input="$3" wd got
	total=$((total + 1))
	wd="$TMP/case$total"
	rm -rf "$wd"; mkdir -p "$wd"
	got="$(cd "$wd" && printf '%s' "$input" | "$SH" 2>/dev/null)"
	if [ "$got" = "$want" ]; then
		pass=$((pass + 1))
		printf "  %sPASS%s  %s\n" "$G" "$Z" "$label"
	else
		fail=$((fail + 1))
		failed+="[$label] "
		printf "  %sFAIL%s  %s\n" "$R" "$Z" "$label"
		printf "        want: [%s]\n" "$want"
		printf "        got:  [%s]\n" "$got"
	fi
}

# ============================================================================
printf "%s== blank inputs and whitespace tolerance ==%s\n" "$C" "$Z"
# ============================================================================
b_stdin "empty command does nothing" "" $'\nexit\n'
b_stdin "single space does nothing" "" $' \nexit\n'
b_stdin "tabs and spaces do nothing" "" $'\t \t   \nexit\n'
b_case "extra spaces between args" \
	'   echo	hello    world	'
b_case "extra spaces at end of command" \
	'echo hello   '

# ============================================================================
printf "%s== command resolution: PATH, absolute, errors ==%s\n" "$C" "$Z"
# ============================================================================
b_case "PATH-resolved echo" \
	'echo hi; echo rc=$?'
b_case "absolute path: /bin/echo" \
	'/bin/echo abc; echo rc=$?'
b_case "PATH-resolved ls" \
	'ls / >/dev/null; echo rc=$?'
b_case "absolute path: /bin/ls" \
	'/bin/ls / >/dev/null; echo rc=$?'
b_case "combined flags: /bin/ls -alF" \
	'/bin/ls -alF / >/dev/null; echo rc=$?'
b_case "separate flags: /bin/ls -l -a -F" \
	'/bin/ls -l -a -F / >/dev/null; echo rc=$?'
b_case "command not found returns 127" \
	'doesnotexist_xyz_4242; echo rc=$?'
b_case "non-existent absolute path returns 127" \
	'/sbin/yubikey_shell_zzz_xyz; echo rc=$?'
b_case "non-executable file returns 126" \
	'printf x > here; chmod 000 here; ./here; echo rc=$?'
b_case "non-existent option returns non-zero" \
	'ls --no-such-flag-here >/dev/null 2>&1; echo rc=$?'

# ============================================================================
printf "%s== redirections and fd manipulation ==%s\n" "$C" "$Z"
# ============================================================================
b_case "stdout > file" \
	'echo hi > out; cat out'
b_case "stdout >> file (append)" \
	'echo a > out; echo b >> out; cat out'
b_case "stdin < file" \
	'echo hi > in; cat < in'
b_case "redirect chain: > and < and >> together" \
	'ls / > listing; < listing cat -e >> listing; wc -l listing'
b_case "fd dup: 2>&1 then >/dev/null" \
	'ls zzznope . 2>&1 >/dev/null'
b_case "fd dup: >/dev/null then 2>&1 silences both" \
	'ls zzznope . >/dev/null 2>&1; echo rc=$?'
b_case "multi-target: >out >&2 2>err lets each fd land" \
	'echo 1 >out >&2 2>err; echo 2 >out 2>err; cat err; cat out'
b_case "close stdin then read missing file" \
	'cat <&- nope_xyz_42'
b_case "read from invalid fd 4" \
	'cat <&4'
b_case "high fd target write then read" \
	'echo content > stash; cat stash'
# Two literal-as-printed PDF probes: after writing a file via `>`, run
# `cat` with three unrelated args (none of which exist). The contract is
# that the shell SURVIVES (cat errors out per-arg but the shell stays
# responsive). Output is empty on both shells; only stderr wording differs.
b_case "cat with three non-existent args, after a successful >" \
	'echo non-standard fd > dup_fd; cat 4 non-standard fd 2>/dev/null; echo rc=$?'
b_case "cat with two non-existent args, after a successful >" \
	'echo abc > redir_one_to_all; cat 9 abc 2>/dev/null; echo rc=$?'
b_case "stdout closed, in sequence with another echo" \
	'echo a >&-; echo b'
b_case "stdout closed, in a pipeline" \
	'echo a >&- | echo b'
b_case "stdout closed, in &&" \
	'echo a >&- && echo b'
b_case "stdout closed, in ||" \
	'echo a >&- || echo b'

# ============================================================================
printf "%s== pipelines ==%s\n" "$C" "$Z"
# ============================================================================
b_case "single pipe: cat | wc -l" \
	'printf "a\nb\nc\n" | wc -l'
b_case "three-stage pipe with cat -e suffix" \
	'printf "%s\n" 3 1 2 | sort -rn | cat -e'
b_case "exit in pipeline stages, shell survives" \
	'exit 1 | exit 2 | exit 3; echo alive'
b_case "broken-pipe consumer terminates producer" \
	'yes | head -n 3'
b_case "four-stage pipeline ends with sed substitution" \
	'printf "abc\nabc\nzzz\n" | grep abc | wc -l | sed -e s/2/YES/g'
b_case "four-stage binary pipeline (deterministic input)" \
	'base64 < /dev/zero | head -c 1000 | grep 42 | wc -l | sed -e s/0/No/g'
b_match "four-stage binary pipeline (random input -> Yes or No)" \
	'^(Yes|No)$' \
	'base64 < /dev/urandom | head -c 1000 | grep 42 | wc -l | sed -e s/1/Yes/g -e s/0/No/g'
b_case "no orphaned base64 after random binary pipeline" \
	'base64 < /dev/urandom | head -c 1000 | grep 42 | wc -l | sed -e s/1/Yes/g -e s/0/No/g >/dev/null; sleep 0.2; ps a | grep "/dev/urandom" | grep -v grep | wc -l'
b_case "sequence then second command file-creating" \
	'ls -1 > before; touch newly_added; ls -1 > after; grep -c newly_added after'
b_case "cat -e displays \$ at line ends" \
	'echo abc > f; echo def >> f; cat -e f'

# ============================================================================
printf "%s== control operators: ; && || ==%s\n" "$C" "$Z"
# ============================================================================
b_case "&& runs second when first succeeds" \
	'true && echo yes'
b_case "&& skips second when first fails" \
	'false && echo no'
b_case "|| runs second when first fails" \
	'false || echo yes'
b_case "|| skips second when first succeeds" \
	'true || echo no'
b_case "ls -l && ls (PDF probe)" \
	'/bin/ls -l / >/dev/null && /bin/ls / >/dev/null; echo rc=$?'
b_case "|| rescue gives status 0" \
	'ls zzznope || echo ok; echo rc=$?'
b_case "|| short-circuit after success leaves status 0" \
	'echo good || echo unused; echo rc=$?'
b_case "false && a || b -> b" \
	'false && echo a || echo b'
b_case "true || a && b -> b" \
	'true || echo a && echo b'
b_case "; sequences run unconditionally" \
	'echo a; false; echo b; echo rc=$?'

# ============================================================================
printf "%s== built-ins: echo / exit / type ==%s\n" "$C" "$Z"
# ============================================================================
b_case "echo with no args is empty line" \
	'echo'
b_case "echo with several args" \
	'echo one two three'
b_case "echo -n suppresses newline" \
	'echo -n hi; echo X'
b_case "exit halts the sequence" \
	'echo a; exit; echo b'
b_case "exit with very large number" \
	'exit 999999999999999999999; echo SHOULD_NOT_PRINT'
b_case "exit with a non-numeric argument" \
	'exit abc'
# Note: `exit 1 2 3` (too-many-args) -- 42sh and `bash --posix -c` diverge
# on shell survival; the case is covered directly by test_builtin_exit.c.
b_case "type identifies a built-in" \
	'type type'
b_case "type identifies an external command" \
	'type ls'
b_case "type combined: builtin + command" \
	'type type ls'

# ============================================================================
printf "%s== built-ins: cd and pwd (paths, OLDPWD, HOME, -L, -P) ==%s\n" "$C" "$Z"
# ============================================================================
b_case "cd to absolute path then pwd" \
	'cd /tmp; pwd'
b_case "cd to a relative path then pwd" \
	'cd /tmp; cd ../etc; pwd'
b_case "cd with no argument goes HOME" \
	'cd /tmp; cd; test "$PWD" = "$HOME" && echo home || echo not_home'
b_case "cd then /bin/pwd then cd-alone then /bin/pwd (HOME via external pwd)" \
	'cd /tmp; /bin/pwd; cd; test "$(/bin/pwd)" = "$HOME" && echo home || echo not_home'
b_case "cd - returns to OLDPWD" \
	'cd /tmp; cd /bin; cd -; pwd'
b_case "cd - alternates each time" \
	'cd /tmp; cd /bin; cd -; cd -; pwd'
b_case "cd -L logical pwd" \
	'cd -L /tmp; pwd'
b_case "cd -P physical pwd" \
	'cd -P /tmp; pwd | head -c1 > /dev/null; echo done'
b_case "cd -L then cd -P .. on one line" \
	'cd -L /tmp; cd -P ..; pwd | head -c1 > /dev/null; echo done'
b_case "cd then /bin/pwd matches pwd builtin" \
	'cd /tmp; /bin/pwd'
b_case "cd to nonexistent fails non-zero" \
	'cd /nonexistent_xyz_42; echo rc=$?'

# ============================================================================
printf "%s== environment: variables, set, unset, export, env ==%s\n" "$C" "$Z"
# ============================================================================
b_case "variable assignment then expansion" \
	'a=hello; echo $a'
b_case "two assignments on one line" \
	'a=hi; b=yo; echo $a $b'
b_case "per-command env: temp survives the same command, not the next" \
	'a=hello b=world; b=42 echo ${a}_${b}; echo $b'
b_case "variables expand into command arguments" \
	'opt=-d; dir=/; ls ${opt} ${dir}'
b_case "unset variable expands to empty" \
	'echo ${empty_xyz_42}|cat -e'
b_case "set lists shell variables" \
	'a=hi; b=yo; set | grep -E "^(a|b)="'
b_case "env shows only environment, not shell-only" \
	'a=hi; b=yo; env | grep -E "^(a|b)=" || echo not-in-env'
b_case "export promotes a shell variable to env" \
	'a=hi; export a; env | grep "^a="'
b_case "export then printenv reads back the value" \
	'b=world; export b; printenv b'
b_case "export with NAME=VALUE assigns and exports in one step" \
	'export TESTV=xyz; env | grep "^TESTV="'
b_case "unset removes from set" \
	'a=hi; unset a; set | grep "^a=" || echo gone'
b_case "unset removes multiple names at once" \
	'a=1; b=2; unset a b; set | grep -E "^(a|b)=" || echo gone'
b_case "empty-value one-shot env assignment" \
	'ONESHOT= env | grep "^ONESHOT="'
b_case "one-shot env assignment is NOT persisted to the next command" \
	'ONESHOT= env | grep "^ONESHOT="; env | grep "^ONESHOT=" || echo gone'
b_case "unset removes exported variables from BOTH env and set" \
	'a=hello; b=world; export a b; unset a b; (env | grep -E "^(a|b)=" || echo env_empty); (set | grep -E "^(a|b)=" || echo set_empty)'
b_case "unset PATH then rebuild, simple command still runs" \
	'unset PATH; PATH=/bin:/usr/bin; mkdir td; echo rc=$?; ls -1 | grep td'
b_case "true ? then false ?" \
	'true; echo $?; false; echo $?'
b_case "rc of last command in a sequence is the rc of the sequence" \
	'true; false; echo rc=$?'

# ============================================================================
printf "%s== heredoc patterns ==%s\n" "$C" "$Z"
# ============================================================================
b_stdin "basic heredoc returns body" $'hello\nworld' \
	$'cat <<END\nhello\nworld\nEND\nexit\n'
b_stdin "heredoc body terminated by exact delim" "bee" \
	$'cat <<EOF\nbee\nEOF\nexit\n'
b_stdin "heredoc inside subshell, piped to sort" $'abb\nabc\nabd' \
	$'(cat <<EOF\nabd\nabc\nabb\nEOF\n) | sort\nexit\n'
# Same sub-shell heredoc but with `cat -e` so the sorted output carries
# the `$` line-end marker (matches the PDF-described expected output:
# "abb$ / abc$ / abd$").
b_stdin "heredoc in subshell with cat -e, sorted output with \$ markers" \
	$'abb$\nabc$\nabd$' \
	$'(cat -e <<EOF\nabd\nabc\nabb\nEOF\n) | sort\nexit\n'
b_stdin "heredoc append: two heredocs feed the same file" $'abc\ndef$\nghi$' \
	$'cat > out << FIN\nabc\nFIN\ncat -e >> out << FIN\ndef\nghi\nFIN\ncat out\nexit\n'
b_stdin "<<- strips leading tabs from body" $'one\ntwo' \
	$'cat <<-EOF\n\tone\n\t\ttwo\n\tEOF\nexit\n'
b_stdin "quoted delim suppresses variable expansion in body" '$HOME' \
	$'cat <<\'EOF\'\n$HOME\nEOF\nexit\n'
# Delimiter split by `\<NL>` -- the PDF shows `cat << EO\` then `> F` as the
# rest of the delimiter, so the effective delim is `EOF`. Covered in the PTY
# harness too; here we feed it via piped stdin.
b_stdin "heredoc delimiter spans a backslash-newline" "hi" \
	$'cat << EO\\\nF\nhi\nEOF\nexit\n'

# `exit 1 2 3` -- shell must reject "too many arguments" and STAY ALIVE.
# `bash --posix -c` exits non-zero (POSIX special-builtin rule), so we
# cannot diff via b_case; instead drive the REPL through piped stdin and
# verify that the shell survived to run the second command.
b_stdin "exit 1 2 3 errors but the shell survives" "still_alive" \
	$'exit 1 2 3\necho still_alive\nexit\n'

# A stability hint left at the bottom of the correction scale: running
# `mkdir kek && chmod 000 kek` should not produce an invalid-free or any
# other memory error. The case PASSES if shell + bash agree (no leaks
# are checked here -- valgrind already covers the shell binary in
# run.sh; this asserts the COMMAND semantics survive).
b_case "mkdir kek && chmod 000 kek -- no invalid free" \
	'mkdir kek && chmod 000 kek; echo rc=$?'

# ============================================================================
# Modular features -- the project subject's optional section lists ~13 of
# these and asks the team to ship at least 6. Each subsection below targets
# one module. Implemented modules run as live tests; not-yet-implemented
# modules keep their probes commented out so the file is a complete spec
# checklist (and a turn-on-when-ready scaffold).
#
# Currently shipped: inhibitors, control groups, command substitution,
#                    tilde (subset of additional expansion), aliases,
#                    hash table, test/[ builtin.   (7 modules; subject
#                    wants >= 6.)
# Not shipped:       pattern matching, parameter expansion formats,
#                    arithmetic expansion, process substitution, history
#                    expansions, contextual completion, vi/readline mode.
# ============================================================================

# ============================================================================
printf "%s== modular: inhibitors ( ' \" \\\\ )  [implemented] ==%s\n" "$C" "$Z"
# ============================================================================
b_case "single quote: contents literal" \
	"echo '\$HOME \`pwd\` \"hi\" \\\\n stays raw'"
b_case "double quote: variable expands" \
	'echo "$HOME"'
b_case "double quote: backslash escapes only \$ \` \" \\" \
	'echo "a\nb"'
b_case "double quote: escaped double quote stays literal" \
	'echo "he said \"hi\""'
b_case "backslash escapes a metacharacter outside quotes" \
	'echo \(\)\;'
b_case "backslash escapes a space outside quotes" \
	'echo a\ b'
b_case "backslash-newline continues the line (-c form)" \
	$'echo a\\\nb'
b_case "single-quote escape idiom 'a'\\''b'" \
	"echo 'a'\\''b'"
b_case "adjacent quoted segments concatenate" \
	'echo "a"b'\''c'\'
# Three verbatim PDF probes (Inhibitors section).
b_stdin "trailing backslash continues into the next line" "foobar" \
	$'echo foo\\\nbar\nexit\n'
b_case "backslash + single quote outside quotes prints '" \
	"echo \\'"
b_case 'escapes inside double quotes: "\\'\''abcd\\'\''"' \
	'echo "\\'\''abcd\\'\''"'

# ============================================================================
printf "%s== modular: pattern matching (globbing)  [NOT IMPLEMENTED] ==%s\n" "$C" "$Z"
# ============================================================================
# Globbing (*, ?, [], !, character intervals) is not in 42sh. Turn on
# the cases below once the expander grows a glob() replacement.
#
# b_case "star expands in current directory" \
#     'touch a.txt b.txt c.txt; echo *.txt'
# b_case "question mark matches one char" \
#     'touch a1 a2 ab; echo a?'
# b_case "bracket class matches one char from set" \
#     'touch a b c d; echo [bc]'
# b_case "negated bracket class" \
#     'touch a b c; echo [!a]'
# b_case "character range" \
#     'touch a b c d; echo [a-c]'
# b_case "quoted star stays literal" \
#     'touch a.txt; echo "*.txt"'
# b_case "escaped star stays literal" \
#     'touch a.txt; echo \*.txt'
# b_case "no match: pattern stays as-is" \
#     'echo /nonexistent_zz_*.zzz'

# ============================================================================
printf "%s== modular: additional expansion -- tilde [implemented] ==%s\n" "$C" "$Z"
# ============================================================================
b_case "bare ~ expands to HOME" \
	'echo ~'
b_case "~/path expands HOME prefix" \
	'echo ~/x'
b_case "~root expands via getpwnam" \
	'echo ~root'
b_case "~ quoted stays literal" \
	"echo '~'"
b_case "~ mid-word stays literal" \
	'echo a~b'
b_case "two ~ on one line" \
	'echo ~ ~/x'

# ============================================================================
printf "%s== modular: additional expansion -- parameter formats  [NOT IMPLEMENTED] ==%s\n" "$C" "$Z"
# ============================================================================
# \${parameter:-word}, \${parameter:=word}, \${parameter:?word},
# \${parameter:+word}, \${#parameter}, \${parameter%pattern},
# \${parameter%%pattern}, \${parameter#pattern}, \${parameter##pattern}.
# Plain \$VAR and \${VAR} work today (those are mandatory, not modular).
#
# b_case "use default when unset"                'echo ${u_xyz:-fallback}'
# b_case "assign default when unset"             'echo ${u_xyz:=fallback}; echo $u_xyz'
# b_case "error when unset"                      'echo ${u_xyz:?msg}'
# b_case "use alternate when set"                'a=hi; echo ${a:+yes}'
# b_case "string length"                         'a=hello; echo ${#a}'
# b_case "strip shortest suffix"                 'p=a.b.c; echo ${p%.*}'
# b_case "strip longest suffix"                  'p=a.b.c; echo ${p%%.*}'
# b_case "strip shortest prefix"                 'p=a.b.c; echo ${p#*.}'
# b_case "strip longest prefix"                  'p=a.b.c; echo ${p##*.}'

# ============================================================================
printf "%s== modular: control groups ( ) and { ; }  [implemented] ==%s\n" "$C" "$Z"
# ============================================================================
b_case "subshell runs a sequence" \
	'(echo a; echo b)'
b_case "block runs a sequence" \
	'{ echo a; echo b; }'
b_case "subshell isolates variables" \
	'X=1; (X=2; echo in=$X); echo out=$X'
b_case "block shares variables" \
	'X=1; { X=2; }; echo $X'
b_case "subshell isolates cd" \
	'(cd /tmp); pwd | head -c1 > /dev/null; echo back'
b_case "subshell propagates exit status" \
	'(exit 7); echo $?'
b_case "subshell in a pipeline" \
	'(echo a; echo b; echo c) | wc -l'
b_case "nested subshells (3 deep)" \
	'( ( ( echo deep ) ) )'
b_case "redirection on subshell" \
	'(echo r1; echo r2) > out; cat out'
# Documented syntax-error probes -- shell must error cleanly without crashing.
b_case "empty () is a syntax error (no crash)" \
	'() ; echo rc=$?'
b_case "(echo a|) is a syntax error" \
	'(echo a|) ; echo rc=$?'
b_case "(; echo b) is a syntax error" \
	'(; echo b) ; echo rc=$?'
b_case "nested empty () is a syntax error" \
	'(echo c; ()) ; echo rc=$?'

# ============================================================================
printf "%s== modular: command substitution \$()  [implemented] ==%s\n" "$C" "$Z"
# ============================================================================
b_case "simple substitution: \$(echo hi)" \
	'echo $(echo hi)'
b_case "quoted substitution preserves spaces" \
	'echo "$(printf %s "a b c")"'
b_case "nested substitution \$( ... \$( ... ) ... )" \
	'echo $(echo nest-$(echo inner))'
b_case "substitution concatenates with surrounding text" \
	'echo before$(echo mid)after'
b_case "captured value lands in a variable" \
	'X=$(echo cap); echo $X'
b_case "unquoted result is subject to field splitting" \
	'echo $(echo a b c) | wc -w'
b_case "exit status of substituted command propagates to \$?" \
	'echo $(false); echo $?'
# Backtick form `cmd` is NOT supported -- 42sh accepts $() only.
# b_case "backtick substitution"  'echo `echo hi`'

# ============================================================================
printf "%s== modular: arithmetic expansion \$(())  [NOT IMPLEMENTED] ==%s\n" "$C" "$Z"
# ============================================================================
# Operators per subject: ++ -- + - * / %, <= >= < >, == !=, && ||.
# Partial support exists (basic +-*/ and parens) but the module is not
# claimed; do not turn these on without finishing the operator set and the
# inside-dquote / not-inside-squote contract.
#
# b_case "simple add"                            'echo $((1+2))'
# b_case "precedence"                            'echo $((1+2*3))'
# b_case "parenthesised group"                   'echo $((( 1+2 )*3))'
# b_case "subtraction and division"              'echo $((10-3*2)); echo $((20/3))'
# b_case "modulo"                                'echo $((10%3))'
# b_case "comparison: 5>3 -> 1"                  'echo $((5>3))'
# b_case "equality"                              'echo $((4==4))'
# b_case "logical and/or"                        'echo $((1 && 0)); echo $((0 || 1))'
# b_case "expansion inside double quotes"        'echo "$((1+1))"'
# b_case "no expansion inside single quotes"     "echo '\$((1+1))'"
# b_case "huge addition does not crash"          'echo $((9999999999999999 + 1))'

# ============================================================================
printf "%s== modular: process substitution <() >()  [NOT IMPLEMENTED] ==%s\n" "$C" "$Z"
# ============================================================================
# b_case "diff <(...) <(...)"   'diff <(echo a) <(echo a); echo rc=$?'
# b_case "feed cat via <()"     'cat <(printf hi)'
# b_case ">() consumer"         'echo body > >(cat); wait'

# ============================================================================
printf "%s== modular: history expansions  [NOT IMPLEMENTED non-interactive] ==%s\n" "$C" "$Z"
# ============================================================================
# History expansions (!!, !word, !N, !-N), `fc` builtin, save-to-file across
# sessions, CTRL-R incremental search. Most of these require an interactive
# session; non-interactive smoke tests would be brittle.
#
# b_case "!! reuses last command"                'echo first; !!'
# b_case "!N reuses by index"                    'echo a; echo b; !1'
# b_case "!-N reuses Nth from end"               'echo a; echo b; echo c; !-2'
# b_case "fc -l lists history"                   'echo x; fc -l'
# b_case "fc -e vim -1 -10 opens range in editor" 'echo a; echo b; fc -e vim -1 -10'
# b_stdin "history persists across sessions" "old_cmd" $'echo old_cmd\nexit\n'
# CTRL-R incremental search: PTY-only; would belong to test_interactive.c.

# ============================================================================
printf "%s== modular: aliases  [implemented] ==%s\n" "$C" "$Z"
# ============================================================================
# Bash --posix does not expand aliases in -c mode without `shopt -s
# expand_aliases`, so we test 42sh's behaviour directly against an
# expected literal rather than diffing against bash. The piped stdin path
# goes through the REPL where alias expansion is active.
b_stdin "alias expands on a later line" \
	"HI" \
	$'alias ll=\'echo HI\'\nll\nexit\n'
b_stdin "arguments after alias are kept" \
	$'a b c' \
	$'alias g=\'echo\'\ng a b c\nexit\n'
b_stdin "alias chain a -> b -> echo" \
	"deep" \
	$'alias a=b\nalias b=echo\na deep\nexit\n'
b_stdin "alias not expanded as a command argument" \
	"z" \
	$'alias z=\'echo Z\'\necho z\nexit\n'
b_stdin "unalias removes the binding" \
	$'before\n42sh: ll: command not found' \
	$'alias ll=\'echo before\'\nll\nunalias ll\nll 2>&1\nexit\n'
# Invalid alias names -- both shells must REJECT (rc!=0), but each uses its
# own error wording on stderr. Compare exit code only by suppressing all
# output and reporting just the status.
b_case "alias name with =VAL is rejected" \
	'alias =val >/dev/null 2>&1; echo rc=$?'
b_case "alias name with / is rejected" \
	'alias / >/dev/null 2>&1; echo rc=$?'
b_case "alias name with a slash mid-name is rejected" \
	'alias a/b=x >/dev/null 2>&1; echo rc=$?'

# ============================================================================
printf "%s== modular: hash table  [implemented] ==%s\n" "$C" "$Z"
# ============================================================================
# Output format of `hash` differs between shells; we verify behaviour, not
# byte-exact wording, by checking observable side effects.
b_stdin "hash adds a name then lists it" \
	"yes" \
	$'hash ls\nhash | grep -q ls && echo yes || echo no\nexit\n'
b_stdin "hash -d removes a single name" \
	"yes" \
	$'hash ls\nhash -d ls\nhash | grep -q ls && echo no || echo yes\nexit\n'
b_stdin "hash -r flushes the whole table" \
	"yes" \
	$'hash ls\nhash cat\nhash -r\nhash | grep -qE "(ls|cat)" && echo no || echo yes\nexit\n'

# ============================================================================
printf "%s== modular: test / [ builtin  [implemented] ==%s\n" "$C" "$Z"
# ============================================================================
b_case "test -e on an existing file" \
	'test -e /etc/passwd; echo $?'
b_case "test -f on a regular file" \
	'test -f /etc/passwd; echo $?'
b_case "test -d on a directory" \
	'test -d /tmp; echo $?'
b_case "test -z empty string" \
	'test -z ""; echo $?'
b_case "test -n non-empty string" \
	'test -n "x"; echo $?'
b_case "test string equality" \
	'test a = a; echo $?'
b_case "test string inequality" \
	'test a != b; echo $?'
b_case "test numeric -eq" \
	'test 1 -eq 1; echo $?'
b_case "test numeric -lt true" \
	'test 1 -lt 2; echo $?'
b_case "test ! negation true" \
	'test ! -f /nope_xyz_42; echo $?'
b_case "[ ... ] basic form" \
	'[ -f /etc/passwd ]; echo $?'
b_case "[ missing ] is an error" \
	'[ -f /etc/passwd ; echo rc=$?'
b_case "test on missing right operand" \
	'test 1 -eq 2>/dev/null; echo rc=$?'
# Remaining unary file-test operators required by the subject:
# -b -c -g -L -p -r -S -s -u -w -x (already have -d -e -f -z -n).
b_case "test -b on a block special file" \
	'test -b /dev/sda 2>/dev/null; echo rc=$?'
b_case "test -c on a character special file" \
	'test -c /dev/null; echo $?'
b_case "test -g on a setgid binary if any" \
	'test -g /usr/bin/wall 2>/dev/null; echo rc=$?'
b_case "test -L on a symlink (relative)" \
	'ln -s target lnk; test -L lnk; echo $?'
b_case "test -p on a FIFO" \
	'mkfifo p; test -p p; echo $?'
b_case "test -r on a readable file" \
	'echo x > f; test -r f; echo $?'
b_case "test -S on a socket -- expect non-zero" \
	'test -S nope; echo $?'
b_case "test -s on a non-empty file" \
	'echo x > f; test -s f; echo $?'
b_case "test -s on an empty file -- expect non-zero" \
	'touch empty; test -s empty; echo $?'
b_case "test -u on a setuid binary if any" \
	'test -u /usr/bin/passwd 2>/dev/null; echo rc=$?'
b_case "test -w on a writable file" \
	'echo x > f; test -w f; echo $?'
b_case "test -x on an executable" \
	'test -x /bin/sh; echo $?'
# Remaining numeric comparison operators required: -ne -ge -le
b_case "test numeric -ne true" \
	'test 1 -ne 2; echo $?'
b_case "test numeric -ge true (equal)" \
	'test 5 -ge 5; echo $?'
b_case "test numeric -le true (less)" \
	'test 3 -le 5; echo $?'

# ============================================================================
printf "%s== modular: contextual completion  [NOT IMPLEMENTED] ==%s\n" "$C" "$Z"
# ============================================================================
# Contextual completion (commands, builtins, files, variables) requires a
# real terminal + readline integration. Belongs to a PTY harness, not a
# non-interactive script. Probes the subject + correction lists:
#   - Completion of commands in $PATH (e.g. typing `l` + TAB)
#   - Completion of builtins (e.g. typing `ec` + TAB -> echo)
#   - Contextual: `ls /sbin/` + TAB shows files in /sbin only
#   - `abc=def` then `echo ${a` + TAB offers the `abc` variable
#   - `unset a` then `echo ${a` + TAB no longer offers `abc`
# All of these need keystroke injection; they live in test_interactive.c
# when implemented.

# ============================================================================
printf "%s== modular: vi / readline editing modes  [NOT IMPLEMENTED] ==%s\n" "$C" "$Z"
# ============================================================================
# Vi-mode and Readline-mode line editing, switched via `set -o vi` / -o
# readline. Tests would need a PTY harness driving keystrokes; out of
# scope for the non-interactive baseline. Required behaviours per subject:
#   - `set -o vi` switches the line editor into vi mode
#   - `set -o readline` (sometimes `set +o vi`) switches into readline mode
#   - Every shortcut listed in the subject works in each mode:
#       Vi:       # , v j k l h w W e E b B ^ $ 0 |
#                 f F ; , a A i I r R c C S x X d D y Y p P u U
#       Readline: C-b C-f C-p C-n C-_ C-t A-t
# All require keystroke injection -> belong to test_interactive.c.

echo
echo "=============================================================="
printf "%sposix_baseline:%s %d cases   %s%d pass%s   %s%d fail%s\n" \
	"$C" "$Z" "$total" "$G" "$pass" "$Z" "$R" "$fail" "$Z"
[ "$fail" -gt 0 ] && printf "%sfailing:%s %s\n" "$R" "$Z" "$failed"
echo "=============================================================="
[ "$fail" -eq 0 ]
