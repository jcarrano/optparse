/**
 * @file
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
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "optparse.h"

#define TERM '\0'   /**< String terminator character */
#define OPT '-'     /**< The character that marks an option */

#define P_ERR(...) fprintf(stderr, __VA_ARGS__)

#ifdef NDEBUG
	#define P_DEBUG(...)
#else /* NDEBUG */
	#define P_DEBUG P_ERR
#endif /*NDEBUG */

#define HELP_STREAM stdout /* TODO: remove this */

/**
 * Like fputs, but the string can be null.
 *
 * Returning void saves a bit on code size.
 */
static void safe_fputs(const char *s, FILE *stream)
{
	(s != NULL) ? fputs(s, stream) : 0;
}

/**
 * Like fputc, but nothing is printed if the character is the terminator.
 */
static void safe_fputc(int c, FILE *stream)
{
	(c != TERM) ? fputc(c, stream) : 0;
}

#if (_XOPEN_SOURCE >= 500) || (_POSIX_C_SOURCE > 200809L)
	#define my_strdup strdup
#else /* no posix source */
	char *my_strdup(const char *s)
	{
		char *dup;
		size_t len = strlen(s) + 1;

		if ((dup = malloc(len)) != NULL) {
			memcpy(dup, s, len);
		}

		return dup;
	}
#endif /* posix source */

#define NEEDS_VALUE(rule) ((rule)->action < _OPTPARSE_MAX_NEEDS_VALUE_END)

/**
 * If the string stri starts with a dash, remove it and return a string to
 * the part after the dash.
 *
 * If it does not start with a dash, returns null.
 */
static const char *strip_dash(const char stri[])
{
	const char *ret = NULL;

	if (stri[0] == OPT) {
		ret = &stri[1];
	}
	return ret;
}

/**
 * Return true if the string is not null and not empty.
 */
static bool str_notempty(const char *str)
{
	return str != NULL && str[0] != TERM;
}

/**
 * Return true if the action indicates a positional argument.
 */
static bool _is_argument(enum OPTPARSE_ACTIONS action)
{
	return action >= _OPTPARSE_POSITIONAL_START
	       && action < _OPTPARSE_POSITIONAL_END;
}

/**
 * Return true if the action indicates an optional argument OR option.
 */
static bool _is_optional(enum OPTPARSE_ACTIONS action)
{
	return action != OPTPARSE_POSITIONAL;
}

static void _option_help(const struct opt_rule *rule)
{
	safe_fputc('-', HELP_STREAM);
	safe_fputc(rule->action_data.option.short_id, HELP_STREAM);
	safe_fputs("\t--", HELP_STREAM);
	safe_fputs(rule->action_data.option.long_id, HELP_STREAM);
}

static void _argument_help(const struct opt_rule *rule)
{
	safe_fputs(rule->action_data.argument.name, HELP_STREAM);

	if (_is_optional(rule->action)) {
		safe_fputc('?', HELP_STREAM);
	}
}

/* TODO: make help stream into an argument */
/* TODO: Expose this function in the public API */
static void do_help(const struct opt_conf *config)
{
	int rule_i;

	safe_fputs(config->helpstr, HELP_STREAM);
	safe_fputc('\n', HELP_STREAM);

	for (rule_i = 0; rule_i < config->n_rules; rule_i++) {
		const struct opt_rule *rule = config->rules + rule_i;

		if (_is_argument(rule->action)) {
			_argument_help(rule);
		} else {
			_option_help(rule);
		}

		safe_fputc('\t', HELP_STREAM);
		safe_fputs(rule->desc, HELP_STREAM);
		safe_fputc('\n', HELP_STREAM);
	}
}

/**
 * Check that there are no optional arguments before non optional ones.
 *
 * @return zero on success, nonzero on error.
 */
static bool sanity_check(const struct opt_conf *config)
{
	int rule_i;
	bool found_optional = false;

	for (rule_i = 0; rule_i < config->n_rules; rule_i++) {
		bool is_positional = _is_argument(config->rules[rule_i].action);
		bool is_optional = _is_optional(config->rules[rule_i].action);

		if (!is_positional) {
			continue;
		}

		if (found_optional && !is_optional) {
			return true;
		}

		found_optional = found_optional || is_optional;
	}

	return false;
}

static int do_user_callback(const struct opt_rule *rule, union opt_data *dest,
			    int positional_idx, const char *value,
			    const char **custom_message)
{
	union opt_key key;
	struct opt_positionalkey pkey;
	int ret;

	if (_is_argument(rule->action)) {
		pkey.position = positional_idx;
		pkey.name = rule->action_data.argument.name;
		key.argument = &pkey;
	} else {
		key.option = &rule->action_data.option;
	}

	ret = rule->default_value._thin_callback(key, value,
						 dest, custom_message);

	return ret;
}

static enum OPTPARSE_ACTIONS real_action(const struct opt_rule *rule)
{
	return _is_argument(rule->action)? rule->action_data.argument.pos_action
				         : rule->action;
}

/**
 * Execute the action associated with an argument.
 *
 * positional_idx is only used for custom commands in positional arguments.
 * value is only used for commands that need it.
 * This assumes key and value are not null if they should not be.
 *
 * @return  An exit code from OPTPARSE_RESULT.
 */
static int do_action(const struct opt_rule *rule,
		     union opt_data *dest,
		     int positional_idx, const char *value,
		     const char **msg)
{
	int ret = OPTPARSE_OK;
	char *end_of_conversion;
	enum OPTPARSE_ACTIONS action = real_action(rule);

	switch (action) {
		case OPTPARSE_IGNORE: case OPTPARSE_IGNORE_SWITCH:
			break;
		case OPTPARSE_INT:
			dest->d_int = strtol(value, &end_of_conversion, 0);
			if (*end_of_conversion != '\0') {
				*msg = "Expected integer";
				ret = -OPTPARSE_BADSYNTAX;
			}
			break;
		case OPTPARSE_UINT:
			dest->d_uint = strtoul(value, &end_of_conversion, 0);
			if (*end_of_conversion != '\0') {
				*msg = "Expected integer";
				ret = -OPTPARSE_BADSYNTAX;
			}
			break;
		case OPTPARSE_FLOAT:
			dest->d_float = strtof(value, &end_of_conversion);
			if (*end_of_conversion != '\0') {
				*msg = "Expected real number";
				ret = -OPTPARSE_BADSYNTAX;
			}
			break;
		case OPTPARSE_STR_NOCOPY:
			dest->d_cstr = value;
			break;
		case OPTPARSE_SET_BOOL:
			dest->d_bool = true;
			break;
		case OPTPARSE_UNSET_BOOL:
			dest->d_bool = false;
			break;
		case OPTPARSE_COUNT:
			dest->d_int++;
			break;
		case OPTPARSE_STR:
			{
				/* avoid memory leak if the option is given
				 * multiple times. */
				free(dest->d_str);
				dest->d_str = my_strdup(value);
				if (dest->d_str == NULL) {
					*msg = "Parser out of memory";
					ret = -OPTPARSE_NOMEM;
				}
			}
			break;
		case OPTPARSE_DO_HELP:
			P_DEBUG("do_action found OPTPARSE_DO_HELP\n");
			 /*this is a meta-action and has to be implemented in
			   generic_parser!! */
			assert(0);
			break;
		case OPTPARSE_CUSTOM_ACTION:
			ret = do_user_callback(rule, dest, positional_idx, value, msg);
			break;
		default:
			P_DEBUG("Unknown action: %d\n", rule->action);
			ret = -OPTPARSE_BADCONFIG;
			break;
	}

	return ret;
}

static bool _match_optionkey(struct opt_optionkey opt_key,
							 const char *long_id, char short_id)
{
	return (short_id && opt_key.short_id == short_id)
	       || ((long_id != NULL) && (opt_key.long_id != NULL)
		   && strcmp(long_id, opt_key.long_id) == 0);
}

/**
 * Find a rule with the given short id or long id.
 *
 * A short id of 0 never matches. A NULL long id never matches.
 */
static const struct opt_rule *find_opt_rule(const struct opt_conf *config,
					    const char *long_id,
					    char short_id)
{
	const struct opt_rule *this_rule = config->rules;
	int i = config->n_rules;

	/* This iteration seems weird but it provides perceptible code size
	 * savings in both gcc and clang (at least in cortexm/thumb).*/
	while(i--) {
		if (!_is_argument(this_rule->action)
		    && _match_optionkey(this_rule->action_data.option, long_id,
					short_id)
		    ) {
			return this_rule;
		}
		this_rule++;
	}

	return NULL;
}

/**
 * Find the positional argument handler for the arg_n-th position (arg_n counts
 * from zero).
 *
 * If repeat_last is true, if arg_n is greater than or equal to the number of
 * available handler, the last one (if any) is returned.
 *
 * @returns     pointer to the matching rule, or NULL if none is found.
 */
static const struct opt_rule *find_arg_rule(const struct opt_conf *config,
					    int arg_n)
{
	int rule_i;
	int last_handler;
	int handlers_found = 0;
	bool repeat_last = !!(config->tune & OPTPARSE_COLLECT_LAST_POS);

	for (rule_i = 0; rule_i < config->n_rules; rule_i++) {
		const struct opt_rule this_rule = config->rules[rule_i];

		if (_is_argument(this_rule.action)) {
			handlers_found++;
			last_handler = rule_i;
			if (handlers_found > arg_n) {
				return config->rules + rule_i;
			}
		}
	}

	return (repeat_last && handlers_found)? config->rules + last_handler
						: NULL;
}

/**
 * Get the memory location where the parse result should be stored.
 */
static union opt_data *get_destination(const struct opt_conf *config,
					const struct opt_rule *rule,
					union opt_data *result_base)
{
	return result_base + (rule - config->rules);
}

/**
 * Copy over the default values from the rules array to the result array.
 *
 * For custom actions, the callback will be called with value = NULL.
 *
 * This procedure also counts the number of required positional arguments and
 * returns it in n_required (which must NOT be null).
 *
 * On error, n_required may not be accurate.
 *
 * This procedure ensures guarantees that no result item will have a wild
 * pointer in d_str (i.e, it will point to an allocated block or be NULL.)
 */
static int assign_default(const struct opt_conf *config,
			  union opt_data *result, int *n_required)
{
	int rule_i;
	int error = 0;
	int positional_idx = 0;
	int _required = 0;

	for (rule_i = 0; !error && rule_i < config->n_rules; rule_i++) {
		enum OPTPARSE_ACTIONS action;
		const struct opt_rule *this_rule = config->rules + rule_i;
		const char *msg = NULL;

		if (!_is_optional(this_rule->action)) {
			_required++;
		}

		if (_is_argument(this_rule->action)) {
			positional_idx++;
		}

		action = real_action(this_rule);

		if (action == OPTPARSE_STR
		    && this_rule->default_value.d_str != NULL) {
			result[rule_i].d_str = my_strdup(this_rule->default_value.d_str);
			if (result[rule_i].d_str == NULL) {
				P_DEBUG("initialization failed: out of memory\n");
				error = -OPTPARSE_NOMEM;
			}
		} else if (action != OPTPARSE_CUSTOM_ACTION) {
			result[rule_i] = this_rule->default_value;
		} else {
			error = do_user_callback(this_rule, result + rule_i,
						 positional_idx, NULL, &msg);
			safe_fputs(msg, HELP_STREAM);
			if (error) {
				P_DEBUG("User cb at index %d failed in init with code %d.\n",
						rule_i, error);
			}
		}
	}

	/* If we exited early, ensure that we do not leave any wild pointer.
	 * We will have to free them later. */
	for (; rule_i < config->n_rules; rule_i++) {
		if (real_action(config->rules + rule_i) == OPTPARSE_STR) {
			result[rule_i].d_str = NULL;
		}
	}

	*n_required = _required;

	return error;
}


void optparse_free_strings(const struct opt_conf *config, union opt_data *result)
{
	const struct opt_rule *this_rule = config->rules;
	int i = config->n_rules;

	/* This iteration seems weird but it provides perceptible code size savings
	 * in both gcc and clang (at least in cortexm/thumb).*/
	while(i--) {
		if (real_action(this_rule) == OPTPARSE_STR) {
			free(result->d_str);
			result->d_str = NULL;
		}
		result++;
		this_rule++;
	}
}

int optparse_cmd(const struct opt_conf *config,
				 union opt_data *result,
				 int argc, const char * const argv[])
{
	int error = 0, i, no_more_options = 0;
	/* Index of the next positional argument */
	int positional_idx = 0;
	int n_required;
	/* Used for handling combined switches like -axf (equivalent to -a -x -f)
	 * Instead of advancing argv, we keep reading from the string*/
	const char *pending_opt = NULL;

	/* TODO: remove assert, return badconfig */
	if (sanity_check(config)) {
		return -OPTPARSE_BADCONFIG;
	}

	i = (config->tune & OPTPARSE_IGNORE_ARGV0) ? 1 : 0;

	error = assign_default(config, result, &n_required);
	if (error) {
		P_ERR("Error initializing default values.\n");
	}

	while (error >= OPTPARSE_OK && i < argc) {
		const char *key, *value, *msg = NULL;
		const struct opt_rule *curr_rule = NULL;
		int positional_idx_delta = 0;

		if (!no_more_options
		    && ((pending_opt != NULL)
			|| str_notempty(key = strip_dash(argv[i])))) {

			bool is_long;

			if (pending_opt == NULL) {
				const char *tmp_key;
				is_long = (tmp_key = strip_dash(key)) != NULL;

				if (is_long) {
					key = tmp_key;
				}

				if (is_long && tmp_key[0] == TERM) { /* we read a "--" */
					no_more_options = 1;
					goto parse_loop_end;
				}
			} else { /* pending_opt != NULL, we have combined switches */
				is_long = false;
				key = pending_opt;
				pending_opt = NULL; /* We reset this because we need to check again
						       if the current option is a switch*/
			}

			curr_rule = find_opt_rule(
				config, is_long ? key : NULL,
						  (!is_long) ? key[0] : 0);

			if (curr_rule != NULL) {
				if (curr_rule->action == OPTPARSE_DO_HELP) {
					do_help(config);
					error = -OPTPARSE_REQHELP; /* BYE! */
				} else if (NEEDS_VALUE(curr_rule)) {
					if (!is_long && key[1] != TERM) {
						/* This allows one to write the option value like -d12.6 */
						value = key + 1;
					} else if (i < (argc - 1)) {
						/* "i" is only incremented here and at the end of the loop */
						value = argv[++i];
					} else {
						msg = "Option needs value";
						error = -OPTPARSE_BADSYNTAX; /* BYE! */
					}
				} else { /* Handle switches (no arguments) */
					value = NULL;
					if (!is_long && key[1] != TERM) {
						pending_opt = key + 1;
					}
				}
			} else {
				msg = "Unknown option";
				error = -OPTPARSE_BADSYNTAX;
			}
		} else {
			curr_rule = find_arg_rule(config, positional_idx);
			value = argv[i];
			positional_idx_delta = 1;

			if (positional_idx > OPTPARSE_MAX_POSITIONAL) {
				msg = "Max number of arguments exceeded";
				error = -OPTPARSE_BADSYNTAX;
			}

			if (curr_rule == NULL) {
				msg = "Too many arguments";
				error = -OPTPARSE_BADSYNTAX; /* BYE! */
			}
		}

		if (error >= OPTPARSE_OK && curr_rule != NULL) {
			error = do_action(curr_rule,
					  get_destination(config, curr_rule, result),
					  positional_idx, value, &msg); /* BYE? */
		}

		positional_idx += positional_idx_delta;

		if (msg) {
			P_ERR("%s: %s\n", msg, argv[i]);
		}

parse_loop_end:
		if (pending_opt == NULL) {
			i++; /* "i" is only incremented here and above */
		}
	}

	if (error >= OPTPARSE_OK && n_required > positional_idx) {
		P_ERR("%d argument required but only %d given\n", n_required,
		      positional_idx);
		error = -OPTPARSE_BADSYNTAX;
	}

	if (error < OPTPARSE_OK) {
		optparse_free_strings(config, result);
	}

	return error >= OPTPARSE_OK? positional_idx : error;
}
