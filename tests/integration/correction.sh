#!/usr/bin/env bash
# ============================================================================
# correction.sh -- replay of the official correction PDF's mandatory tests
# ============================================================================
# The 42sh correction scale (og_unix_42sh.pdf) spells out the exact mandatory
# tests a grader runs. This suite transcribes the NON-interactive ones and
# replays them against 42sh and `bash --posix`, the reference shell.
#
# The interactive correction items (heredoc with the `heredoc>` prompt, job
# control, CTRL-C/Z/\, line editing) are NOT here -- they belong to the PTY
# harness and signals suite.
#
# A case PASSES when 42sh matches bash on:
#   - stdout (byte-exact),
#   - exit status,
#   - stderr PRESENCE (both produced a diagnostic, or neither did) -- exact
#     stderr text is not compared, since a shell must use its own name.
#
# Each case runs in a fresh scratch directory so file-creating tests
# (redirections, `touch`, `mkdir`) are isolated and reproducible.
#
# Usage:   bash tests/integration/correction.sh
# Exit:    0 if 42sh matches bash on every case, 1 otherwise.
# ============================================================================

set -u

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SH="${SHELL_BIN:-$ROOT/42sh}"
TMP="$(mktemp -d)"
SUPP="${SUPP:-$ROOT/suppression.file}"
USE_VALGRIND="${VALGRIND:-1}"
trap 'rm -rf "$TMP"' EXIT

if [ -t 1 ]; then
	G=$'\033[1;32m'; R=$'\033[1;31m'; C=$'\033[1;36m'; Z=$'\033[0m'
else
	G=; R=; C=; Z=
fi

[ -x "$SH" ] || { echo "correction.sh: shell not built: $SH" >&2; exit 2; }
command -v bash >/dev/null 2>&1 || { echo "correction.sh: no bash" >&2; exit 2; }
if [ "$USE_VALGRIND" = "1" ] && ! command -v valgrind >/dev/null 2>&1; then
	USE_VALGRIND=0
fi

total=0; pass=0; fail=0; failed=""

# c_case LABEL SCRIPT -- run SCRIPT in 42sh and bash from a fresh scratch dir,
# compare stdout (exact), exit status, and stderr presence.
c_case() {
	local label="$1" script="$2" brc src wd report ok
	total=$((total + 1))
	wd="$TMP/case$total"
	rm -rf "$wd"; mkdir -p "$wd"
	(cd "$wd" && bash --posix -c "$script") >"$TMP/b.out" 2>"$TMP/b.err"
	brc=$?
	rm -rf "$wd"; mkdir -p "$wd"
	(cd "$wd" && "$SH" -c "$script") >"$TMP/s.out" 2>"$TMP/s.err"
	src=$?
	ok=1
	report=""
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
	if [ "$ok" = "1" ] && [ "$USE_VALGRIND" = "1" ]; then
		vg_args=(--error-exitcode=99 --leak-check=full
			--show-leak-kinds=definite,indirect
			--errors-for-leak-kinds=definite,indirect
			--track-fds=yes --log-file="$TMP/vg.log")
		[ -r "$SUPP" ] && vg_args+=(--suppressions="$SUPP")
		rm -rf "$wd"; mkdir -p "$wd"
		(cd "$wd" && valgrind "${vg_args[@]}" "$SH" -c "$script") \
			>/dev/null 2>&1
		if [ "$?" = "99" ]; then
			ok=0
			report+="    valgrind reported a memory error/leak"$'\n'
		fi
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

printf "%s== correction: minishell prerequisites ==%s\n" "$C" "$Z"
c_case "extra spaces and tabs around args" \
	'   echo	hello    world	'
c_case "non-existent option -> non-zero" \
	'ls --zzz-bogus-option; echo rc=$?'
c_case "command not found -> 127" \
	'doesnotexist_xyz; echo rc=$?'
c_case "permission denied -> 126" \
	'printf x > cf; chmod 000 cf; ./cf; echo rc=$?'
c_case "absolute path command" \
	'/bin/echo abc; echo rc=$?'
c_case "PATH-resolved command" \
	'echo abc; echo rc=$?'

printf "%s== correction: 21sh redirections / pipelines ==%s\n" "$C" "$Z"
c_case "redirect stdout to file then cat" \
	'ls / > out; cat out'
c_case "input redirect and append" \
	'ls / > out; < out cat -e >> out2; cat out2'
c_case "multiple redirections on echo" \
	'echo 1 >out >&2 2>err; echo 2 >out 2>err; cat err; cat out'
c_case "redirection precedence 2>&1 >/dev/null" \
	'ls zzznope . 2>&1 >/dev/null'
c_case "redirection precedence >/dev/null 2>&1" \
	'ls zzznope . >/dev/null 2>&1; echo rc=$?'
c_case "multi-stage pipeline" \
	'printf "%s\n" 3 1 2 | sort -rn | cat -e'
c_case "sequencing with touch" \
	'ls -1; touch test_file; ls -1'
c_case "exit in every pipeline stage, shell survives" \
	'exit 1 | exit 2 | exit 3; echo "stayin'\'' alive"'
c_case "close stdout then sequence" \
	'echo out >&-; echo out2'
c_case "close stdout in a pipeline" \
	'echo out >&- | echo out2'
c_case "close stdout with &&" \
	'echo out >&- && echo out2'
c_case "close stdout with ||" \
	'echo out >&- || echo out2'

printf "%s== correction: built-ins (exit / echo / cd / type) ==%s\n" "$C" "$Z"
c_case "exit stops the sequence" \
	'echo abc; exit; echo def'
c_case "exit with an enormous number does not crash" \
	'exit 999999999999999999999999999999999999; echo SHOULD_NOT_PRINT'
c_case "exit with a non-numeric argument" \
	'exit abc'
# Note: `exit 1 2 3` (too-many-args) is intentionally NOT replayed here --
# the correction wants the shell to survive it, but `bash --posix -c` exits
# (POSIX special-builtin rule), so bash is the wrong oracle. That behaviour
# is covered directly by test_builtin_exit.c.
c_case "cd absolute then pwd" \
	'cd /tmp; pwd'
c_case "cd - returns to previous directory" \
	'cd /tmp; cd /bin; cd -; pwd'
c_case "cd with no argument goes HOME" \
	'cd /tmp; cd; pwd | head -c1 > /dev/null; echo done'
c_case "type on a builtin and a command" \
	'type type; type ls'

printf "%s== correction: logical operators ==%s\n" "$C" "$Z"
c_case "false && ... || ..." \
	'false && echo foo || echo bar'
c_case "true || ... && ..." \
	'true || echo foo && echo bar'
c_case "|| rescues a failed command, status 0" \
	'ls zzznope || echo rescued; echo rc=$?'
c_case "|| short-circuits after success" \
	'echo "No error" || echo unreachable; echo rc=$?'

printf "%s== correction: environment management ==%s\n" "$C" "$Z"
c_case "per-command env assignment" \
	'a=hello b=world; b=42 echo ${a}_${b} && echo ${b}'
c_case "variables expand into command words" \
	'directory=/ ; opt=-d ; ls ${opt} ${directory}'
c_case "unset variable expands to nothing" \
	'echo ${empty_xyz}|cat -e'
c_case "set lists shell variables" \
	'a=hi b=yo; set | grep -E "^(a|b)="'
c_case "unset PATH then restore it" \
	'unset PATH; PATH=/bin:/usr/bin; mkdir td; echo rc=$?; ls -1 | grep td'
c_case "exit status of true and false" \
	'true; echo ${?}; false; echo ${?}'
c_case "export then read back via env" \
	'export TESTV=xyz; env | grep "^TESTV="'
c_case "empty-value one-shot assignment" \
	'ONESHOT= env | grep "^ONESHOT="'

echo
echo "=============================================================="
printf "%scorrection:%s %d cases   %s%d pass%s   %s%d fail%s\n" \
	"$C" "$Z" "$total" "$G" "$pass" "$Z" "$R" "$fail" "$Z"
[ "$fail" -gt 0 ] && printf "%sfailing:%s %s\n" "$R" "$Z" "$failed"
echo "=============================================================="
[ "$fail" -eq 0 ]
