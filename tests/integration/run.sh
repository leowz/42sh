#!/usr/bin/env bash
# Integration tests: compare ./42sh -c "<line>" against bash --posix -c "<line>"
# (stdout, stderr, exit status), then run an EXTRA valgrind pass against 42sh
# only. Both shells run non-interactively; valgrind is never applied to bash.
#
# Usage:
#   bash tests/integration/run.sh            # full run with valgrind
#   VALGRIND=0 bash tests/integration/run.sh # skip the valgrind pass
#   CASES=path/to/cases.txt bash ...         # custom cases file
#   SHELL_BIN=./42sh bash ...                # custom shell binary
#
# Exit status: 0 on success, 1 if any comparison or valgrind run failed.

set -u

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SHELL_BIN="${SHELL_BIN:-$ROOT/42sh}"
CASES="${CASES:-$ROOT/tests/integration/cases.txt}"
SUPP="${SUPP:-$ROOT/suppression.file}"
USE_VALGRIND="${VALGRIND:-1}"

if [ -t 1 ]; then
	RED=$'\033[1;31m'; GREEN=$'\033[1;32m'; YELLOW=$'\033[1;33m'
	CYAN=$'\033[1;36m'; RESET=$'\033[0m'
else
	RED=; GREEN=; YELLOW=; CYAN=; RESET=
fi

die() { printf "%sintegration: %s%s\n" "$RED" "$*" "$RESET" >&2; exit 2; }

[ -x "$SHELL_BIN" ] || die "shell binary not found or not executable: $SHELL_BIN"
[ -r "$CASES" ]    || die "cases file not found: $CASES"
command -v bash >/dev/null 2>&1 || die "bash not found in PATH"

if [ "$USE_VALGRIND" = "1" ]; then
	if ! command -v valgrind >/dev/null 2>&1; then
		printf "%sintegration: valgrind not found in PATH; skipping VG pass%s\n" "$YELLOW" "$RESET" >&2
		USE_VALGRIND=0
	fi
fi

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

pass=0; fail=0; vg_fail=0; total=0
failed_cases=""
vg_failed_cases=""

i=0
while IFS= read -r raw || [ -n "$raw" ]; do
	i=$((i+1))
	# strip trailing CR (windows line endings) and leading whitespace
	line="${raw%$'\r'}"
	trimmed="${line#"${line%%[![:space:]]*}"}"
	[ -z "$trimmed" ] && continue
	[ "${trimmed:0:1}" = "#" ] && continue

	total=$((total+1))

	bash_out="$TMP/$i.bash.out";  bash_err="$TMP/$i.bash.err"
	sh_out="$TMP/$i.sh.out";      sh_err="$TMP/$i.sh.err"

	bash --posix -c "$line" >"$bash_out" 2>"$bash_err"
	bash_rc=$?
	"$SHELL_BIN" -c "$line" >"$sh_out" 2>"$sh_err"
	sh_rc=$?

	ok=1
	report=""
	if [ "$bash_rc" != "$sh_rc" ]; then
		ok=0
		report+="    exit:   bash=$bash_rc 42sh=$sh_rc"$'\n'
	fi
	if ! cmp -s "$bash_out" "$sh_out"; then
		ok=0
		report+="    stdout differs (- bash, + 42sh):"$'\n'
		report+="$(diff -u "$bash_out" "$sh_out" | sed 's/^/      /')"$'\n'
	fi
	if ! cmp -s "$bash_err" "$sh_err"; then
		ok=0
		report+="    stderr differs (- bash, + 42sh):"$'\n'
		report+="$(diff -u "$bash_err" "$sh_err" | sed 's/^/      /')"$'\n'
	fi

	if [ "$ok" = "1" ]; then
		pass=$((pass+1))
		printf "%sPASS%s [%03d] %s\n" "$GREEN" "$RESET" "$i" "$line"
	else
		fail=$((fail+1))
		failed_cases+="$i "
		printf "%sFAIL%s [%03d] %s\n%s" "$RED" "$RESET" "$i" "$line" "$report"
	fi

	# Extra valgrind pass - never compared against bash, only checks 42sh.
	if [ "$USE_VALGRIND" = "1" ]; then
		vg_log="$TMP/$i.vg.log"
		vg_args=(--error-exitcode=99
			--leak-check=full
			--show-leak-kinds=definite,indirect
			--errors-for-leak-kinds=definite,indirect
			--track-fds=yes
			--log-file="$vg_log")
		[ -r "$SUPP" ] && vg_args+=(--suppressions="$SUPP")
		valgrind "${vg_args[@]}" "$SHELL_BIN" -c "$line" >/dev/null 2>&1
		vg_rc=$?
		if [ "$vg_rc" = "99" ]; then
			vg_fail=$((vg_fail+1))
			vg_failed_cases+="$i "
			printf "  %sVG  [%03d] valgrind reported errors:%s\n" "$YELLOW" "$i" "$RESET"
			sed 's/^/    /' "$vg_log"
		fi
	fi
done <"$CASES"

echo
printf "%sintegration:%s total=%d %spass=%d%s %sfail=%d%s" \
	"$CYAN" "$RESET" "$total" "$GREEN" "$pass" "$RESET" "$RED" "$fail" "$RESET"
if [ "$USE_VALGRIND" = "1" ]; then
	printf " %svalgrind_errors=%d%s" "$YELLOW" "$vg_fail" "$RESET"
fi
echo

if [ "$fail" -gt 0 ]; then
	printf "%sfailing cases:%s %s\n" "$RED" "$RESET" "$failed_cases"
fi
if [ "$vg_fail" -gt 0 ]; then
	printf "%svalgrind cases:%s %s\n" "$YELLOW" "$RESET" "$vg_failed_cases"
fi

if [ "$fail" -gt 0 ] || [ "$vg_fail" -gt 0 ]; then
	exit 1
fi
exit 0
