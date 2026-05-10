# docs/conf.py - Sphinx configuration for 42sh documentation
# Uses Breathe to pull Doxygen XML into Sphinx/RTD.

project = "42sh"
author = "wengzhang, jguillem, jspitz, pulgamecanica, zweng"
copyright = "2026, 42sh contributors"

extensions = [
    "breathe",
]

# -- Breathe (Doxygen XML → Sphinx) -----------------------------------------
breathe_projects = {
    "core": "_doxygen/core/xml",
    "tests": "_doxygen/test/xml",
}
breathe_default_project = "core"

# This is a C project, not C++. Render symbols using the Sphinx C domain
# so prototypes, links, and the index don't show C++-isms (namespaces,
# overloads, mangled signatures).
breathe_domain_by_extension = {
    "h": "c",
    "c": "c",
}
primary_domain = "c"
highlight_language = "c"

# -- General -----------------------------------------------------------------
exclude_patterns = [
    "_build",
    "_doxygen",    # doxygen output (XML + man) - not Sphinx sources
    "assets",
    "index.html",  # old custom HTML
    "pages.json",
]

# -- Warnings ----------------------------------------------------------------
suppress_warnings = ["duplicate_declaration.c"]

# -- HTML output (Read the Docs theme) ---------------------------------------
html_theme = "sphinx_rtd_theme"
html_static_path = ["assets"]
html_favicon = "favicon.ico"
html_title = "42sh Documentation"
html_show_sourcelink = False
