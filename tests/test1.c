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
	COPYME,
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

[COPYME] = OPTPARSE_O(STR, OPTPARSE_NO_SHORT, "copyme", NULL, "free-me"),

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
	TEST_ASSERT_EQUAL_UINT(423, results[UINTTHING].d_uint);
	TEST_ASSERT_EQUAL_INT(4, results[VERBOSITY].d_int);
	TEST_ASSERT_EQUAL_STRING("hello", results[KEY].d_str);
	TEST_ASSERT_EQUAL_STRING("pasted", results[QTHING].d_cstr);
	TEST_ASSERT_EQUAL_STRING("free-me", results[COPYME].d_str);
	TEST_ASSERT_EQUAL_FLOAT(5.5f, results[FLOATTHING].d_float);
	TEST_ASSERT_TRUE(results[SETTABLE].d_bool);
	TEST_ASSERT_FALSE(results[UNSETTABLE].d_bool);


	free(results[KEY].d_str);
	free(results[COPYME].d_str);
}

/* test giving too few arguments */
static void test_optparse_toofew(void)
{
	union opt_data results[N_RULES];
	int parse_result;
	static const char *argv[] = {"prog", "aa", "bbb", "--key"};

	parse_result = optparse_cmd(&cfg, results, 0, argv);
	TEST_ASSERT_EQUAL_INT(-OPTPARSE_BADSYNTAX, parse_result);

	parse_result = optparse_cmd(&cfg, results, 1, argv);
	TEST_ASSERT_EQUAL_INT(-OPTPARSE_BADSYNTAX, parse_result);

	parse_result = optparse_cmd(&cfg, results, 2, argv);
	TEST_ASSERT_EQUAL_INT(-OPTPARSE_BADSYNTAX, parse_result);

	parse_result = optparse_cmd(&cfg, results, 3, argv);
	TEST_ASSERT_EQUAL_INT(2, parse_result);
	optparse_free_strings(&cfg, results);

	TEST_ASSERT_EQUAL_STRING("aa", results[ARG1].d_cstr);
	TEST_ASSERT_EQUAL_UINT(3, results[ARG2].d_uint);
	optparse_free_strings(&cfg, results);

	parse_result = optparse_cmd(&cfg, results, 4, argv);
	TEST_ASSERT_EQUAL_INT(-OPTPARSE_BADSYNTAX, parse_result);
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

	static const char *argv_help2[] = {"test", "-c", "3", "-h",
					   "--key", "i_should_be_freed",
					   "--invalid-option"};

	parse_result = optparse_cmd(&cfg, results,
				    sizeof(argv_help2) / sizeof(*argv_help2), argv_help2);

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

	optparse_free_strings(&cfg, results);
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
	optparse_free_strings(&cfg, results);

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
	optparse_free_strings(&cfg, results);

	/* Now check that the -f option with invalid argument fails */
	parse_result = optparse_cmd(&cfg, results,
				    sizeof(argv) / sizeof(*argv), argv);


	TEST_ASSERT_EQUAL_INT(-OPTPARSE_BADSYNTAX, parse_result);
}

/**
 * Test number syntax errors (float)
 */
static void test_optparse_uint_err(void)
{
	union opt_data results[N_RULES];
	int parse_result;
	static const char *argv_good[] = {NULL, "x1", "x2", "--cc", "95"};
	static const char *argv_bad1[] = {NULL, "x1", "x2", "--cc", "-12"};
	static const char *argv_bad2[] = {NULL, "x1", "x2", "--cc", "j"};
	int argc = sizeof(argv_good) / sizeof(*argv_good);

	parse_result = optparse_cmd(&cfg, results, argc, argv_good);
	TEST_ASSERT_EQUAL_INT(2, parse_result);
	optparse_free_strings(&cfg, results);

	parse_result = optparse_cmd(&cfg, results, argc, argv_bad1);
	TEST_ASSERT_EQUAL_INT(2, parse_result);
	TEST_ASSERT_EQUAL_UINT((unsigned int)(-12), results[UINTTHING].d_uint);
	optparse_free_strings(&cfg, results);

	parse_result = optparse_cmd(&cfg, results, argc, argv_bad2);
	TEST_ASSERT_EQUAL_INT(-OPTPARSE_BADSYNTAX, parse_result);
}

enum _rules_bad1 {
	ARG1p,
	ARG1r,
	N_BAD1_RULES
};

static const struct opt_rule rules_bad1[N_BAD1_RULES] = {
[ARG1p] = OPTPARSE_P_OPT(STR_NOCOPY, "optional-stuff",
			"This is optional.", "k"),
[ARG1r] = OPTPARSE_P(STR_NOCOPY, "first-argument", "This is required",
		    "m")
};

static const struct opt_conf cfg_bad1 = {
	.helpstr = "Bad config 1",
	.tune = 0,
	.rules = rules_bad1,
	.n_rules = N_BAD1_RULES
};

static void test_optparse_invalid_positional(void)
{
	union opt_data results[N_BAD1_RULES];
	int parse_result;
	static const char *argv[] = {"x1", "x2"};

	parse_result = optparse_cmd(&cfg_bad1, results,
				    sizeof(argv) / sizeof(*argv) , argv);

	TEST_ASSERT_EQUAL_INT(-OPTPARSE_BADCONFIG, parse_result);
}



static const struct opt_rule rules_trivia2[] = {
	OPTPARSE_P_OPT(STR_NOCOPY, "optional-stuff", "This is optional.", "x")
};

static const struct opt_conf cfg_trivia2 = {
	.helpstr = "Trivial example 2",
	.tune = 0,
	.rules = rules_trivia2,
	.n_rules = 1
};

static void test_optparse_one_arg(void)
{
	union opt_data results[1];
	int parse_result;
	static const char *argv[] = {"x1", "x2"};

	parse_result = optparse_cmd(&cfg_trivia2, results, 0, argv);
	TEST_ASSERT_EQUAL_INT(0, parse_result);
	optparse_free_strings(&cfg_trivia2, results);

	parse_result = optparse_cmd(&cfg_trivia2, results, 1, argv);
	TEST_ASSERT_EQUAL_INT(1, parse_result);
	optparse_free_strings(&cfg_trivia2, results);

	parse_result = optparse_cmd(&cfg_trivia2, results, 2, argv);
	TEST_ASSERT_EQUAL_INT(-OPTPARSE_BADSYNTAX, parse_result);
}


int fail_cb(union opt_key key, const char *value,
	    union opt_data *dest,
	    const char **message)
{
	/* If the value is null, we are being requested the default value. */
	if (value == NULL) {
		switch (key.option->short_id) {
			case 'm':
				*message = "I like failing with nomem\n";
				return -OPTPARSE_NOMEM;
			case 's':
				*message = "I like failing with syntax error\n";
				return -OPTPARSE_BADSYNTAX;
			case 'c':
				*message = "I like failing with cfg error\n";
				return -OPTPARSE_BADCONFIG;
		}
	} else {
		dest->d_uint = value[0];
		return -OPTPARSE_OK;
	}
}

static struct opt_rule rules_fail_m[] = {
	OPTPARSE_O(STR, OPTPARSE_NO_SHORT, "copyme", "String to copy", "free-me"),
	OPTPARSE_O(CUSTOM_ACTION, 'm', NULL, "This will fail.", fail_cb),
	OPTPARSE_O(STR, OPTPARSE_NO_SHORT, "imdynamic", "Another String to copy", "check-wild")
};


static void test_init_fail(void)
{
	static const struct opt_conf cfg_failm = {
		.helpstr = "Failure example",
		.tune = 0,
		.rules = rules_fail_m,
		.n_rules = 3
	};

	union opt_data results[3];
	int parse_result;
	static const char *argv[] = {"-s"};

	parse_result = optparse_cmd(&cfg_failm, results, 1, argv);
	TEST_ASSERT_EQUAL_INT(-OPTPARSE_NOMEM, parse_result);

	rules_fail_m[1].action_data.option.short_id = 'c';
	parse_result = optparse_cmd(&cfg_failm, results, 1, argv);
	TEST_ASSERT_EQUAL_INT(-OPTPARSE_BADCONFIG, parse_result);

	rules_fail_m[1].action_data.option.short_id = 's';
	parse_result = optparse_cmd(&cfg_failm, results, 1, argv);
	TEST_ASSERT_EQUAL_INT(-OPTPARSE_BADSYNTAX, parse_result);

	rules_fail_m[1].action = 50;
	parse_result = optparse_cmd(&cfg_failm, results, 1, argv);
	TEST_ASSERT_EQUAL_INT(-OPTPARSE_BADCONFIG, parse_result);

	TEST_ASSERT_EQUAL_PTR(NULL, results[0].d_str);
	TEST_ASSERT_EQUAL_PTR(NULL, results[2].d_str);
}

/* this is the most inefficient algoerithm ever. Maybe it is a reason to
 * introduce fat callbacks.
 */
int paste_strings(union opt_key key, const char *value,
		  union opt_data *dest,
		  const char **message)
{
	/* If the value is null, we are being requested the default value. */
	if (value == NULL) {
		dest->d_str = NULL;
	} else {
		size_t l0 = (dest->d_str == NULL)? 0 : strlen(dest->d_str);
		size_t l = l0 + strlen(value);
		dest->d_str = realloc(dest->d_str, l+1);

		if (!l0) {
			*dest->d_str = 0;
		}

		strcat(dest->d_str, value);
	}

	return -OPTPARSE_OK;
}

static const struct opt_rule optcollector[] = {
	OPTPARSE_P_OPT(STR, "thing", "String to copy", "free-me"),
	OPTPARSE_P_OPT(CUSTOM_ACTION, "collect-all-this", "Will paste all strings", paste_strings),
};

static const struct opt_conf cfg_optcollector = {
	.helpstr = "test last arg collection",
	.tune = OPTPARSE_COLLECT_LAST_POS,
	.rules = optcollector,
	.n_rules = 2
};

static void test_collect(void)
{
	union opt_data results[2];
	int parse_result;
	static const char *argv[] = {"a1", "b2", "c23", "x24"};

	parse_result = optparse_cmd(&cfg_optcollector, results, 0, argv);

	TEST_ASSERT_EQUAL_INT(0, parse_result);
	TEST_ASSERT_EQUAL_STRING("free-me", results[0].d_str);
	TEST_ASSERT_EQUAL_PTR(NULL, results[1].d_str);

	free(results[0].d_str);

	parse_result = optparse_cmd(&cfg_optcollector, results, 1, argv);

	TEST_ASSERT_EQUAL_INT(1, parse_result);
	TEST_ASSERT_EQUAL_STRING("a1", results[0].d_str);
	TEST_ASSERT_EQUAL_PTR(NULL, results[1].d_str);

	optparse_free_strings(&cfg_optcollector, results);
	TEST_ASSERT_EQUAL_PTR(NULL, results[0].d_str);
	TEST_ASSERT_EQUAL_PTR(NULL, results[1].d_str);

	parse_result = optparse_cmd(&cfg_optcollector, results, 2, argv);

	TEST_ASSERT_EQUAL_INT(2, parse_result);
	TEST_ASSERT_EQUAL_STRING("a1", results[0].d_str);
	TEST_ASSERT_EQUAL_STRING("b2", results[1].d_str);

	optparse_free_strings(&cfg_optcollector, results);
	free(results[1].d_str);
	TEST_ASSERT_EQUAL_PTR(NULL, results[0].d_str);

	parse_result = optparse_cmd(&cfg_optcollector, results, 4, argv);

	TEST_ASSERT_EQUAL_INT(4, parse_result);
	TEST_ASSERT_EQUAL_STRING("a1", results[0].d_str);
	TEST_ASSERT_EQUAL_STRING("b2c23x24", results[1].d_str);

	optparse_free_strings(&cfg_optcollector, results);
	free(results[1].d_str);
	TEST_ASSERT_EQUAL_PTR(NULL, results[0].d_str);
}

static const struct opt_rule manyopts[1] = {
	OPTPARSE_P_OPT(COUNT, "countme", NULL, 1000),
};

static const struct opt_conf cfg_many = {
	.helpstr = "test many switches",
	.tune = OPTPARSE_COLLECT_LAST_POS,
	.rules = manyopts,
	.n_rules = 1
};

static void test_toomany(void)
{
	union opt_data results[1];
	int parse_result;
	static const char *argv[256] = {"hello"};

	for (int i=0; i < 256; i++) {
		argv[i] = argv[0];
	}

	parse_result = optparse_cmd(&cfg_many, results, 0, argv);
	TEST_ASSERT_EQUAL_INT(0, parse_result);
	TEST_ASSERT_EQUAL_INT(1000, results[0].d_uint);

	parse_result = optparse_cmd(&cfg_many, results, 1, argv);
	TEST_ASSERT_EQUAL_INT(1, parse_result);
	TEST_ASSERT_EQUAL_INT(1001, results[0].d_uint);

	parse_result = optparse_cmd(&cfg_many, results, 256, argv);
	TEST_ASSERT_EQUAL_INT(256, parse_result);
	TEST_ASSERT_EQUAL_INT(1256, results[0].d_uint);

	parse_result = optparse_cmd(&cfg_many, results, 257, argv);
	TEST_ASSERT_EQUAL_INT(-OPTPARSE_BADSYNTAX, parse_result);
}

int report_position(union opt_key key, const char *value,
		    union opt_data *dest,
		    const char **message)
{
	if (value == NULL) {
		dest->d_int = -1;
	} else {
		dest->d_int = key.argument->position;
	}

	return -OPTPARSE_OK;
}

static const struct opt_rule testpos[] = {
	OPTPARSE_O(COUNT, 'k', NULL, NULL, 9),
	OPTPARSE_O(SET_BOOL, 's', NULL, NULL, 0),
	OPTPARSE_P_OPT(CUSTOM_ACTION, "report_pos1", NULL, report_position),
	OPTPARSE_P_OPT(CUSTOM_ACTION, "report_pos2", NULL, report_position),
	OPTPARSE_P_OPT(CUSTOM_ACTION, "report_pos3", NULL, report_position),
};

static const struct opt_conf cfg_pos = {
	.helpstr = "test how position gets passed to cb",
	.tune = OPTPARSE_COLLECT_LAST_POS,
	.rules = testpos,
	.n_rules = sizeof(testpos)/ sizeof(*testpos)
};

static void test_pos(void)
{
	union opt_data results[sizeof(testpos)/ sizeof(*testpos)];
	int parse_result;
	static const char *argv[257 + 2] = {"k", "-k", "-s", "l", "v"};

	for (int i=5; i < sizeof(argv)/sizeof(*argv); i++) {
		argv[i] = argv[0];
	}

	parse_result = optparse_cmd(&cfg_pos, results, 0, argv);
	TEST_ASSERT_EQUAL_INT(0, parse_result);
	TEST_ASSERT_EQUAL_UINT(9, results[0].d_uint);
	TEST_ASSERT_EQUAL_UINT(0, results[1].d_uint);
	TEST_ASSERT_EQUAL_INT(-1, results[2].d_int);
	TEST_ASSERT_EQUAL_INT(-1, results[3].d_int);
	TEST_ASSERT_EQUAL_INT(-1, results[4].d_int);

	parse_result = optparse_cmd(&cfg_pos, results, 1, argv);
	TEST_ASSERT_EQUAL_INT(1, parse_result);
	TEST_ASSERT_EQUAL_UINT(9, results[0].d_uint);
	TEST_ASSERT_EQUAL_UINT(0, results[1].d_uint);
	TEST_ASSERT_EQUAL_INT(0, results[2].d_int);
	TEST_ASSERT_EQUAL_INT(-1, results[3].d_int);
	TEST_ASSERT_EQUAL_INT(-1, results[4].d_int);

	parse_result = optparse_cmd(&cfg_pos, results, 5, argv);
	TEST_ASSERT_EQUAL_INT(3, parse_result);
	TEST_ASSERT_EQUAL_UINT(10, results[0].d_uint);
	TEST_ASSERT_EQUAL_UINT(1, results[1].d_uint);
	TEST_ASSERT_EQUAL_INT(0, results[2].d_int);
	TEST_ASSERT_EQUAL_INT(1, results[3].d_int);
	TEST_ASSERT_EQUAL_INT(2, results[4].d_int);

	parse_result = optparse_cmd(&cfg_pos, results,
				    sizeof(argv)/sizeof(*argv)-1, argv);
	TEST_ASSERT_EQUAL_INT(256, parse_result);
	TEST_ASSERT_EQUAL_UINT(10, results[0].d_uint);
	TEST_ASSERT_EQUAL_UINT(1, results[1].d_uint);
	TEST_ASSERT_EQUAL_INT(0, results[2].d_int);
	TEST_ASSERT_EQUAL_INT(1, results[3].d_int);
	TEST_ASSERT_EQUAL_INT(255, results[4].d_int);

	parse_result = optparse_cmd(&cfg_pos, results,
				sizeof(argv)/sizeof(*argv), argv);
	TEST_ASSERT_EQUAL_INT(-OPTPARSE_BADSYNTAX, parse_result);
}

int main(void) {
	UNITY_BEGIN();
	RUN_TEST(test_optparse_trivial);
	RUN_TEST(test_optparse_toofew);
	RUN_TEST(test_optparse_basic);
	RUN_TEST(test_optparse_default);
	RUN_TEST(test_optparse_help);
	RUN_TEST(test_optparse_int_err);
	RUN_TEST(test_optparse_uint_err);
	RUN_TEST(test_optparse_float_err);
	RUN_TEST(test_optparse_invalid_positional);
	RUN_TEST(test_optparse_one_arg);
	RUN_TEST(test_init_fail);
	RUN_TEST(test_collect);
	RUN_TEST(test_toomany);
	RUN_TEST(test_pos);
	return UNITY_END();
}
