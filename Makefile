
LDFLAGS += -shared -Wl,-Bsymbolic

CFLAGS = -Werror=undef -Werror=implicit -Werror=return-type  -Wall -Wstrict-prototypes -Wmissing-prototypes -DUSE_THREADS \
	 -D_THREAD_SAFE -D_REENTRANT -DPOSIX_THREADS -D_POSIX_THREAD_SAFE_FUNCTIONS -O2 -D_GNU_SOURCE -fPIC

all: priv priv/nerves_key_pkcs11.so

SRC=$(wildcard src/*.c)
HEADERS=$(wildcard src/*.h)
OBJ=$(SRC:.c=.o)

$(OBJ): $(HEADERS)

priv:
	mkdir -p priv

priv/nerves_key_pkcs11.so: $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) priv/*.so src/*.o

format:
	astyle -n $(SRC)

.PHONY: all clean format
