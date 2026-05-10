AST (Abstract Syntax Tree)
==========================

.. doxygenfile:: ast.h
   :project: core


How is the t_ast generated?
---------------------------
The abstract syntax tree is generated from the tokenized line.
note: The `shell` must be provided in order to handle the heredoc, no other reason.

``t_ast *parser_parse(t_list *tokens, t_shell *shell)``


Ast to string
-----------------

.. doxygenfile:: ast_to_string.c
   :project: core

Parser
------

.. doxygenfile:: parser.h
   :project: core

Parser Entry Point
----------------------------

.. doxygenfile:: parser.c
   :project: core

Simple Commands
----------------------------------

.. doxygenfile:: parser_command.c
   :project: core

Pipes
--------------------------

.. doxygenfile:: parser_pipeline.c
   :project: core

Logical Operators
------------------------------------

.. doxygenfile:: parser_and_or.c
   :project: core

Sequences and Background
-----------------------------------------

.. doxygenfile:: parser_list.c
   :project: core

Subshells and Blocks
--------------------------------------

.. doxygenfile:: parser_group.c
   :project: core

Here-documents
----------------------------------

.. doxygenfile:: parser_heredoc.c
   :project: core

Utilities
---------------------------

.. doxygenfile:: parser_utils.c
   :project: core

AST Memory Management
---------------------------------

.. doxygenfile:: memory.c
   :project: core
