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

Docs are available at https://jcarrano.github.io/optparse/.

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

.. code:: c

   #include <stdio.h>
   #include "optparse.h"

   enum _rules {
   	VERBOSITY,
   	SETTABLE,
   	INTTHING,
   	HELP_OPT,
   	ARG1,
   	N_RULES
   };

   static const struct opt_rule rules[N_RULES] = {
   [VERBOSITY] = OPTPARSE_O(COUNT, 'v', "verbose",
   			 "Verbosity level (can be given multiple times)", 0),

   [SETTABLE] = OPTPARSE_O(SET_BOOL, 's', NULL, "Set a flag 's'", false),

   [INTTHING] = OPTPARSE_O(INT, 'c',  "cool", "Set an integer", -10),

   [HELP_OPT] = OPTPARSE_O(DO_HELP, 'h', "help", "Show this help", 0),
   /* positionals */
   [ARG1] = OPTPARSE_P(STR_NOCOPY, "first-argument", "Just store this string",
   		    "hello!"),
   };

   static const struct opt_conf cfg = {
   	.helpstr = "Example program",
   	.tune = OPTPARSE_IGNORE_ARGV0,
   	.rules = rules,
   	.n_rules = N_RULES
   };


   int main(int argc, char *argv[])
   {
   	union opt_data results[N_RULES];
   	int parse_result;

   	parse_result = optparse_cmd(&cfg, results, argc, argv);

   	if (parse_result < OPTPARSE_OK) {
   		return 1;
   	}

   	printf("Verbosity level is %d\n", results[VERBOSITY].d_int);
   	printf("Flag 's' is %s\n", results[SETTABLE].d_bool? "on" : "off");
   	printf("c = %d (try inputting hex too!)\n", results[INTTHING].d_int);
   	printf("(Optional) argument value: %s\n", results[ARG1].d_cstr);

   	return 0;
   }
