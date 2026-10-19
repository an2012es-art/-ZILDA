DEVKITPRO ?= /opt/devkitpro
DEVKITARM ?= $(DEVKITPRO)/devkitARM
LIBNDS    ?= $(DEVKITPRO)/libnds
CC        := $(DEVKITARM)/bin/arm-none-eabi-gcc
NDSTOOL   := $(DEVKITPRO)/tools/bin/ndstool

ARCH    := -mthumb -mthumb-interwork -march=armv5te -mtune=arm946e-s
CFLAGS  := -g -Wall -O2 $(ARCH) -I$(LIBNDS)/include -DARM9
LDFLAGS := -specs=ds_arm9.specs -g $(ARCH) -L$(LIBNDS)/lib

all: forest.nds

forest.nds: forest.elf
	$(NDSTOOL) -c $@ -9 $<

forest.elf: main.c
	$(CC) $(CFLAGS) main.c $(LDFLAGS) -lnds9 -o $@

clean:
	rm -f forest.elf forest.nds
