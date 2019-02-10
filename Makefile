shared: line-arg.h line-arg.c
	gcc -std=c99 -Wall -Werror -fpic -c line-arg.c
	gcc -shared line-arg.o -o liblnA.so
	rm line-arg.o

static: line-arg.h line-arg.c
	gcc -std=c99 -Wall -Werror -c line-arg.c
	ar rcs liblnA.a line-arg.o
	rm line-arg.o
