
TEST_PROGS := gpio-mockup
OBJS := gpio-sysfs.o gpio-chardev.o

$(TEST_PROGS): $(OBJS)

all: $(TEST_PROGS)

include ../lib.mk

clean:
	rm $(TEST_PROGS) $(OBJS)
