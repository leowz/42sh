#!/usr/bin/env bash
# Integration tests: compare ./42sh -c "<line>" against bash --posix -c "<line>"
# (stdout, stderr, exit status), then run an EXTRA valgrind pass against 42sh
# only. Both shells run non-interactively; valgrind is never applied to bash.
#
# After the cases file, a heredoc phase runs multi-line scripts (which the
# one-line -c form cannot express) through both 42sh and bash and compares
# them. Each script is fed once via a pipe and once via a redirected file,
# since those are two different stdin kinds the heredoc collector must keep
# synchronised with the command stream.
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

# Files that commands inside cases.txt create (cwd-relative artifacts plus
# two under /tmp). Record which already exist so cleanup removes only what
# this run produced and never a developer's pre-existing file.
CASE_ARTIFACTS=(out err test_file /tmp/ftsh_ls_out /tmp/ftsh_ls_out2)
PRE_EXISTING=()
for f in "${CASE_ARTIFACTS[@]}"; do
	[ -e "$f" ] && PRE_EXISTING+=("$f")
done

cleanup() {
	rm -rf "$TMP"
	for f in "${CASE_ARTIFACTS[@]}"; do
		case " ${PRE_EXISTING[*]} " in
		*" $f "*) : ;;
		*) rm -f "$f" ;;
		esac
	done
}
trap cleanup EXIT

# Canonicalise a shell's stderr so a diagnostic can be compared across
# shells: strip the shell-name prefix (`bash:` / `42sh:`) and the bash-only
# `-c:` and `line N:` decorations. What remains is the actual diagnostic
# text the two shells are expected to share -- a shell MUST identify itself
# by its own name, so a byte-exact match against bash is unachievable and
# wrong to require. Normalised output is written to $TMP/e.bash and
# $TMP/e.sh for the caller to diff. Returns 0 when the diagnostics match.
err_match() {
	local re='s/^(bash|42sh): //; s/^-c: //; s/^line [0-9]+: //'
	sed -E "$re" "$1" >"$TMP/e.bash"
	sed -E "$re" "$2" >"$TMP/e.sh"
	cmp -s "$TMP/e.bash" "$TMP/e.sh"
}

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
	if ! err_match "$bash_err" "$sh_err"; then
		ok=0
		report+="    stderr differs (- bash, + 42sh; shell-name prefix normalised):"$'\n'
		report+="$(diff -u "$TMP/e.bash" "$TMP/e.sh" | sed 's/^/      /')"$'\n'
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

# ── Heredoc phase ───────────────────────────────────────────────────────
# Run one multi-line script through 42sh and bash, comparing stdout, stderr
# and exit status -- exactly like the cases above, but the whole script is
# delivered on stdin. Each script is tried in two stdin modes:
#   pipe : non-seekable input  (cmd | 42sh)
#   file : seekable input      (42sh < script)
# A broken heredoc collector desynchronises here: the body comes back empty,
# body lines run as commands, or -- in file mode -- post-heredoc commands
# run twice. All three show up as a diff against bash.
hd_case() {
	local label script mode
	label="$1"
	script="$2"
	for mode in pipe file; do
		total=$((total + 1))
		bash_out="$TMP/hd.bash.out"; bash_err="$TMP/hd.bash.err"
		sh_out="$TMP/hd.sh.out";     sh_err="$TMP/hd.sh.err"
		if [ "$mode" = "pipe" ]; then
			printf '%s' "$script" | bash --posix >"$bash_out" 2>"$bash_err"
			bash_rc=$?
			printf '%s' "$script" | "$SHELL_BIN" >"$sh_out" 2>"$sh_err"
			sh_rc=$?
		else
			printf '%s' "$script" >"$TMP/hd.script"
			bash --posix <"$TMP/hd.script" >"$bash_out" 2>"$bash_err"
			bash_rc=$?
			"$SHELL_BIN" <"$TMP/hd.script" >"$sh_out" 2>"$sh_err"
			sh_rc=$?
		fi
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
		if ! err_match "$bash_err" "$sh_err"; then
			ok=0
			report+="    stderr differs (- bash, + 42sh; shell-name prefix normalised):"$'\n'
			report+="$(diff -u "$TMP/e.bash" "$TMP/e.sh" | sed 's/^/      /')"$'\n'
		fi
		if [ "$ok" = "1" ]; then
			pass=$((pass + 1))
			printf "%sPASS%s [hd] %s (%s)\n" "$GREEN" "$RESET" "$label" "$mode"
		else
			fail=$((fail + 1))
			failed_cases+="hd:$label/$mode "
			printf "%sFAIL%s [hd] %s (%s)\n%s" \
				"$RED" "$RESET" "$label" "$mode" "$report"
		fi
		if [ "$USE_VALGRIND" = "1" ] && [ "$mode" = "pipe" ]; then
			vg_log="$TMP/hd.vg.log"
			vg_args=(--error-exitcode=99
				--leak-check=full
				--show-leak-kinds=definite,indirect
				--errors-for-leak-kinds=definite,indirect
				--track-fds=yes
				--log-file="$vg_log")
			[ -r "$SUPP" ] && vg_args+=(--suppressions="$SUPP")
			printf '%s' "$script" \
				| valgrind "${vg_args[@]}" "$SHELL_BIN" >/dev/null 2>&1
			if [ "$?" = "99" ]; then
				vg_fail=$((vg_fail + 1))
				vg_failed_cases+="hd:$label "
				printf "  %sVG  [hd] %s valgrind reported errors:%s\n" \
					"$YELLOW" "$label" "$RESET"
				sed 's/^/    /' "$vg_log"
			fi
		fi
	done
}

echo
printf "%s-- heredoc phase --%s\n" "$CYAN" "$RESET"
hd_case "body delivered"         $'cat << EOF\nalpha\nbeta\nEOF\necho TAIL\n'
hd_case "body is inert data"     $'cat << EOF\nINSIDE TEXT\nEOF\necho TAIL\n'
hd_case "empty body"             $'cat << EOF\nEOF\necho TAIL\n'
hd_case "strip tabs <<-"         $'cat <<- EOF\n\t\tindented\n\tEOF\n'
hd_case "quoted delim no expand" $'cat << \'EOF\'\n$HOME\nEOF\n'
hd_case "multiple heredocs"      $'cat << A\nfromA\nA\ncat << B\nfromB\nB\necho TAIL\n'
hd_case "heredoc in pipeline"    $'cat << EOF | wc -l\nl1\nl2\nl3\nEOF\n'

# ── SIGPIPE / broken-pipe phase ─────────────────────────────────────────
# `producer | head`: head takes what it needs, exits, and closes the pipe,
# so the (infinite) producer upstream must be terminated -- the shell must
# reap it and not hang. stderr is deliberately NOT compared: a producer
# killed by SIGPIPE is silent, but where SIGPIPE is ignored it prints
# "write error: Broken pipe" instead -- environment-dependent by nature
# (this is exactly why such a pipeline cannot be a normal cases.txt entry).
# What IS deterministic -- and what this phase asserts -- is stdout, exit
# status, and that the pipeline terminates. Each run is wrapped in
# `timeout`; a 124 exit means 42sh hung.
sigpipe_case() {
	local label cmd
	label="$1"
	cmd="$2"
	total=$((total + 1))
	bash_out="$TMP/sp.bash.out"
	sh_out="$TMP/sp.sh.out"
	timeout 10 bash --posix -c "$cmd" >"$bash_out" 2>/dev/null
	bash_rc=$?
	timeout 10 "$SHELL_BIN" -c "$cmd" >"$sh_out" 2>/dev/null
	sh_rc=$?
	ok=1
	report=""
	if [ "$sh_rc" = "124" ]; then
		ok=0
		report+="    42sh HUNG (timeout) -- pipeline did not terminate"$'\n'
	fi
	if [ "$bash_rc" != "$sh_rc" ]; then
		ok=0
		report+="    exit:   bash=$bash_rc 42sh=$sh_rc"$'\n'
	fi
	if ! cmp -s "$bash_out" "$sh_out"; then
		ok=0
		report+="    stdout differs (- bash, + 42sh):"$'\n'
		report+="$(diff -u "$bash_out" "$sh_out" | sed 's/^/      /')"$'\n'
	fi
	if [ "$ok" = "1" ]; then
		pass=$((pass + 1))
		printf "%sPASS%s [sp] %s\n" "$GREEN" "$RESET" "$label"
	else
		fail=$((fail + 1))
		failed_cases+="sp:$label "
		printf "%sFAIL%s [sp] %s\n%s" "$RED" "$RESET" "$label" "$report"
	fi
	if [ "$USE_VALGRIND" = "1" ]; then
		vg_log="$TMP/sp.vg.log"
		vg_args=(--error-exitcode=99
			--leak-check=full
			--show-leak-kinds=definite,indirect
			--errors-for-leak-kinds=definite,indirect
			--track-fds=yes
			--log-file="$vg_log")
		[ -r "$SUPP" ] && vg_args+=(--suppressions="$SUPP")
		timeout 30 valgrind "${vg_args[@]}" "$SHELL_BIN" -c "$cmd" \
			>/dev/null 2>&1
		if [ "$?" = "99" ]; then
			vg_fail=$((vg_fail + 1))
			vg_failed_cases+="sp:$label "
			printf "  %sVG  [sp] %s valgrind reported errors:%s\n" \
				"$YELLOW" "$label" "$RESET"
			sed 's/^/    /' "$vg_log"
		fi
	fi
}

echo
printf "%s-- SIGPIPE / broken-pipe phase --%s\n" "$CYAN" "$RESET"
sigpipe_case "infinite producer, head -n 1" 'yes | head -n 1'
sigpipe_case "infinite producer, head -n 5" 'yes | head -n 5'
sigpipe_case "SIGPIPE through middle stage" 'yes | cat | head -n 3'
sigpipe_case "infinite binary src, head -c" 'cat /dev/zero | head -c 16 | wc -c'

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
