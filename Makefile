# ssm Makefile
# Author: lumiknit (aasr4r4@gmail.com)
# Copyright (C) 2023 lumiknit
# License: MIT

TARGET=ssm
CFLAGS=-Wall -Wextra -O2
LDFLAGS=

OBJS=ssm.o ssm_gc.o ssm_stack.o
INCLUDES=-Iinclude
SRCDIR=src

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: src/%.c include/*.h
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJS)