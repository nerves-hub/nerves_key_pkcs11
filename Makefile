# Makefile for building the shared library
#
# Makefile targets:
#
# all/install   build and install the shared library
# clean         clean build products and intermediates
#
# Variables to override:
#
# MIX_APP_PATH  path to the build directory
#
# CC            C compiler
# CROSSCOMPILE	crosscompiler prefix, if any
# CFLAGS	compiler flags for compiling all C files
# LDFLAGS	linker flags for linking all binaries

ifeq ($(MIX_COMPILE_PATH),)
call_from_make:
	mix compile
endif

PREFIX = $(MIX_APP_PATH)/priv
BUILD  = $(MIX_APP_PATH)/obj

BINARY = $(PREFIX)/nerves_key_pkcs11.so
DEFAULT_TARGETS = $(PREFIX) $(BINARY)

# Check that we're on a supported build platform
ifeq ($(CROSSCOMPILE),)
    # Not crosscompiling, so check that we're on Linux.
    ifneq ($(shell uname -s),Linux)
        $(warning nerves_key_pkcs11 only works on Linux, but crosscompilation)
        $(warning is supported by defining $$CROSSCOMPILE.)
        $(warning See Makefile for details. If using Nerves,)
        $(warning this should be done automatically.)
        $(warning .)
        $(warning Skipping C compilation unless targets explicitly passed to make.)
	DEFAULT_TARGETS =
    endif
endif

LDFLAGS += -shared -Wl,-Bsymbolic

CFLAGS += -Werror=undef -Werror=implicit -Werror=return-type  -Wall -Wstrict-prototypes -Wmissing-prototypes -DUSE_THREADS \
	 -D_THREAD_SAFE -D_REENTRANT -DPOSIX_THREADS -D_POSIX_THREAD_SAFE_FUNCTIONS -O2 -D_GNU_SOURCE -fPIC

SRC=$(wildcard src/*.c)
HEADERS=$(wildcard src/*.h)
OBJ=$(SRC:src/%.c=$(BUILD)/%.o)

calling_from_make:
	mix compile

all: install

install: $(BUILD) $(DEFAULT_TARGETS)

$(OBJ): $(HEADERS) Makefile

$(BUILD)/%.o: src/%.c
	@echo " CC $(notdir $@)"
	$(CC) -c $(CFLAGS) -o $@ $<

$(BINARY): $(OBJ)
	@echo " LD $(notdir $@)"
	$(CC) -o $@ $^ $(LDFLAGS)

$(PREFIX) $(BUILD):
	mkdir -p $@

format:
	astyle -n $(SRC)

clean:
	$(RM) $(BINARY) $(OBJ)

.PHONY: all clean format calling_from_make install

# Don't echo commands unless the caller exports "V=1"
${V}.SILENT:
