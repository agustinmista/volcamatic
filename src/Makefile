LOADER    = volcaload
ERASER    = volcaerase
CONVERTER = volcaconvert
FLASHER   = volcaflash

TARGETS = $(LOADER) $(ERASER)
ALL     = $(TARGETS) $(CONVERTER) $(FLASHER)

DESTDIR = $(HOME)/.local/bin

CC     = gcc
CFLAGS = -g -Wall -O3

KORG_SDK = korg

.PHONY: default all install clean

default: $(TARGETS)
all: default

SHAREDC = $(filter-out $(patsubst %, %.c, $(TARGETS)), \
	                     $(wildcard *.c)                 \
											 $(wildcard $(KORG_SDK)/*.c))
OBJECTS = $(patsubst %.c, %.o, $(SHAREDC))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGETS): % : %.c $(OBJECTS)
	$(CC) $@.c $(OBJECTS) $(CFLAGS) -o $@

.PRECIOUS: $(TARGETS) $(OBJECTS)

install: $(TARGETS)
	mkdir -p $(DESTDIR)
	cp $(ALL) $(DESTDIR)

clean:
	rm -f *.o
	rm -f $(TARGETS)
