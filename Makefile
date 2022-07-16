OUT = ssm

CFLAGS = -g
BFLAGS =

RM = rm -rf

# ---- 

OBJS = \
	ssm_main.o \
	ssm_vm.o \
	ssm_mem.o \
	ssm_code.o \
	ssm_alloc.o

.PHONY: all clean

all: $(OUT)

$(OUT): $(OBJS)
	$(CC) $(BFLAGS) $(CFLAGS) -o $(OUT) $(OBJS)

clean:
	$(RM) $(OBJS) $(OUT)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
