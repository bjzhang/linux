ifneq ($(ARCH),arm64)
TEST_PROGS := seccomp_bpf
endif
CFLAGS += -Wl,-no-as-needed -Wall
LDFLAGS += -lpthread

all: $(TEST_PROGS)

include ../lib.mk

clean:
	$(RM) $(TEST_PROGS)
