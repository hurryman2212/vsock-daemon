CC=gcc
CXX=g++

DEBUG_FLAGS=-Og -g3 -ggdb
RELEASE_FLAGS=-DNDEBUG -march=native -O2 -flto

COMMON_FLAGS=-mrdrnd -fverbose-asm -save-temps -Wall -Wextra -Werror

CFLAGS=-std=gnu11 $(COMMON_FLAGS)
CXXFLAGS=-std=c++11 $(COMMON_FLAGS)

all: debug

debug: CFLAGS += $(DEBUG_FLAGS)
debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: build

release: CFLAGS += $(RELEASE_FLAGS)
release: CXXFLAGS += $(RELEASE_FLAGS)
release: build

build: vsock-daemon test

vsock-daemon: vsock-daemon.o
	$(CC) -o $@ $(LDFLAGS) $(CFLAGS) $^
test: test.o
	$(CC) -o $@ $(LDFLAGS) $(CFLAGS) $^

vsock-daemon.o: vsock-daemon.c
	$(CC) -c $(CFLAGS) $^
test.o: test.c
	$(CC) -c $(CFLAGS) $^

clean:
	rm -f vsock-daemon test *.i *.o *.s
distclean:
	find . ! \( -name ".git" -prune -o $(shell grep "\!" .gitignore | tr -d "\!" | tr "\n" " " | sed 's/[^[:space:],]\+/"&"/g' | sed 's/[^ ]* */-name &-prune -o /g' | rev | cut -d " " -f3- | rev) \) -type f -exec rm -f {} +
