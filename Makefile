CFLAGS=-std=c11 -g -static

tsugucc: tsugucc.c

test: tsugucc
	./test.sh

clean:
	rm -f tsugucc *.o *~ tmp*

.PHONY: test clean