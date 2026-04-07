Parser
======

Recursive-descent parser: transforms the token stream into an AST.

.. doxygenfile:: parser.h
   :project: core

parser.c — Main Entry Point
----------------------------

.. doxygenfile:: parser.c
   :project: core

parser_command.c — Simple Commands
----------------------------------

.. doxygenfile:: parser_command.c
   :project: core

parser_pipeline.c — Pipes
--------------------------

.. doxygenfile:: parser_pipeline.c
   :project: core

parser_and_or.c — Logical Operators
------------------------------------

.. doxygenfile:: parser_and_or.c
   :project: core

parser_list.c — Sequences and Background
-----------------------------------------

.. doxygenfile:: parser_list.c
   :project: core

parser_group.c — Subshells and Blocks
--------------------------------------

.. doxygenfile:: parser_group.c
   :project: core

parser_heredoc.c — Here-documents
----------------------------------

.. doxygenfile:: parser_heredoc.c
   :project: core

parser_utils.c — Utilities
---------------------------

.. doxygenfile:: parser_utils.c
   :project: core

memory.c — AST Memory Management
---------------------------------

.. doxygenfile:: memory.c
   :project: core

ast_display.c — Debug Display
------------------------------

.. doxygenfile:: ast_display.c
   :project: core
