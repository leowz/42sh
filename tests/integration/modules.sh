#!/usr/bin/env bash
# ============================================================================
# modules.sh -- 42sh modular-feature scoreboard
# ============================================================================
# A module-INDEXED integration suite: one section per modular feature from
# plan/13_modular_features.md. The subject requires 6 working modules to pass,
# and this file is the single place to see, at a glance, which ones are done.
#
# It is NOT a duplicate of the unit tests: those test functions (test_lexer.c,
# test_expander.c, ...); this drives the real compiled ./42sh end-to-end and
# compares against `bash --posix`. Different level, complementary, no drift.
#
# Conventions:
#   - Each case runs `./42sh -c "<script>"` and `bash --posix -c "<script>"`,
#     comparing STDOUT + EXIT STATUS. stderr is NOT compared: error wording is
#     shell-specific ("42sh:" vs "bash:") and not a meaningful contract.
#   - Implemented modules get HARD, thorough cases (edge cases, not happy path).
#   - Unimplemented modules are marked SKIP.
#   - A case for a known, accepted gap is marked XFAIL: it is shown and tracked
#     but does NOT fail the suite. If an XFAIL ever passes it shows as XPASS
#     ("promote it"). Only an unexpected FAIL makes the suite exit non-zero,
#     so this file is safe to run in CI: red == a real regression.
#
# Usage:  bash tests/integration/modules.sh        (or: make modules)
# Exit:   0 if no unexpected FAIL, 1 otherwise.
# ============================================================================

set -u

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SH="${SHELL_BIN:-$ROOT/42sh}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

if [ -t 1 ]; then
	G=$'\033[1;32m'; R=$'\033[1;31m'; Y=$'\033[1;33m'
	C=$'\033[1;36m'; Z=$'\033[0m'
else
	G=; R=; Y=; C=; Z=
fi

[ -x "$SH" ] || { echo "modules.sh: shell binary not built: $SH" >&2; exit 2; }
command -v bash >/dev/null 2>&1 || { echo "modules.sh: bash not found" >&2; exit 2; }

total=0; pass=0; fail=0; xfail=0; xpass=0
mods_impl=0; mods_complete=0; mods_skip=0
cur_mod=""; cur_n=0; cur_ok=0; cur_xf=0

# flush_module -- print the result line for the module just finished.
flush_module() {
	[ -z "$cur_mod" ] && return
	mods_impl=$((mods_impl + 1))
	if [ "$cur_n" -gt 0 ] && [ "$cur_n" -eq "$cur_ok" ]; then
		mods_complete=$((mods_complete + 1))
		printf "  %smodule COMPLETE%s   %d/%d cases\n" "$G" "$Z" "$cur_ok" "$cur_n"
	else
		printf "  %smodule HAS GAPS%s   %d/%d cases pass, %d known gap(s)\n" \
			"$Y" "$Z" "$cur_ok" "$cur_n" "$cur_xf"
	fi
	cur_mod=""
}

# module NAME -- start an implemented-module section.
module() {
	flush_module
	cur_mod="$1"; cur_n=0; cur_ok=0; cur_xf=0
	printf "\n%s== %s ==%s\n" "$C" "$1" "$Z"
}

# skip NAME REASON -- declare an unimplemented module.
skip() {
	flush_module
	mods_skip=$((mods_skip + 1))
	printf "\n%s== %s ==%s\n  %sSKIP%s  %s\n" "$C" "$1" "$Z" "$Y" "$Z" "$2"
}

# m_case DESC SCRIPT [xfail] -- run one case (42sh vs bash, stdout+exit).
m_case() {
	local desc="$1" script="$2" mode="${3:-pass}" brc src match
	total=$((total + 1)); cur_n=$((cur_n + 1))
	bash --posix -c "$script" >"$TMP/b" 2>/dev/null; brc=$?
	"$SH" -c "$script" >"$TMP/s" 2>/dev/null; src=$?
	match=0
	[ "$brc" = "$src" ] && cmp -s "$TMP/b" "$TMP/s" && match=1
	if [ "$mode" = "xfail" ]; then
		if [ "$match" = 1 ]; then
			xpass=$((xpass + 1)); cur_ok=$((cur_ok + 1))
			printf "  %sXPASS%s %s  (known gap now PASSES -- promote it)\n" \
				"$Y" "$Z" "$desc"
		else
			xfail=$((xfail + 1)); cur_xf=$((cur_xf + 1))
			printf "  %sXFAIL%s %s  (known gap, accepted)\n" "$Y" "$Z" "$desc"
		fi
		return
	fi
	if [ "$match" = 1 ]; then
		pass=$((pass + 1)); cur_ok=$((cur_ok + 1))
		printf "  %sPASS%s  %s\n" "$G" "$Z" "$desc"
	else
		fail=$((fail + 1))
		printf "  %sFAIL%s  %s\n" "$R" "$Z" "$desc"
		printf "        cmd:  %s\n" "$script"
		printf "        bash: exit=%s [%s]\n" "$brc" "$(tr '\n' '~' <"$TMP/b")"
		printf "        42sh: exit=%s [%s]\n" "$src" "$(tr '\n' '~' <"$TMP/s")"
	fi
}

# m_alias DESC SCRIPT EXPECTED -- pipe a multi-line SCRIPT to 42sh and check
# its stdout against a fixed EXPECTED value. Aliases are not diffed against
# bash: bash does not expand aliases in a non-interactive shell unless
# `shopt -s expand_aliases` is set, so a direct bash comparison is not
# meaningful. The script is piped (not -c) so the REPL reads it line by
# line -- an alias only takes effect on a line read after its definition.
m_alias() {
	local label="$1" script="$2" want="$3" got
	total=$((total + 1))
	cur_n=$((cur_n + 1))
	got="$(printf '%s' "$script" | "$SH" 2>/dev/null)"
	if [ "$got" = "$want" ]; then
		pass=$((pass + 1))
		cur_ok=$((cur_ok + 1))
		printf "  %sPASS%s  %s\n" "$G" "$Z" "$label"
	else
		fail=$((fail + 1))
		printf "  %sFAIL%s  %s\n" "$R" "$Z" "$label"
		printf "        expected: [%s]\n" "$want"
		printf "        actual:   [%s]\n" "$got"
	fi
}

# ============================================================================
# MODULE 1 -- Inhibitors  ('  "  \)
# ============================================================================
module "1. Inhibitors ( ' \" \\ )"
m_case "single quotes: everything literal" \
	"echo 'all \$HOME \`x\` \${y} \"q\" \\n stays literal'"
m_case "single quotes: no var expansion"      "echo '\$HOME'"
m_case "double quotes: var DOES expand"        'echo "[$HOME]"'
m_case "double quotes: backslash escapes \$"   'echo "a\$b"'
m_case "double quotes: \\n stays literal"      'echo "a\nb"'
m_case "double quotes: escaped double quote"   'echo "he said \"hi\""'
m_case "double quotes: escaped backslash"      'echo "a\\b"'
m_case "double quotes: escaped backtick"       'echo "a\`b"'
m_case "double quotes: \${x} literal via \\"   'echo "\${notvar}"'
m_case "double quotes: spaces preserved"       'echo "a   b   c"'
m_case "double quotes: var concat"             'echo "$HOME/sub/dir"'
m_case "unquoted backslash escapes \$"         'echo \$HOME'
m_case "unquoted backslash escapes backslash"  'echo \\'
m_case "unquoted backslash escapes space"      'echo a\ b'
m_case "unquoted backslash escapes tab"        $'echo x\\\ty'
m_case "single-quote-escape idiom 'a'\\''b'"   "echo 'a'\\''b'"
m_case "adjacent quote concatenation"          'echo a'\''b'\''c"d"e'
m_case "single quote inside double quote"      'echo "it'\''s working"'
m_case "double quote inside single quote"      "echo 'say \"hi\" now'"
m_case "all three inhibitors in one word"      'echo a\ b'\''c d'\''"e f"'
m_case "backslash-newline line continuation"   $'echo a\\\nb'
m_case "line continuation mid-word"             $'ec\\\nho hi'
m_case "line continuation inside double quotes" $'echo "a\\\nb"'
m_case "line continuation literal in single q"  $'echo \'a\\\nb\''

# ============================================================================
# MODULE 2 -- Globbing  (*  ?  [])
# ============================================================================
skip "2. Globbing (* ? [])" "not implemented"
# when implemented: echo /etc/pass* ; echo /dev/nul? ; echo [a-z]* ; quoted "*"

# ============================================================================
# MODULE 3 -- Tilde expansion
# ============================================================================
module "3. Tilde expansion"
m_case "bare ~ expands to HOME"            'echo ~'
m_case "~/path expands"                    'echo ~/a/b/c'
m_case "~ trailing slash"                  'echo ~/'
m_case "~ quoted in double quotes literal" 'echo "~"'
m_case "~ quoted in single quotes literal" "echo '~'"
m_case "~ literal inside double quotes"    'echo "x ~ y"'
m_case "~user (root) via getpwnam"         'echo ~root'
m_case "~ mid-word stays literal"          'echo a~b'
m_case "prefix then ~ stays literal"       'echo x~'
m_case "unknown ~user stays literal"       'echo ~nosuchuser_zzz999'
m_case "~ in assignment value"             'X=~/d; echo "$X"'
m_case "two ~ in one command"              'echo ~ ~/x'

# ============================================================================
# MODULE 4 -- Parameter expansion formats  (${:-} ${#} ${%} ...)
# ============================================================================
skip "4. Parameter expansion formats" \
	"not implemented (plain \$VAR / \${VAR} work -- that is mandatory, not the module)"
# when implemented: ${U:-def} ${U:=x} ${U:?} ${U:+y} ${#HOME} ${PATH##*:} ${V%pat}

# ============================================================================
# MODULE 5 -- Control groups  ()  {}
# ============================================================================
module "5. Control groups ( ) and { }"
m_case "subshell runs a sequence"           '(echo a; echo b)'
m_case "block runs a sequence"              '{ echo a; echo b; }'
m_case "subshell ISOLATES variables"        'X=1; (X=2; echo in $X); echo out $X'
m_case "block SHARES variables"             'X=1; { X=2; }; echo $X'
m_case "subshell ISOLATES cd"               '(cd / && pwd); echo back'
m_case "block cd AFFECTS parent"            '{ cd /; }; pwd'
m_case "subshell propagates exit status"    '(exit 7); echo $?'
m_case "block propagates exit status"       '{ false; }; echo $?'
m_case "subshell in a pipeline"             '(echo a; echo b; echo c) | wc -l'
m_case "block in a pipeline"                '{ echo a; echo b; } | wc -l'
m_case "subshell on both pipe sides"        '(echo piped) | (cat)'
m_case "nested subshells (3 deep)"          '( ( ( echo deep ) ) )'
m_case "group combined with && / ||"        '(true && echo yes) || echo no'
m_case "subshell rescues with ||"           '(false || echo rescued)'
m_case "redirection applied to subshell"    "(echo r1; echo r2) > $TMP/gr; cat $TMP/gr"
m_case "redirection applied to block"       "{ echo b1; echo b2; } > $TMP/gb; cat $TMP/gb"
m_case "exit status mid-sequence of groups" '(exit 3); echo a; (exit 5); echo $?'
m_case "empty subshell () is a syntax error" '()'
m_case "unclosed ( is a syntax error"        '('

# ============================================================================
# MODULE 6 -- Command substitution  $()  ``
# ============================================================================
skip "6. Command substitution \$() \`\`" "not implemented (\$() causes a parser error)"
# when implemented: echo $(echo hi) ; echo `whoami` ; echo $(echo $(echo nest))

# ============================================================================
# MODULE 7 -- Arithmetic expansion  $(())
# ============================================================================
skip "7. Arithmetic expansion \$(())" "not implemented (\$(()) causes a parser error)"
# when implemented: echo $((1+2)) ; echo $((5>3)) ; x=5; echo $((x*2))

# ============================================================================
# MODULE 8 -- Process substitution  <()  >()
# ============================================================================
skip "8. Process substitution <() >()" "not implemented"
# when implemented: diff <(echo a) <(echo a) ; cat <(echo via-procsub)

# ============================================================================
# MODULE 9 -- History management
# ============================================================================
skip "9. History management" \
	"partial: list + file + 'history' builtin work; expansions (!! !n) and fc missing"
# when implemented: !! ; !-1 ; !echo ; fc -l

# ============================================================================
# MODULE 10 -- Contextual completion
# ============================================================================
skip "10. Contextual completion" "not implemented (readline default file completion only)"

# ============================================================================
# MODULE 11 -- Vi / Emacs editing modes
# ============================================================================
skip "11. Vi/Emacs editing modes" "not implemented (no 'set -o vi' / 'set -o emacs')"

# ============================================================================
# MODULE 12 -- Aliases
# ============================================================================
module "12. Aliases"
m_alias "alias expands on a later line" \
	$'alias ll=\'echo HI\'\nll\n' "HI"
m_alias "arguments after the alias are kept" \
	$'alias g=\'echo\'\ng a b c\n' "a b c"
m_alias "alias chain a->b->echo resolves" \
	$'alias a=b\nalias b=echo\na deep\n' "deep"
m_alias "self-referential alias expands once" \
	$'alias ls=\'echo SELF\'\nls\n' "SELF"
m_alias "alias expands after ;" \
	$'alias g=\'echo\'\necho x ; g y\n' $'x\ny'
m_alias "alias expands after |" \
	$'alias up=\'tr a-z A-Z\'\necho hi | up\n' "HI"
m_alias "alias not expanded as an argument" \
	$'alias z=\'echo Z\'\necho z\n' "z"
m_alias "quoted command word not expanded" \
	$'alias x=\'echo BAD\'\n"x"\n' ""
m_alias "unalias stops expansion" \
	$'alias e=\'echo VISIBLE\'\ne\nunalias e\ne\n' "VISIBLE"
m_alias "unalias -a clears every alias" \
	$'alias e=\'echo SEEN\'\ne\nunalias -a\ne\n' "SEEN"

# ============================================================================
# MODULE 13 -- Hash table
# ============================================================================
skip "13. Hash table" "not implemented (no 'hash' builtin)"
# when implemented: hash ; hash -r ; cached PATH lookups

# ============================================================================
# MODULE 14 -- test / [ builtin
# ============================================================================
skip "14. test / [ builtin" "not implemented (uses external /usr/bin/test)"
# when implemented: test -f F ; [ -d /tmp ] ; [ 5 -gt 3 ] ; test -z "" ; ! test ...

# ============================================================================
flush_module
echo
echo "=============================================================="
printf "%smodules.sh -- 42sh modular-feature scoreboard%s\n" "$C" "$Z"
printf "  modules:  %d/14 implemented   %d/%d complete (no gaps)   %d/14 skipped\n" \
	"$mods_impl" "$mods_complete" "$mods_impl" "$mods_skip"
printf "  cases:    %d run   %s%d pass%s   %s%d fail%s   %s%d xfail%s" \
	"$total" "$G" "$pass" "$Z" "$R" "$fail" "$Z" "$Y" "$xfail" "$Z"
[ "$xpass" -gt 0 ] && printf "   %s%d xpass%s" "$Y" "$xpass" "$Z"
echo
printf "  subject:  6 modules required to pass  ->  have %d\n" "$mods_impl"
echo "=============================================================="

[ "$fail" -eq 0 ]
