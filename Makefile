OUT = ssm

CFLAGS = -g
BFLAGS =

OBJS = \
	ssm_main.o \
	ssm_vm.o \
	ssm_mem.o \
	ssm_code.o \
	ssm_alloc.o

$(OUT): $(OBJS)
	$(CC) $(BFLAGS) $(CFLAGS) -o $(OUT) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
