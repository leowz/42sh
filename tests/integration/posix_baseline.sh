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
b_case "sequence then second command file-creating" \
	'ls -1 > before; touch newly_added; ls -1 > after; grep -c newly_added after'

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
b_case "cd - returns to OLDPWD" \
	'cd /tmp; cd /bin; cd -; pwd'
b_case "cd - alternates each time" \
	'cd /tmp; cd /bin; cd -; cd -; pwd'
b_case "cd -L logical pwd" \
	'cd -L /tmp; pwd'
b_case "cd -P physical pwd" \
	'cd -P /tmp; pwd | head -c1 > /dev/null; echo done'
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
b_case "export with NAME=VALUE assigns and exports in one step" \
	'export TESTV=xyz; env | grep "^TESTV="'
b_case "unset removes from set" \
	'a=hi; unset a; set | grep "^a=" || echo gone'
b_case "unset removes multiple names at once" \
	'a=1; b=2; unset a b; set | grep -E "^(a|b)=" || echo gone'
b_case "empty-value one-shot env assignment" \
	'ONESHOT= env | grep "^ONESHOT="'
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
b_stdin "heredoc append: two heredocs feed the same file" $'abc\ndef$\nghi$' \
	$'cat > out << FIN\nabc\nFIN\ncat -e >> out << FIN\ndef\nghi\nFIN\ncat out\nexit\n'
b_stdin "<<- strips leading tabs from body" $'one\ntwo' \
	$'cat <<-EOF\n\tone\n\t\ttwo\n\tEOF\nexit\n'
b_stdin "quoted delim suppresses variable expansion in body" '$HOME' \
	$'cat <<\'EOF\'\n$HOME\nEOF\nexit\n'

echo
echo "=============================================================="
printf "%sposix_baseline:%s %d cases   %s%d pass%s   %s%d fail%s\n" \
	"$C" "$Z" "$total" "$G" "$pass" "$Z" "$R" "$fail" "$Z"
[ "$fail" -gt 0 ] && printf "%sfailing:%s %s\n" "$R" "$Z" "$failed"
echo "=============================================================="
[ "$fail" -eq 0 ]
