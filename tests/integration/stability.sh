#!/usr/bin/env bash
# ============================================================================
# stability.sh -- hostile-input stability suite for 42sh
# ============================================================================
# The subject's first keyword is "stability"; the correction's rule is blunt:
# a segfault, bus error, double free or memory leak ends the defense with a
# score of 0. This suite exists to find such a failure before a grader does.
#
# It throws deliberately malformed / pathological input at 42sh -- deep
# nesting, unbalanced operators, huge lines, binary garbage, redirection and
# pipe soup -- and a case PASSES only if the shell:
#   - does NOT die from a crash signal (SIGSEGV/SIGABRT/SIGBUS/SIGFPE/SIGILL),
#   - does NOT hang (caught by `timeout`),
#   - does NOT report a memory error/leak under valgrind.
# The shell's exit *code* and output are irrelevant here -- only survival is.
#
# Each input is fed two ways: piped on stdin, and via `-c`.
#
# Usage:   bash tests/integration/stability.sh   (VALGRIND=0 to skip valgrind)
# Exit:    0 if the shell survived every case, 1 otherwise.
# ============================================================================

set -u

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SH="${SHELL_BIN:-$ROOT/42sh}"
SUPP="${SUPP:-$ROOT/suppression.file}"
USE_VALGRIND="${VALGRIND:-1}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

# Hostile inputs can spawn redirections that create files with arbitrary
# names. Run every case from an isolated scratch dir so debris never lands
# in the repo root (where it contaminates subsequent test runs).
mkdir -p "$TMP/work"
cd "$TMP/work" || { echo "stability.sh: cannot enter scratch dir" >&2; exit 2; }

if [ -t 1 ]; then
	G=$'\033[1;32m'; R=$'\033[1;31m'; Y=$'\033[1;33m'
	C=$'\033[1;36m'; Z=$'\033[0m'
else
	G=; R=; Y=; C=; Z=
fi

[ -x "$SH" ] || { echo "stability.sh: shell not built: $SH" >&2; exit 2; }
if [ "$USE_VALGRIND" = "1" ] && ! command -v valgrind >/dev/null 2>&1; then
	printf "%sstability: valgrind not found; skipping valgrind pass%s\n" \
		"$Y" "$Z" >&2
	USE_VALGRIND=0
fi

total=0; pass=0; fail=0; failed=""

# A process killed by a crash signal exits with 128+signal; 124 is `timeout`.
# SIGILL=4, SIGABRT=6, SIGBUS=7, SIGFPE=8, SIGSEGV=11.
is_crash() {
	case "$1" in
		124|132|134|135|136|139) return 0 ;;
		*) return 1 ;;
	esac
}

# --- self-test: the harness must catch a process that genuinely crashes -----
# If the detector cannot recognise a real signal death, every result below is
# meaningless -- so abort rather than report a false "all clear".
( sh -c 'kill -SEGV $$' ) >/dev/null 2>&1
if ! is_crash "$?"; then
	echo "stability.sh: crash detector self-test FAILED -- aborting" >&2
	exit 2
fi
( sh -c 'kill -ABRT $$' ) >/dev/null 2>&1
is_crash "$?" || { echo "stability.sh: self-test FAILED (ABRT)" >&2; exit 2; }

# st_case LABEL INPUT -- feed INPUT to 42sh piped + via -c, must survive both.
st_case() {
	local label="$1" input="$2" rc rcp note vg
	total=$((total + 1))
	note=""
	printf '%s' "$input" | timeout 10 "$SH" >/dev/null 2>&1
	rcp=$?
	timeout 10 "$SH" -c "$input" >/dev/null 2>&1
	rc=$?
	if is_crash "$rcp"; then
		note="piped run crashed/hung (exit $rcp)"
	elif is_crash "$rc"; then
		note="-c run crashed/hung (exit $rc)"
	fi
	if [ -z "$note" ] && [ "$USE_VALGRIND" = "1" ]; then
		vg_args=(--error-exitcode=99 --leak-check=full
			--show-leak-kinds=definite,indirect
			--errors-for-leak-kinds=definite,indirect
			--track-fds=yes --log-file="$TMP/vg.log")
		[ -r "$SUPP" ] && vg_args+=(--suppressions="$SUPP")
		printf '%s' "$input" | timeout 30 valgrind "${vg_args[@]}" \
			"$SH" >/dev/null 2>&1
		vg=$?
		if [ "$vg" = "99" ]; then
			note="valgrind reported a memory error/leak"
		elif is_crash "$vg"; then
			note="crashed under valgrind (exit $vg)"
		fi
	fi
	if [ -z "$note" ]; then
		pass=$((pass + 1))
		printf "  %sPASS%s  %s\n" "$G" "$Z" "$label"
	else
		fail=$((fail + 1))
		failed+="[$label] "
		printf "  %sFAIL%s  %s\n        %s\n" "$R" "$Z" "$label" "$note"
		[ -r "$TMP/vg.log" ] && grep -E "Invalid|lost|Process termin" \
			"$TMP/vg.log" | head -3 | sed 's/^/        /'
	fi
}

printf "%s== stability: unbalanced operators ==%s\n" "$C" "$Z"
st_case "lone ("                   "("
st_case "lone )"                   ")"
st_case "double ("                 "(("
st_case "reversed )("              ")("
st_case "lone {"                   "{"
st_case "lone }"                   "}"
st_case "lone pipe"                "|"
st_case "double pipe only"         "||"
st_case "lone semicolon"           ";"
st_case "triple semicolon"         ";;;"
st_case "lone ampersand"           "&"
st_case "triple ampersand"         "&&&"
st_case "operator soup"            ";;;&&&|||"
st_case "mixed operator soup"      "&|;<>"
st_case "lone backslash"           "\\"
st_case "unclosed single quote"    "echo '"
st_case "unclosed double quote"    "echo \""
st_case "trailing backslash"       "echo foo\\"
st_case "empty subshell"           "()"
st_case "empty block"              "{}"
st_case "pipe into nothing"        "echo |"
st_case "nothing into pipe"        "| echo"
st_case "leading semicolon"        "; echo a"
st_case "subshell pipe to nothing" "(echo a|)"

printf "%s== stability: redirection garbage ==%s\n" "$C" "$Z"
st_case "triple output redir"      "> > >"
st_case "triple input redir"       "< < <"
st_case "redir to nothing"         "echo >"
st_case "redir from nothing"       "< cat"
st_case "many > on one command"    "echo x > a > b > c > d > e"
st_case "append to nothing"        "echo >>"
st_case "dup with no target"       "echo >&"
st_case "heredoc with no delim"    "cat <<"
st_case "heredoc delim then EOF"   "cat << EOF"
st_case "redir then operator"      "> ; echo a"

printf "%s== stability: pipe / quote / expansion pathologies ==%s\n" "$C" "$Z"
st_case "triple pipe"              "| | |"
st_case "four pipes"               "a |||| b"
st_case "bare dollar"              "echo \$"
st_case "empty brace expansion"    "echo \${}"
st_case "unclosed brace expansion" "echo \${FOO"
st_case "dollar paren"             "echo \$("
st_case "dollar double paren"      "echo \$(("
st_case "glob soup"                "echo *** ??? [[["
st_case "tilde soup"               "echo ~~~~~"
st_case "all special chars"        "echo \$\`\\\"'*?[]{}()|&;<>"
st_case "many empty groups"        "()()()()()()()"

printf "%s== stability: scale / exhaustion ==%s\n" "$C" "$Z"
st_case "200 nested subshells" \
	"$(printf '(%.0s' $(seq 1 200))echo deep$(printf ')%.0s' $(seq 1 200))"
st_case "200 nested blocks" \
	"$(printf '{ %.0s' $(seq 1 200))echo deep$(printf '; }%.0s' $(seq 1 200))"
st_case "100 KB single line" \
	"echo $(head -c 100000 /dev/zero | tr '\0' a)"
st_case "10000 arguments" \
	"echo $(printf 'x %.0s' $(seq 1 10000))"
st_case "200-stage pipeline" \
	"echo a$(printf ' | cat%.0s' $(seq 1 200))"
st_case "deeply unbalanced parens" \
	"$(printf '(%.0s' $(seq 1 500))"

printf "%s== stability: binary garbage on stdin ==%s\n" "$C" "$Z"
for i in 1 2 3; do
	garbage="$(head -c 4096 /dev/urandom | tr -d '\0')"
	st_case "random binary blob #$i" "$garbage"
done

echo
echo "=============================================================="
printf "%sstability:%s %d cases   %s%d survived%s   %s%d failed%s\n" \
	"$C" "$Z" "$total" "$G" "$pass" "$Z" "$R" "$fail" "$Z"
[ "$fail" -gt 0 ] && printf "%sfailed:%s %s\n" "$R" "$Z" "$failed"
echo "=============================================================="
[ "$fail" -eq 0 ]
