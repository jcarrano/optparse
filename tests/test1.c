/**
 * @file
 * @author  Juan I Carrano <juan@carrano.com.ar.
 *
 * Unit tests for optparse.
 */

#include <string.h>
#include <malloc.h>

#include "unity.h"

#include "optparse.h"

void setUp(void)
{
}

void tearDown(void)
{
}

/**
 * Test an empty parser.
 *
 * According to the docs, all fields of struct opt_conf can be zero. In that case
 * the parser will fail except if argc = 0.
 *
 * Also, if argc=0, argv=NULL should be OK.
 */
static void test_optparse_trivial(void)
{
	struct opt_conf cfg;
	int parse_result;
	static const char *argv1[] = { "batman" };
	static const char *argv2[] = { "-y", "3" };
	static const char *argv_dash[] = { "--" };

	memset(&cfg, 0, sizeof(cfg));

	/* Do nothing: should never fail. */
	parse_result = optparse_cmd(&cfg, NULL, 0, NULL);
	TEST_ASSERT_EQUAL_INT(OPTPARSE_OK, parse_result);

	/* Ignore the first argument. */
	cfg.tune |= OPTPARSE_IGNORE_ARGV0;

	parse_result = optparse_cmd(&cfg, NULL, 1, argv1);
	TEST_ASSERT_EQUAL_INT(0, parse_result);

	/* Do not ignore the first argument. */
	cfg.tune = 0;

	parse_result = optparse_cmd(&cfg, NULL, 1, argv1);
	TEST_ASSERT_EQUAL_INT(-OPTPARSE_BADSYNTAX, parse_result);

	parse_result = optparse_cmd(&cfg, NULL, 2, argv2);
	TEST_ASSERT_EQUAL_INT(-OPTPARSE_BADSYNTAX, parse_result);

	/* This is OK because "--" is eaten up by the parser */
	parse_result = optparse_cmd(&cfg, NULL, 1, argv_dash);
	TEST_ASSERT_EQUAL_INT(OPTPARSE_OK, parse_result);
}

enum _rules {
	VERBOSITY,
	SETTABLE,
	UNSETTABLE,
	ARG1,
	WILLNOTUSE,
	QTHING,
	KEY,
	HELP_OPT,
	FLOATTHING,
	INTTHING,
	ARG2,
	UINTTHING,
	IGNORED1,
	IGNORED2,
	ARG3,
	ARG4,
	ARG5,
	N_RULES,
};

#define MAX_ARGS 3

int count_letters(union opt_key key, const char *value,
				  union opt_data *dest,
				  const char **message)
{
	bool valid;
	(void)message;

	/* If the value is null, we are being requested the default value. */
	if (value == NULL) {
		dest->d_uint = 402;
	} else {
		dest->d_uint = strlen(value);
	}

	/* Test error handling. */
	valid = dest->d_uint != 1;

	return (strcmp("count-my-letters", key.argument->name) == 0 && valid)?
			-OPTPARSE_OK : -OPTPARSE_BADSYNTAX;
}

static const struct opt_rule rules[N_RULES] = {
[VERBOSITY] = OPTPARSE_O(COUNT, 'v', "verbose",
			 "Verbosity level (can be specified multiple times)", 0),

[WILLNOTUSE] = OPTPARSE_O(COUNT, 'W', NULL,
			  "Do not use (check initialization)", 101),

[SETTABLE] = OPTPARSE_O(SET_BOOL, 's', NULL, "Set a flag 's'", false),

[UNSETTABLE] = OPTPARSE_O(UNSET_BOOL, 'u', NULL, "Unset the flag 'u'", true),

[QTHING] = OPTPARSE_O(STR_NOCOPY, 'q', "qthing", "Set the string q.", "nothing"),

[KEY] = OPTPARSE_O(STR, OPTPARSE_NO_SHORT, "key", "Choose a key", NULL),

/* (-f, --q)  just to add some confusion! */
[FLOATTHING] = OPTPARSE_O(FLOAT, 'f', "q", "Set a float", 1.0f),

[INTTHING] = OPTPARSE_O(INT, 'c', NULL, "Set an integer (try a negative value)",
			-10),

[UINTTHING] = OPTPARSE_O(UINT, OPTPARSE_NO_SHORT, "cc", "Set uint", 19),

[IGNORED1] = OPTPARSE_O(IGNORE, 'i', NULL, "No op (takes 1 arg)", 0),
[IGNORED2] = OPTPARSE_O(IGNORE_SWITCH, '9', "124", "No op switch", 0),
[HELP_OPT] = OPTPARSE_O(DO_HELP, 'h', "help", "Show this help", 0),
/* positionals */
[ARG1] = OPTPARSE_P(STR_NOCOPY, "first-argument", "Just store this string",
		    "hello!"),
[ARG2] = OPTPARSE_P(CUSTOM_ACTION, "count-my-letters",
		    "Store the n. of letter in this str.", count_letters),

[ARG3] = OPTPARSE_P_OPT(STR_NOCOPY, "optional-stuff",
			"This is optional.", NULL),
[ARG4] = OPTPARSE_P_OPT(INT, "an optional integer",
			"check that it gets correctly parsed", 89),
[ARG5] = OPTPARSE_P_OPT(CUSTOM_ACTION, "count-my-letters",
			"Store the n. of letter in this str.", count_letters),
};

static const struct opt_conf cfg = {
	.helpstr = "Test program",
	.tune = OPTPARSE_IGNORE_ARGV0,
	.rules = rules,
	.n_rules = N_RULES
};

/**
 * Test all actions.
 */
static void test_optparse_basic(void)
{
	union opt_data results[N_RULES];
	int parse_result;
	static const char *argv[] = {"test", "-c", "-3", "--key", "hello",
				     "-vf5.5", "-qpasted", "-vv9", "--verbose",
				     "-i", "-v", /* -v is an argument to -i */
				     "-s", "-u", "-", "quack", "--124", "--cc",
				     "423", "--", "-qwerty"};

	parse_result = optparse_cmd(&cfg, results,
				    sizeof(argv) / sizeof(*argv), argv);

	//TEST_ASSERT_EQUAL_INT(101, results[WILLNOTUSE].d_int);

	/* 3, because we supplied 3 positional arguments. */
	TEST_ASSERT_EQUAL_INT(3, parse_result);
	TEST_ASSERT_EQUAL_INT(89, results[ARG4].d_int);
	TEST_ASSERT_EQUAL_STRING("-", results[ARG1].d_cstr);
	TEST_ASSERT_EQUAL_INT(strlen("quack"), results[ARG2].d_uint);
	TEST_ASSERT_EQUAL_STRING("-qwerty", results[ARG3].d_cstr);

	TEST_ASSERT_EQUAL_INT(101, results[WILLNOTUSE].d_int);

	TEST_ASSERT_EQUAL_INT(-3, results[INTTHING].d_int);
	TEST_ASSERT_EQUAL_INT(423, results[UINTTHING].d_uint);
	TEST_ASSERT_EQUAL_INT(4, results[VERBOSITY].d_int);
	TEST_ASSERT_EQUAL_STRING("hello", results[KEY].d_str);
	TEST_ASSERT_EQUAL_STRING("pasted", results[QTHING].d_cstr);
	TEST_ASSERT_EQUAL_FLOAT(5.5f, results[FLOATTHING].d_float);
	TEST_ASSERT_TRUE(results[SETTABLE].d_bool);
	TEST_ASSERT_FALSE(results[UNSETTABLE].d_bool);


	free(results[KEY].d_str);
}

/**
 * Test help.
 */
static void test_optparse_help(void)
{
	union opt_data results[N_RULES];
	int parse_result;
	static const char *argv_help[] = {"test", "-c", "3", "-h",
					  "--invalid-option"};

	parse_result = optparse_cmd(&cfg, results,
				    sizeof(argv_help) / sizeof(*argv_help), argv_help);

	TEST_ASSERT_EQUAL_INT(-OPTPARSE_REQHELP, parse_result);
}

/**
 * Test default value assignments.
 */
static void test_optparse_default(void)
{
	union opt_data results[N_RULES];
	int parse_result;
	static const char *argv[] = {NULL, "b", "cc"};

	parse_result = optparse_cmd(&cfg, results,
				    sizeof(argv) / sizeof(*argv), argv);

	TEST_ASSERT_EQUAL_INT(2, parse_result);

#define my_TEST_ASSERT_EQUAL_INT(idx, field) \
  TEST_ASSERT_EQUAL_INT(rules[idx].default_value.field, results[idx].field)

#define my_TEST_ASSERT_EQUAL_STRING(idx, field) \
  TEST_ASSERT_EQUAL_STRING(rules[idx].default_value.field, results[idx].field)

#define my_TEST_ASSERT_EQUAL_FLOAT(idx, field) \
  TEST_ASSERT_EQUAL_FLOAT(rules[idx].default_value.field, results[idx].field)

	TEST_ASSERT_EQUAL_STRING("b", results[ARG1].d_cstr);
	TEST_ASSERT_EQUAL_INT(2, results[ARG2].d_uint);
	my_TEST_ASSERT_EQUAL_STRING(ARG3, d_cstr);
	my_TEST_ASSERT_EQUAL_INT(ARG4, d_int);

	/* this one has a special behavior. */
	TEST_ASSERT_EQUAL_INT(402, results[ARG5].d_uint);

	my_TEST_ASSERT_EQUAL_INT(WILLNOTUSE, d_int);
	my_TEST_ASSERT_EQUAL_INT(INTTHING, d_int);
	my_TEST_ASSERT_EQUAL_INT(UINTTHING, d_uint);
	my_TEST_ASSERT_EQUAL_INT(VERBOSITY, d_int);
	my_TEST_ASSERT_EQUAL_STRING(KEY, d_str);
	my_TEST_ASSERT_EQUAL_STRING(QTHING, d_cstr);
	my_TEST_ASSERT_EQUAL_FLOAT(FLOATTHING, d_float);
	my_TEST_ASSERT_EQUAL_INT(SETTABLE, d_bool);
	my_TEST_ASSERT_EQUAL_INT(UNSETTABLE, d_bool);

	free(results[KEY].d_str);
}

/**
 * Test number syntax errors (int)
 */
static void test_optparse_int_err(void)
{
	union opt_data results[N_RULES];
	int parse_result;
	static const char *argv[] = { NULL, "x1", "x2", "-c", "45."};

	/* first check that without the -c option it works */
	parse_result = optparse_cmd(&cfg, results,
				    sizeof(argv) / sizeof(*argv) - 2, argv);

	TEST_ASSERT_EQUAL_INT(2, parse_result);

	/* Now check that the -c option with invalid argument fails */
	parse_result = optparse_cmd(&cfg, results,
				    sizeof(argv) / sizeof(*argv), argv);


	TEST_ASSERT_EQUAL_INT(-OPTPARSE_BADSYNTAX, parse_result);
}

/**
 * Test number syntax errors (float)
 */
static void test_optparse_float_err(void)
{
	union opt_data results[N_RULES];
	int parse_result;
	static const char *argv[] = {NULL, "x1", "x2", "-f", "46.7.9"};

	/* first check that without the -f option it works */
	parse_result = optparse_cmd(&cfg, results,
				    sizeof(argv) / sizeof(*argv) - 2, argv);

	TEST_ASSERT_EQUAL_INT(2, parse_result);

	/* Now check that the -f option with invalid argument fails */
	parse_result = optparse_cmd(&cfg, results,
				    sizeof(argv) / sizeof(*argv), argv);


	TEST_ASSERT_EQUAL_INT(-OPTPARSE_BADSYNTAX, parse_result);
}

int main(void) {
	UNITY_BEGIN();
	RUN_TEST(test_optparse_trivial);
	RUN_TEST(test_optparse_basic);
	RUN_TEST(test_optparse_default);
	RUN_TEST(test_optparse_help);
	RUN_TEST(test_optparse_int_err);
	RUN_TEST(test_optparse_float_err);
	return UNITY_END();
}
