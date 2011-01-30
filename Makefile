# Makefile for pproxy

CFLAGS ?= -O2 -Wall
LDFLAGS ?= -lpthread

TARGET = pproxy
SOURCES = $(wildcard *.c)
OBJS = $(patsubst %.c,%.o,$(SOURCES))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

$(OBJS): %.o: %.c
	$(CC) $(CFLAGS) -c -o $*.o $<

clean:
	$(RM) *.o *~ $(TARGET)

