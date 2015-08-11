# Makefile for vm selftests

ifeq ($(ARCH),powerpc)
support_userfaultfd = yes
endif
ifeq ($(ARCH),x86)
support_userfaultfd = yes
endif

CFLAGS = -Wall
BINARIES = compaction_test
BINARIES += hugepage-mmap
BINARIES += hugepage-shm
BINARIES += map_hugetlb
BINARIES += mlock2-tests
BINARIES += on-fault-limit
BINARIES += thuge-gen
BINARIES += transhuge-stress
ifdef support_userfaultfd
BINARIES += userfaultfd
endif

all: $(BINARIES)
%: %.c
	$(CC) $(CFLAGS) -o $@ $^ -lrt
userfaultfd: userfaultfd.c
	$(CC) $(CFLAGS) -O2 -o $@ $^ -lpthread

TEST_PROGS := run_vmtests
TEST_FILES := $(BINARIES)

include ../lib.mk

clean:
	$(RM) $(BINARIES)
