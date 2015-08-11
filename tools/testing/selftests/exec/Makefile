CFLAGS = -Wall
BINARIES = execveat
DEPS = execveat.symlink execveat.denatured script
all: $(BINARIES) $(DEPS)

subdir:
	mkdir -p $@
script:
	echo '#!/bin/sh' > $@
	echo 'exit $$*' >> $@
	chmod +x $@
execveat.symlink: execveat
	ln -s -f $< $@
execveat.denatured: execveat
	cp $< $@
	chmod -x $@
%: %.c
	$(CC) $(CFLAGS) -o $@ $^

TEST_PROGS := execveat

include ../lib.mk

override EMIT_TESTS := echo "mkdir -p subdir; (./execveat && echo \"selftests: execveat [PASS]\") || echo \"selftests: execveat [FAIL]\""

override define INSTALL_RULE
       mkdir -p $(INSTALL_PATH)
       install -t $(INSTALL_PATH) $(TEST_PROGS) $(TEST_PROGS_EXTENDED) $(TEST_FILES)
       cp -a execveat.symlink execveat.denatured $(INSTALL_PATH)
endef

clean:
	rm -rf $(BINARIES) $(DEPS) subdir.moved execveat.moved xxxxx*
