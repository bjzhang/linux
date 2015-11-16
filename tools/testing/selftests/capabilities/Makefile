TEST_FILES := validate_cap
TEST_PROGS := test_execve

CFLAGS += -O2 -g -std=gnu99 -Wall
LDLIBS += -lcap-ng -lrt -ldl

all: $(TEST_PROGS) $(TEST_FILES)

clean:
	$(RM) $(TEST_PROGS) $(TEST_FILES)

include ../lib.mk

