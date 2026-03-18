# Makefile for mini shell with tokenizer and command splitter

CC = gcc
CFLAGS = -Wall -g

# default target
all: minishell

# link objects into final program
minishell: main.o token.o command.o
	$(CC) $(CFLAGS) main.o token.o command.o -o minishell

# compile object files
main.o: main.c token.h command.h
	$(CC) $(CFLAGS) -c main.c

token.o: token.c token.h
	$(CC) $(CFLAGS) -c token.c

command.o: command.c command.h token.h
	$(CC) $(CFLAGS) -c command.c

# clean up build artifacts
clean:
	rm -f *.o minishell
