VOLCALOAD = volcaload
VOLCAERASE = volcaerase

TARGETS = $(VOLCALOAD) $(VOLCAERASE)

CC     = gcc
CFLAGS = -g -Wall -O3

.PHONY: default all clean

default: $(TARGETS)
all: default

SHAREDC = $(filter-out $(patsubst %, %.c, $(TARGETS)), $(wildcard *.c))
OBJECTS = $(patsubst %.c, %.o, $(SHAREDC))
HEADERS = $(wildcard *.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGETS): % : %.c $(OBJECTS)
	$(CC) $@.c $(OBJECTS) $(CFLAGS) -o $@

.PRECIOUS: $(TARGETS) $(OBJECTS)

clean:
	rm -f *.o
	rm -f $(TARGETS)
