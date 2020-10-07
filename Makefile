CFLAGS=-std=c11 -g -static
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

tsugucc: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(OBJS): tsugucc.h

test: tsugucc
	./test.sh

clean:
	rm -f tsugucc *.o *~ tmp*

.PHONY: test clean