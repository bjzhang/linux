TEST_GEN_PROGS := tm-resched-dscr tm-syscall tm-signal-msr-resv tm-signal-stack \
	tm-vmxcopy tm-fork tm-tar tm-tmspr tm-exec tm-execed

include ../../lib.mk

$(TEST_GEN_PROGS): ../harness.c ../utils.c

CFLAGS += -mhtm

tm-syscall: tm-syscall-asm.S
tm-syscall: CFLAGS += -I../../../../../usr/include
tm-tmspr: CFLAGS += -pthread

clean:
	$(RM) $(TEST_GEN_PROGS) *.o
