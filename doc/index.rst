========
optparse
========

Summary
=======

This module provides a generic way of describing and parsing command line
arguments. Features:

- Supports both short form ("-k") and long form ("--key") options.
- Supports both mandatory and optional positional arguments.
- Built-in parsers for most common options/argument types.
- Automatically formats and prints a help string.
- Supports custom parsers via callbacks.
- The parser specification is designed to be read-only so it can be
  placed in the .text section of a binary (and in an MCU, into the FLASH).

Limitations
-----------

- There is no way to tell that an option was not specified: options have
  default value and it is generally not possible to tell this value apart
  from a user supplied value.
- When using custom actions, a character used as a short option key should
  NOT be used as a long option key in another rule or else the user callback
  will not be able to tell them apart (may or may not be an issue.)

Terminology
===========

option
   Optional value. Consists of a key an possibly a value. Each type of option
   has a predefined number of arguments it takes that may be either 0 or 1.

   Example: ``-v --number 5 -k hello``

   Options (key, value): (v, none), (number, 5), (k, hello)

   Options are identified by a string (or character) key.

switch
   An option taking 0 values.

(positional) argument:
   A value without a key. For example, in ``-c 3 abcd``, abcd is an argument.
   Arguments can be either mandatory or optional and are identified by an
   integer index. Because of this, optional arguments must always come AFTER
   mandatory ones.

short-option
   An option that starts with a single dash and consists of a single character.
   For example ``-k 78``, ``-c``, ``-f filename``.

long-option
   An option that starts with two dashes and consists of one or more characters
   (i.e. a string). For example ``--echo``, ``--factor 1.5``.

Usage
=====

Initialize the required :doxy:r:`opt_conf` and :doxy:r:`opt_rule`
structures and call :doxy:r:`optparse.h::optparse_cmd`.

Tips for defining rules
-----------------------

1. Make an enum with the name of the parameters (options and arguments) and
   with a last element marking its length::

     enum {HEIGHT, WIDTH, VERBOSE, ETC, .... , N_PARAMETERS

2. Declare ``const struct opt_rule rules[N_PARAMETERS];``
3. Initialize it. If your compiler supports specifying sub-objects in array and
   struct/union initializers, they initializer macros can be used. These
   provide a fail-safe way to construct an array (i.e. they ensure that the
   correct tags and union elements are set::

     const struct opt_rule rules[N_PARAMETERS] = {
       [WIDTH] = OPTPARSE_O_INT('w', "width", 640, "Display height in px"),
       [HEIGHT] = OPTPARSE_O_INT('h', "height", 480, "Display height in px"),
       ... etc ...
     }

4. Declare and initialize an :doxy:r:`opt_conf` object.

Reference
=========

.. doxy:c:: optparse.h::*
   :children:


Indices and tables
==================

* :ref:`genindex`
* :ref:`search`
