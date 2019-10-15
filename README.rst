========
optparse
========

------------------------------------
Ergonomic command line parsing for C
------------------------------------

``optparse`` is *tiny* but *fancy* library for parsing command line arguments
in C.

Features:

- Automatic help/usage generation.
- Long ("--long") and short ("-s") option style.
- Option merging ("-xaf" can mean "-x -a -f")
- Option-value merging ("-upepe" can mean "-u pepe")
- Use ``--`` to end options (to allow positional arguments starting with dash).
- No dynamic allocation (unless you want it).
- Configuration is specified in ``const`` structures & arrays.
- 3-clause BSD license.

Documentation
=============

Type ``make docs`` to build the documentation. Also, look at the header files.

Building and installing
=======================

A makefile is included for convenience - it generates static and dynamic
libraries and can also install them.

All this might not be necessary: the C/H files could just be copied and compiled
into the application, or the object file or the static archive.

Typing ``make`` (or ``make all``) will create both a static and a dynamic
library.

``make install`` installs it into the destination specified by PREFIX (by
default `/usr/local`.

``make tests`` will run the unit tests and record coverage.


Examples
========

.. include:: tests/readme-example.c
   :code: c
   :start-line: 8
