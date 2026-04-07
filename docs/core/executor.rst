Executor
========

Walks the AST and runs each node: simple commands, pipelines,
logical operators, subshells, blocks, and background jobs.

.. doxygenfile:: executor.h
   :project: core

executor.c — Dispatch
---------------------

.. doxygenfile:: executor.c
   :project: core

exec_command.c — Simple Commands
--------------------------------

.. doxygenfile:: exec_command.c
   :project: core

exec_logical.c — &&, ||, ;
---------------------------

.. doxygenfile:: exec_logical.c
   :project: core

exec_pipeline.c — Pipes
------------------------

.. doxygenfile:: exec_pipeline.c
   :project: core

exec_subshell.c — Subshell, Block, Background
----------------------------------------------

.. doxygenfile:: exec_subshell.c
   :project: core

redirections.c — I/O Redirections
---------------------------------

.. doxygenfile:: redirections.c
   :project: core

heredoc.c — Here-documents
---------------------------

.. doxygenfile:: heredoc.c
   :project: core

command_search.c — PATH Lookup
------------------------------

.. doxygenfile:: command_search.c
   :project: core

exec_utils.c — Utilities
-------------------------

.. doxygenfile:: exec_utils.c
   :project: core
