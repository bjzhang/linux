ifneq ($(INSTALL_HDR_PATH),)
KERNEL_HEADERS := $(INSTALL_HDR_PATH)/include
else
KERNEL_HEADERS ?= $(abspath ../../usr/include)
endif

CC = $(CROSS_COMPILE)gcc
CFLAGS += -Wall -g -D_GNU_SOURCE -I $(KERNEL_HEADERS)

all: lsgpio

lsgpio: lsgpio.o gpio-utils.o

%.o: %.c gpio-utils.h

.PHONY: clean
clean:
	rm -f *.o lsgpio
