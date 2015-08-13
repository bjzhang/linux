# Makefile for vm selftests

uname_M := $(shell uname -m 2>/dev/null || echo not)
ARCH ?= $(shell echo $(uname_M) | sed -e s/i.86/i386/ -e s/ppc.*/powerpc/)

ifeq ($(ARCH),powerpc)
support_userfaultfd = yes
endif
ifeq ($(ARCH),x86)
support_userfaultfd = yes
endif

CFLAGS = -Wall -I ../../../../usr/include $(EXTRA_CFLAGS)
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
