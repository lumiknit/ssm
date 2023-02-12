# ssm Makefile
# Author: lumiknit (aasr4r4@gmail.com)
# Copyright (C) 2023 lumiknit
# License: MIT

EXE_TARGET=ssm
TEST_TARGET=ssm-test

CFLAGS=-Wall -Wextra -O2
LDFLAGS=
INCLUDES=-Iinclude
SRCDIR=src

OBJS=ssm_gc.o ssm_stack.o
EXE_OBJS=ssm.o $(OBJS)
TEST_OBJS=ssm_test.o $(OBJS)
ALL_OBJS=ssm.o ssm_test.o $(OBJS)


.PHONY: all test clean

all: $(EXE_TARGET) $(TEST_TARGET)

$(EXE_TARGET): $(EXE_OBJS)
	$(CC) $(LDFLAGS) -o $@ $<

$(TEST_TARGET): $(TEST_OBJS)
	$(CC) $(LDFLAGS) -o $@ $<

%.o: src/%.c include/*.h
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -f $(EXE_TARGET) $(TEST_TARGET) $(ALL_OBJS)