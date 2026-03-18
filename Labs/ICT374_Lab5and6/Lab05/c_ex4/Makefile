# makefile for teststack
# filename: Makefile

testStack: testStack.o stack.o
	gcc testStack.o stack.o -o testStack

testStack.o: testStack.c stack.h
	gcc -c testStack.c

stack.o: stack.c stack.h
	gcc -c stack.c

clean:
	rm *.o
