# Makefile for building the NIF
#
# Makefile targets:
#
# all/install   build and install the NIF
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

NIF = $(PREFIX)/nerves_key_pkcs11.so

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
	DEFAULT_TARGETS = $(PREFIX)

	# This isn't needed since C compliation on OSX isn't supported, but this is
	# useful for experimentation.
        LDFLAGS += -undefined dynamic_lookup -dynamiclib
        LDFLAGS += $(shell pkg-config --libs libp11)
        CFLAGS += $(shell pkg-config --cflags libp11)
    else
        LDFLAGS += -fPIC -shared
        CFLAGS += -fPIC
    endif
else
# Crosscompiled build
LDFLAGS += -fPIC -shared
CFLAGS += -fPIC
endif
DEFAULT_TARGETS ?= $(PREFIX) $(NIF)

#LDFLAGS += -shared -Wl,-Bsymbolic

#CFLAGS = -Werror=undef -Werror=implicit -Werror=return-type  -Wall -Wstrict-prototypes -Wmissing-prototypes -DUSE_THREADS \
#	 -D_THREAD_SAFE -D_REENTRANT -DPOSIX_THREADS -D_POSIX_THREAD_SAFE_FUNCTIONS -O2 -D_GNU_SOURCE -fPIC

SRC=$(wildcard src/*.c)
HEADERS=$(wildcard src/*.h)
OBJ=$(SRC:src/%.c=$(BUILD)/%.o)

all: install

install: $(BUILD) $(PREFIX) $(DEFAULT_TARGETS)

$(OBJ): $(HEADERS)

$(BUILD)/%.o: src/%.c
	$(CC) -c $(CFLAGS) -o $@ $<

$(NIF): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

$(PREFIX) $(BUILD):
	mkdir -p $@

format:
	astyle -n $(SRC)

clean:
	$(RM) $(PREFIX)/nerves_key_pkcs11.so $(OBJ)

.PHONY: all clean format calling_from_make install
