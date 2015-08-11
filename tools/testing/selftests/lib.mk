# This mimics the top-level Makefile. We do it explicitly here so that this
# Makefile can operate with or without the kbuild infrastructure.

# ARCH can be overridden by the user for cross compiling
ARCH ?= $(shell uname -m)
ARCH := $(shell echo $(ARCH) | sed -e s/i.86/x86/ -e s/x86_64/x86/ \
				   -e s/sun4u/sparc64/ \
				   -e s/arm.*/arm/ -e s/sa110/arm/ \
				   -e s/s390x/s390/ -e s/parisc64/parisc/ \
				   -e s/ppc.*/powerpc/ -e s/mips.*/mips/ \
				   -e s/sh[234].*/sh/ -e s/aarch64.*/arm64/ )

CC := $(CROSS_COMPILE)gcc

CFLAGS += $(CFLAGS_EXTRA)

define RUN_TESTS
	@for TEST in $(TEST_PROGS); do \
		(./$$TEST && echo "selftests: $$TEST [PASS]") || echo "selftests: $$TEST [FAIL]"; \
	done;
endef

run_tests: all
	$(RUN_TESTS)

define INSTALL_RULE
	mkdir -p $(INSTALL_PATH)
	@for TEST_DIR in $(TEST_DIRS); do\
		cp -r $$TEST_DIR $(INSTALL_PATH); \
	done;
	install -t $(INSTALL_PATH) $(TEST_PROGS) $(TEST_PROGS_EXTENDED) $(TEST_FILES)
endef

install: all
ifdef INSTALL_PATH
	$(INSTALL_RULE)
else
	$(error Error: set INSTALL_PATH to use install)
endif

define EMIT_TESTS
	@for TEST in $(TEST_PROGS); do \
		echo "(./$$TEST && echo \"selftests: $$TEST [PASS]\") || echo \"selftests: $$TEST [FAIL]\""; \
	done;
endef

emit_tests:
	$(EMIT_TESTS)

.PHONY: run_tests all clean install emit_tests
