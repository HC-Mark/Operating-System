SHELL:=/bin/bash -O extglob
mysh: cir_buf.h mysh.c
	gcc -g -o mysh mysh.c cir_buf.c

matrix: matrix.c
	gcc -g -pthread -o matrix matrix.c

clean:
	rm -f !(*.h|*.c|Makefile|README)
