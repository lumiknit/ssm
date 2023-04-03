# ssm Makefile
# Author: lumiknit (aasr4r4@gmail.com)
# Copyright (C) 2023 lumiknit
# License: MIT

EXE_TARGET=ssm
LIB_TARGET=ssmrt
TEST_TARGET=ssm-test

CFLAGS=-Wall -Wextra -O2
LDFLAGS=
INCLUDES=-Iinclude
SRCDIR=src

RT_OBJS=ssm_gc.o ssm_stack.o
VM_OBJS=ssm_vm.o
EXE_OBJS=ssm.o $(VM_OBJS) $(RT_OBJS)
TEST_OBJS=ssm_test.o $(VM_OBJS) $(RT_OBJS)
ALL_OBJS=ssm.o ssm_test.o $(VM_OBJS) $(RT_OBJS)


.PHONY: all preprocess test clean

# TODO: Add preprocessing (spec.yaml, cgen)

all: preprocess $(EXE_TARGET) $(LIB_TARGET).so $(LIB_TARGET).a $(TEST_TARGET)

preprocess:
	@echo "[INFO] Generate spec.json"
	./opcodes/spec.yaml
	@echo "[INFO] Generate C codes"
	./opcodes/cgen.rb

$(EXE_TARGET): $(EXE_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

$(LIB_TARGET).so: $(RT_OBJS)
	$(CC) $(LDFLAGS) -shared -o $@ $^

$(LIB_TARGET).a: $(RT_OBJS)
	ar rcs $@ $^

$(TEST_TARGET): $(TEST_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: src/%.c include/*.h
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -f $(EXE_TARGET) $(LIB_TARGET).so $(LIB_TARGET).a $(TEST_TARGET) $(ALL_OBJS)