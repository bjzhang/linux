TEST_GEN_PROGS := tm-resched-dscr tm-syscall tm-signal-msr-resv tm-signal-stack \
	tm-vmxcopy tm-fork tm-tar tm-tmspr tm-exec tm-execed

include ../../lib.mk

$(TEST_GEN_PROGS): ../harness.c ../utils.c

CFLAGS += -mhtm

$(OUTPUT)tm-syscall: tm-syscall-asm.S
$(OUTPUT)tm-syscall: CFLAGS += -I../../../../../usr/include
$(OUTPUT)tm-tmspr: CFLAGS += -pthread

clean:
	$(RM) $(TEST_GEN_PROGS) $(OUTPUT)*.o
