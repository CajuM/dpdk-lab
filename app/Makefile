APP = dpdk-lab

PKGCONF ?= pkg-config

SRCS-y := main.c

CFLAGS += -O3 -Wall -Werror
CFLAGS += $(shell $(PKGCONF) --cflags libdpdk)
LDFLAGS = $(shell $(PKGCONF) --libs libdpdk)

.PHONY: all clean
all: $(APP)

$(APP): $(SRCS-y) Makefile
	$(CC) $(CFLAGS) $(SRCS-y) -o $@ $(LDFLAGS)

build:
	@mkdir -p $@

clean:
	rm -f $(APP)
