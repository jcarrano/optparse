/**
 * @file
 *
 * @author	Juan I Carrano <juan@carrano.com.ar>
 * @copyright	Copyright (c) 2010-2019 Juan I Carrano
 * @copyright	All rights reserved.
 * ```
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of copyright holders nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * ```
 *
 * @brief Command-line argument parser
 */

#ifndef OPTPARSE_H
#define OPTPARSE_H

#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <stdbool.h>

/**
 * Used to indicate that an option has no short (i.e. single character) variant.
 */
#define OPTPARSE_NO_SHORT '\0'

/**
 * Error codes used by optparse_cmd.
 *
 * User callbacks should use the same error codes. On error, a negative code
 * is returned.
 */
enum OPTPARSE_RESULT {
	OPTPARSE_OK,            /**< Parsing suceeded */
	OPTPARSE_NOMEM,         /**< Not enough memory. Only OPTPARSE_STR or a
				     custom parser may cause this error. */
	OPTPARSE_BADSYNTAX,     /**< Command line is wrongly formed */
	OPTPARSE_BADCONFIG,     /**< The parser configuration is invalid. */
	OPTPARSE_REQHELP        /**< The help option was requested. */
};

/**
 * Built-in actions for options.
 */
enum OPTPARSE_ACTIONS {
	/** Ignore a key and its value. Takes 1 argument. */
	OPTPARSE_IGNORE,

	/** Delegate the action to a callback.
	 *  int _thin_callback(union opt_key key, const char *value,
	 *		       union opt_data *data, const char **msg)
	 */
	OPTPARSE_CUSTOM_ACTION,

	// TODO: figure out if this is needed
	//OPTPARSE_CUSTOM_ACTION_FAT, /**< */

	/** Parse the value as an unsigned int and store it in opt_data::d_uint */
	OPTPARSE_UINT,
	/** Parse the value as an int and store it in opt_data::d_int */
	OPTPARSE_INT,
	/** Parse the value as float and store it in opt_data::d_float */
	OPTPARSE_FLOAT,

	/** Make a copy of the value string with strdup() and place it
	 *  in data::d_str. The string must be deallocated with free().
	 */
	OPTPARSE_STR,

	/** Place a pointer to the value string in data::d_cstr.
	 *
	 * In contrast with OPTPARSE_STR, this does NOT make a copy of the
	 * string. It should not be used if the strings will be modified.
	 */
	OPTPARSE_STR_NOCOPY,

	/** Marker for the end of options that need a value.*/
	_OPTPARSE_MAX_NEEDS_VALUE_END,

	/** Ignore a switch. Takes 0 arguments */
	OPTPARSE_IGNORE_SWITCH=_OPTPARSE_MAX_NEEDS_VALUE_END,
	/** Set opt_data::d_bool to true. Takes 0 arguments.*/
	OPTPARSE_SET_BOOL,
	/** Set opt_data::d_bool to false. Takes 0 arguments.*/
	OPTPARSE_UNSET_BOOL,

	/** Count the number of occurences.
	 *  Increments data::d_int   each time the option is found.
	 */
	OPTPARSE_COUNT,

       /** Print the following strings, except if they are NULL:
	* - opt_conf::helpstr
	* - For each element of opt_conf::opt_rule:
	*  - short_id (tab) long_id (tab) desc
	*
	* In addition it causes the parser to exit with code OPTPARSE_REQHELP.
	*/
	OPTPARSE_DO_HELP,

	_OPTPARSE_POSITIONAL_START,

	/** Indicates that the rule is a mandatory positional argument and not
	 * an option. */
	OPTPARSE_POSITIONAL=_OPTPARSE_POSITIONAL_START,

	/** Indicates that the rule is an optional positional argument and
	 * not an option. */
	OPTPARSE_POSITIONAL_OPT
};

/**
 * Built-in actions for positional arguments.
 *
 * These are a subset of the actions for options.
 */
enum OPTPARSE_POSITIONAL_ACTIONS {
	OPTPARSE_POS_IGNORE = OPTPARSE_IGNORE,
	OPTPARSE_POS_COUNT = OPTPARSE_COUNT,
	OPTPARSE_POS_CUSTOM_ACTION = OPTPARSE_CUSTOM_ACTION,
//    OPTPARSE_POS_CUSTOM_ACTION_FAT = OPTPARSE_CUSTOM_ACTION_FAT,
	OPTPARSE_POS_UINT = OPTPARSE_UINT,
	OPTPARSE_POS_INT = OPTPARSE_INT,
	OPTPARSE_POS_FLOAT = OPTPARSE_FLOAT,
	OPTPARSE_POS_STR = OPTPARSE_STR,
	OPTPARSE_POS_STR_NOCOPY = OPTPARSE_STR_NOCOPY,
};

/** Maximum number of positional arguments supported. */
#define OPTPARSE_MAX_POSITIONAL UCHAR_MAX

/**
 * Identify a positional argument.
 */
struct opt_positionalkey {
	/** Position (counting from zero) of this argument.
	 * Note that options do not contribute to this count. Whether argv[0] is
	 * counted depends on the status of OPTPARSE_IGNORE_ARGV0.
	 */
	unsigned char position;

	/** User-supplied name of this arguments (from opt_rule_t::name) .*/
	const char *name;
};

struct opt_optionkey;

/**
 * Structure to inform user callbacks of the argument or option being
 * parsed.
 */
union opt_key {
	const struct opt_positionalkey *argument;
	const struct opt_optionkey *option;
};

/**
 * Value type for options and arguments.
 */
union opt_data {
	int d_int;
	unsigned int d_uint;
	bool d_bool;
	float d_float;
	/** Pointer to a string allocated by optparse.
	 *  This string must be free()d by the user
	 */
	char *d_str;
	/** Pointer to a string, constant variant.
	 * This will just point to an argv[] element.*/
	const char *d_cstr;
	/** Generic callback data for custom parsers. */
	void *data;

	/**
	 * Callback for user defined actions.
	 *
	 * Prototype:
	 *  int _thin_callback(union opt_key key,
	 *                     const char *value,
	 *                     union opt_data *data, const char **msg)
	 *
	 * When parsing an option, key.option will point to a struct holding the
	 * short and long option keys. When parsing a positional argument
	 * key.option will point to a opt_positionalkey structure holding the
	 * index and the name.
	 *
	 * The callback must return a non-negative value to indicate success.
	 * It should place the result of the conversion in *data.
	 *
	 * During the initialization procedure, it will be called with a NULL
	 * valued data to set the default value. After this, data will never be
	 * NULL.
	 *
	 * The callback can place a message into msg and it will get printed to
	 * the error stream (without causing an error if the result value is not
	 * negative).
	 */
	int (*_thin_callback)(union opt_key, const char *, union opt_data *,
						  const char **);
};


/**
 * Configuration rules for a single command line option.
 */
struct opt_rule {
	/** Parsing type/action. See OPTPARSE_ACTIONS. */
	enum OPTPARSE_ACTIONS action;

	/** Keys for options or action type for positional arguments. */
	union {
		/**
		 * Positional handling specification.
		 *
		 * If the action is OPTPARSE_POSITIONAL or
		 * OPTPARSE_POSITIONAL_OPT, this should hold the required
		 * action.
		 */
		struct {
			enum OPTPARSE_POSITIONAL_ACTIONS pos_action;
			const char *name;
		} argument;

		/** If this rule is for an option (not a positional argument)
		 * this should hold the option keys.*/
		struct opt_optionkey {
			/** Short option name ("-w"), can be OPTPARSE_NO_SHORT.*/
			char short_id;
			/** Long option name ("--width"), Can be NULL */
			const char *long_id;
		} option;
	} action_data;

	/** Help description. Can be NULL */
	const char *desc;

	/** Value for when the option is not given. */
	union opt_data default_value;
};

// /**< The parse results will be placed in this union depending on action */

/**
 * Options that control the behavior of the option parser.
 *
 * The bitmasks (without the _b suffix are probably more useful.
 */
enum OPTPARSE_TUNABLES {
	OPTPARSE_IGNORE_ARGV0_b,
	OPTPARSE_COLLECT_LAST_POS_b,
};

/** Indicates if argv[0] should be skipped */
#define OPTPARSE_IGNORE_ARGV0 (1 << OPTPARSE_IGNORE_ARGV0_b)

/** If this flag is set, the last positional argument rule is applied to all
    extra positional arguments (i.e. it "collects" them all) */
#define OPTPARSE_COLLECT_LAST_POS (1 << OPTPARSE_COLLECT_LAST_POS_b)

typedef uint16_t optparse_tune; /**< Option bitfield */

/**
 * Configuration for the command line parser.
 */
struct opt_conf {
	const char *helpstr; /**< Program's description and general help string. */
	const struct opt_rule *rules;  /**< Array of options. */
	int n_rules;              /**< Number of elements in rules. */
	optparse_tune tune;       /**< Option bitfield. */
};

/**
 * Main interface to the option parser.
 *
 * Short options:
 *
 * A short option that takes a value can be immediately followed by the value
 * in the same argv string. For example "-upeter" would assign "peter" to the
 * "u" option.
 *
 * Short switches can be merged together like "-xj".
 *
 * Dash handling:
 *
 * A double dash (--) indicates the parser that there are no more  options/
 * switches and all the remaining command line arguments are to be interpreted
 * as positional arguments
 *
 * A single dash is interpreted as a positional argument argument.
 *
 * Optional positional arguments:
 *
 * Optional positional arguments MUST follow mandatory arguments. The n-th
 * optional argument cannot be set if the (n-1)th optional argument is not
 * set.
 *
 * @return  Number of positional arguments converted on success, or a
 *          negative error code from OPTPARSE_RESULT on error.
 */
int optparse_cmd(const struct opt_conf *config,
		 union opt_data *result,
		 int argc, const char * const argv[]);

/**
 * Free all strings allocated by OPTPARSE_STR and OPTPARSE_POS_STR.
 *
 * Note that on a parsing error all strings are automatically deallocated.
 *
 * This procedure will set all d_str fields to NULL, so it is safe to call
 * it more than once.
 */
void optparse_free_strings(const struct opt_conf *config,
			   union opt_data *result);

/**
 * @defgroup initializers  Optparse initializers
 * @{
 *
 * @brief   Initialize optparse structures.
 *
 * These initializers can be used to safely construct configurations, without
 * the risk of missing a parameter.
 */

#define _OPTPARSE_IGNORE_SWITCH_INIT    d_int
#define _OPTPARSE_IGNORE_INIT           d_int
#define _OPTPARSE_SET_BOOL_INIT         d_bool
#define _OPTPARSE_UNSET_BOOL_INIT       d_bool
#define _OPTPARSE_COUNT_INIT            d_int
#define _OPTPARSE_CUSTOM_ACTION_INIT    _thin_callback
#define _OPTPARSE_UINT_INIT             d_uint
#define _OPTPARSE_INT_INIT              d_int
#define _OPTPARSE_FLOAT_INIT            d_float
#define _OPTPARSE_STR_INIT              d_str
#define _OPTPARSE_STR_NOCOPY_INIT       d_cstr
#define _OPTPARSE_DO_HELP_INIT          d_int

/**
 * Declare an option.
 *
 * type must be a member of OPTPARSE_ACTIONS, with the "OPTPARSE_" prefix
 * removed. For example:
 *
 * OPTPARSE_O(SET_BOOL, 's', "set", "Set a flag 's'", false)
 */
#define OPTPARSE_O(type, short_, long_, description, default_value_) \
	{.action=OPTPARSE_##type, .action_data.option.short_id=(short_), \
	 .action_data.option.long_id=(long_), .desc=(description), \
	 .default_value._OPTPARSE_##type##_INIT = (default_value_)}

#define _OPTPARSE_P(type0, type, name_, description, default_value_) \
	{.action=(type0), .action_data.argument.pos_action = OPTPARSE_POS_##type, \
	 .action_data.argument.name=(name_), .desc=(description), \
	 .default_value._OPTPARSE_##type##_INIT = (default_value_)}

/**
 * Declare a mandatory positional argument.
 *
 * type must be a member of OPTPARSE_POS_ACTIONS, with the "OPTPARSE_POS" prefix
 * removed. For example:
 *
 * OPTPARSE_P(STR_NOCOPY, "filename", "Output file", "whatever.out")
 */
#define OPTPARSE_P(type, name, description, default_value) \
	_OPTPARSE_P(OPTPARSE_POSITIONAL, type, name, description, default_value)

/**
 * Declare an optional positional argument.
 *
 * Like OPTPARSE_P, but it is not an error if the argument is missing.
 * Remember that optional arguments MUST come after mandatory ones on the
 * opt_conf::rules array.
 */
#define OPTPARSE_P_OPT(type, name, description, default_value) \
	_OPTPARSE_P(OPTPARSE_POSITIONAL_OPT, type, name, description, default_value)

/** @} */

#endif /* OPTPARSE_H */
