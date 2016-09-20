TEST_GEN_PROGS := switch_endian_test

ASFLAGS += -O2 -Wall -g -nostdlib -m64

include ../../lib.mk

$(OUTPUT)switch_endian_test: $(OUTPUT)check-reversed.S

$(OUTPUT)check-reversed.o: $(OUTPUT)check.o
	$(CROSS_COMPILE)objcopy -j .text --reverse-bytes=4 -O binary $< $@

$(OUTPUT)check-reversed.S: $(OUTPUT)check-reversed.o
	hexdump -v -e '/1 ".byte 0x%02X\n"' $< > $@

clean:
	$(RM) $(TEST_GEN_PROGS) $(OUTPUT)*.o $(OUTPUT)check-reversed.S
