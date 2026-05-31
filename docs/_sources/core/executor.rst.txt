Executor
========

Walks the AST and runs each node: simple commands, pipelines,
logical operators, subshells, blocks, and background jobs.

.. doxygenfile:: executor.h
   :project: core

Dispatch
---------------------

.. doxygenfile:: executor.c
   :project: core

Simple Commands
--------------------------------

.. doxygenfile:: exec_command.c
   :project: core

Logical (&&, ||, ;)
---------------------------

.. doxygenfile:: exec_logical.c
   :project: core

Pipes
------------------------

.. doxygenfile:: exec_pipeline.c
   :project: core

Subshell, Block, Background
----------------------------------------------

.. doxygenfile:: exec_subshell.c
   :project: core

I/O Redirections
---------------------------------

.. doxygenfile:: redirections.c
   :project: core

PATH Lookup
------------------------------

.. doxygenfile:: command_search.c
   :project: core

Utilities
-------------------------

.. doxygenfile:: exec_utils.c
   :project: core
