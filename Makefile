# keep-it-simple makefile

# tiny hack to make shell autocompletion work
PATHSEP := /

# windows hacks
ifeq ($(strip $(OS)), Windows_NT)
  MKDIR         ?= md
  RM		?= del
  RMDIR		?= rmdir /s /q
  PATHSEP := \\
else
  MKDIR         ?= mkdir -p
  RM		?= rm -f
  RMDIR		?= rm -rf
endif


OUT_DIR ?= build
OUT_DIR_ = $(OUT_DIR)$(PATHSEP)
SRC ?= src
TESTS ?= tests
SRC_ = $(SRC)$(PATHSEP)
TESTS_ ?= $(TESTS)$(PATHSEP)
INCLUDE ?= include
LIBNAME ?= liboptparse
OUT_FILE_ = $(OUT_DIR_)$(LIBNAME)
VERSION ?= 0.3
SOVERSION ?= 0
SONAME ?= $(LIBNAME).so.$(SOVERSION)
PREFIX ?= /usr/local

OUT_DIR_DYN_ = $(OUT_DIR_)dynamic$(PATHSEP)
OUT_DIR_STATIC_ = $(OUT_DIR_)static$(PATHSEP)

OUT_FILE_DYN = $(OUT_FILE_).so.$(VERSION)
OUT_FILE_STATIC = $(OUT_FILE_).a

DBGFLAGS ?=
OPTFLAGS ?= -Os
INCLUDES ?=
CWARNS ?= -pedantic -Wall -Wextra -Wconversion -Wnull-dereference
CFLAGS ?= -std=c99 $(DBGFLAGS) $(CWARNS) $(OPTFLAGS) $(INCLUDES)
SOOPTS ?= -Wl,-soname,$(SONAME)

ARFLAGS = rcs

SOURCES = $(SRC_)optparse.c $(SRC_)optparse.h


.PHONY: all
all: $(OUT_FILE_DYN) $(OUT_FILE_STATIC)

$(OUT_DIR_STATIC_)optparse.o: $(SOURCES) | $(OUT_DIR_STATIC_)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT_DIR_DYN_)optparse.o: $(SOURCES) | $(OUT_DIR_DYN_)
	$(CC) $(CFLAGS) -fPIC -c $< -o $@


$(OUT_DIR) $(OUT_DIR_DYN_) $(OUT_DIR_STATIC_):
	$(MKDIR) $@

# create a static library file from object files
$(OUT_FILE_STATIC): $(OUT_DIR_STATIC_)optparse.o | $(OUT_DIR)
	$(info "Creating static archive")
	$(AR) $(ARFLAGS) $@ $^

# create a dynamic library file from object files
$(OUT_FILE_DYN): $(OUT_DIR_DYN_)optparse.o | $(OUT_DIR)
	$(CC) $(CFLAGS) -shared $^ $(SOOPTS) -o $@


# Test handling. Unfortunately -fprofile-note is not yet in gcc stable so
# files end up everywhere.

TEST_PROG = $(OUT_DIR_)test1
EXAMPLE_PROG = $(OUT_DIR_)readme-example

$(TEST_PROG): OPTFLAGS = -O0
$(TEST_PROG): DBGFLAGS = --coverage -g3 -dA
$(TEST_PROG): INCLUDES = -I$(TESTS_)unity -I$(SRC)

$(TEST_PROG): $(TESTS_)test1.c $(SOURCES)  $(TESTS_)unity$(PATHSEP)unity.c | $(OUT_DIR)
	$(CC) $(CFLAGS) $(filter %.c,$^) -o $@

optparse.gcda: $(TEST_PROG)
	$<

optparse.c.gcov: optparse.gcda
	gcov -a -k -f optparse


$(EXAMPLE_PROG): INCLUDES = -I$(SRC)
# use the example to see if the static lib works
$(EXAMPLE_PROG): $(TESTS_)readme-example.c $(OUT_FILE_STATIC) | $(OUT_DIR)
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: test example-test

example-test: $(EXAMPLE_PROG)
	$< -vvsv --cool 90 -- -whatever
	$< x

test: optparse.c.gcov example-test

.PHONY: clean
clean:
	$(RMDIR) $(OUT_DIR)
	$(RM) $(wildcard *.gcda) $(wildcard *.gcno) $(wildcard *.gcov)

# The install recipe has no dependencies on purpose: if one invokes
# make install as root he does not want the build to be run as root too
.PHONY: install
install:
	install -d $(PREFIX)/lib
	install -m 644 $(OUT_FILE_STATIC) $(PREFIX)/lib
	install -s $(OUT_FILE_DYN) $(PREFIX)/lib
	rm -rf $(PREFIX)/lib/$(LIBNAME).so
	ln -s $(OUT_FILE_DYN:$(OUT_DIR_)%=%) $(PREFIX)/lib/$(LIBNAME).so
	ldconfig -v -n $(PREFIX)/lib
