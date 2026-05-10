#!/bin/sh
# scripts/gendocs.sh - Documentation generation for 42sh
#
# Commands:
#   docs       - run doxygen (core + tests) and add SEE ALSO cross-refs
#   html       - convert man pages to HTML and regenerate docs/pages.json
#   see-also   - only (re)add SEE ALSO sections to existing man pages
#   pages-json - only regenerate docs/pages.json
#
# Always run from the project root.

set -e

CORE_MAN="docs/_doxygen/core/man/man9"
TEST_MAN="docs/_doxygen/test/man/man9"
CORE_HTML="docs/_doxygen/core_html"
TEST_HTML="docs/_doxygen/test_html"

GREEN="\033[1;32m"
RED="\033[1;31m"
EOC="\033[0;0m"

info() { printf "${GREEN}%s${EOC}\n" "$*"; }
err()  { printf "${RED}%s${EOC}\n"  "$*" >&2; }

# ---------------------------------------------------------------------------
# add_see_also <man_dir>
#   Appends a .SH SEE ALSO section to every real (non-.so) man page in
#   <man_dir>, cross-referencing all sibling pages.
# ---------------------------------------------------------------------------
add_see_also() {
    dir="$1"
    ls "$dir"/*.9 >/dev/null 2>&1 || return 0

    real=$(for f in "$dir"/*.9; do
        head -1 "$f" | grep -q '^\.so' || printf '%s\n' "$(basename "$f" .9)"
    done)

    for name in $real; do
        refs=$(printf '%s\n' $real \
            | while read -r other; do
                [ "$other" = "$name" ] || printf '.BR %s (9),\n' "$other"
              done \
            | sed '$s/,$//')
        printf '.SH SEE ALSO\n%s\n' "$refs" >> "$dir/$name.9"
    done
}

# ---------------------------------------------------------------------------
# man_to_html <man_dir> <out_dir>
#   Converts every real man page in <man_dir> to an HTML file in <out_dir>
#   using man2html.  Post-processing:
#     - sed:  fix a Value:\.PP artefact left by doxygen
#     - perl: convert <DL COMPACT> bullet lists into clean <UL> lists
# ---------------------------------------------------------------------------
man_to_html() {
    src="$1" dst="$2"
    ls "$src"/*.9 >/dev/null 2>&1 || return 0
    mkdir -p "$dst"

    for f in "$src"/*.9; do
        grep -q '^\.so' "$f" && continue
        name=$(basename "$f" .9)
        printf '  converting %s...\n' "$name"
		man2html "$f" \
            | sed 's/Value:\.PP/Value:/g' \
            | perl -0777 -pe \
                's{<DL COMPACT>\n(<DT>&bull;<DD>\n.*?)</DL>}{"<UL>\n".(do{my $c=$1;$c=~s{<DT>&bull;<DD>\n}{<LI>\n}g;$c})."</UL>"}ges' \
            > "$dst/$name.html"
    done
}

# ---------------------------------------------------------------------------
# json_list <html_dir>
#   Prints a comma-separated JSON array body from the .html files in <html_dir>.
# ---------------------------------------------------------------------------
json_list() {
    sep=''
    for f in "$1"/*.html; do
        [ -f "$f" ] || continue
        name=$(basename "$f" .html)
        printf '%s"%s"' "$sep" "$name"
        sep=', '
    done
}

# ---------------------------------------------------------------------------
# gen_pages_json
#   Writes docs/pages.json from whatever .html files exist under docs/.
# ---------------------------------------------------------------------------
gen_pages_json() {
    printf '{\n  "core": [%s],\n  "test": [%s]\n}\n' \
        "$(json_list "$CORE_HTML")" \
        "$(json_list "$TEST_HTML")" \
        > docs/pages.json
    info "docs/pages.json written."
}

# ---------------------------------------------------------------------------
# Commands
# ---------------------------------------------------------------------------

cmd_docs() {
    mkdir -p docs/_doxygen/core docs/_doxygen/test
    info "[1/2] Generating core man pages..."
    doxygen Doxyfile
    info "[2/2] Generating test man pages..."
    ( cd tests && doxygen Doxyfile )
    info "Adding SEE ALSO cross-references..."
    add_see_also "$CORE_MAN"
    add_see_also "$TEST_MAN"
    info "Man pages ready."
}

cmd_html() {
    info "Converting man pages to HTML..."
    mkdir -p "$CORE_HTML" "$TEST_HTML"
    man_to_html "$CORE_MAN" "$CORE_HTML"
    man_to_html "$TEST_MAN" "$TEST_HTML"
    gen_pages_json
    info "Docs ready - run 'make serve' to view."
}

# ---------------------------------------------------------------------------
# Dispatch
# ---------------------------------------------------------------------------

case "${1:-docs}" in
    docs)       cmd_docs ;;
    html)       cmd_html ;;
    see-also)   add_see_also "$CORE_MAN"; add_see_also "$TEST_MAN" ;;
    pages-json) gen_pages_json ;;
    *)
        err "Usage: $0 {docs|html|see-also|pages-json}"
        exit 1
        ;;
esac
