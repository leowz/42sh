#!/usr/bin/env bash
# ============================================================================
# signals.sh -- child-process signal-death handling
# ============================================================================
# Correction PDF, signals section: "The shell must in no way leave if one of
# its children ends by a signal, even if it's a SIGKILL." -- and a message
# naming the received signal is expected.
#
# For every signal whose default action terminates a process, this suite
# spawns a child that kills itself with that signal and checks that 42sh:
#   - SURVIVES   -- the shell itself exits cleanly, does not crash or hang;
#   - reports $? -- the exit status of the killed child is 128 + signal.
#
# The shell's diagnostic message is captured and printed for review, but is
# not gated: its exact wording legitimately varies between shells.
#
# Usage:   bash tests/integration/signals.sh
# Exit:    0 if 42sh survives every signal with the correct status, else 1.
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

[ -x "$SH" ] || { echo "signals.sh: shell not built: $SH" >&2; exit 2; }

total=0; pass=0; fail=0; failed=""

# sig_case NAME NUMBER -- a child kills itself with NUMBER; 42sh must survive
# and report the child's status as 128+NUMBER.
sig_case() {
	local name="$1" num="$2" want out shrc ok why msg
	total=$((total + 1))
	want="rc=$((128 + num))"
	# The child is a plain `sh` that signals itself; 42sh then runs `echo`,
	# so a surviving shell exits 0 and prints the child's status via $?.
	out="$(timeout 10 "$SH" -c "sh -c 'kill -$num \$\$'; echo rc=\$?" \
		2>"$TMP/err")"
	shrc=$?
	ok=1
	why=""
	if [ "$shrc" != "0" ]; then
		ok=0
		why="42sh did not survive (own exit $shrc)"
	fi
	if ! printf '%s\n' "$out" | grep -qx "$want"; then
		ok=0
		why="$why; wrong status (want $want, got [${out//$'\n'/ }])"
	fi
	msg="$(tr -d '\n' <"$TMP/err" | cut -c1-50)"
	if [ "$ok" = "1" ]; then
		pass=$((pass + 1))
		printf "  %sPASS%s  SIG%-5s child -> %s   %sshell msg:%s %s\n" \
			"$G" "$Z" "$name" "$want" "$C" "$Z" "${msg:-(none)}"
	else
		fail=$((fail + 1))
		failed+="SIG$name "
		printf "  %sFAIL%s  SIG%-5s %s\n" "$R" "$Z" "$name" "$why"
	fi
}

printf "%s== signals: a child killed by each signal must not sink 42sh ==%s\n" \
	"$C" "$Z"
sig_case HUP  1
sig_case INT  2
sig_case QUIT 3
sig_case ILL  4
sig_case TRAP 5
sig_case ABRT 6
sig_case BUS  7
sig_case FPE  8
sig_case KILL 9
sig_case USR1 10
sig_case SEGV 11
sig_case USR2 12
sig_case PIPE 13
sig_case ALRM 14
sig_case TERM 15

echo
echo "=============================================================="
printf "%ssignals:%s %d signals   %s%d survived%s   %s%d failed%s\n" \
	"$C" "$Z" "$total" "$G" "$pass" "$Z" "$R" "$fail" "$Z"
[ "$fail" -gt 0 ] && printf "%sfailed:%s %s\n" "$R" "$Z" "$failed"
echo "=============================================================="
[ "$fail" -eq 0 ]
