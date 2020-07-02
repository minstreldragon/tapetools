CC=gcc
RM=rm -f

SRCS=tapext.c
OBJS=$(subst .c,.o,$(SRCS))

all: tapext

tapext: $(OBJS)
	$(CC) -o tapext $(OBJS)

tapext.o: tapext.c

clean:
	$(RM) $(OBJS) tapext
