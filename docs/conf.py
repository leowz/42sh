# docs/conf.py — Sphinx configuration for 42sh documentation
# Uses Breathe to pull Doxygen XML into Sphinx/RTD.

project = "42sh"
author = "wengzhang, jguillem, jspitz, pulgamecanica, zweng"
copyright = "2026, 42sh contributors"

extensions = [
    "breathe",
]

# -- Breathe (Doxygen XML → Sphinx) -----------------------------------------
breathe_projects = {
    "core": "core/xml",
    "tests": "test/xml",
}
breathe_default_project = "core"

# -- General -----------------------------------------------------------------
exclude_patterns = [
    "_build",
    "core/xml",    # doxygen XML output — not Sphinx sources
    "core/man",    # doxygen man output
    "test",        # doxygen test output
    "assets",
    "index.html",  # old custom HTML
    "pages.json",
]

# -- Warnings ----------------------------------------------------------------
suppress_warnings = ["duplicate_declaration.cpp"]

# -- HTML output (Read the Docs theme) ---------------------------------------
html_theme = "sphinx_rtd_theme"
html_static_path = ["assets"]
html_favicon = "favicon.ico"
html_title = "42sh Documentation"
html_show_sourcelink = False
