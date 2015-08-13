
uname_M := $(shell uname -m 2>/dev/null || echo not)
ARCH ?= $(shell echo $(uname_M) | sed -e s/aarch64.*/arm64/)

ifneq ($(ARCH),arm64)
TEST_PROGS := seccomp_bpf
endif
CFLAGS += -Wl,-no-as-needed -Wall
LDFLAGS += -lpthread

all: $(TEST_PROGS)

include ../lib.mk

clean:
	$(RM) $(TEST_PROGS)
