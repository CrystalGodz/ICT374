# Makefile for tokeniser program

CC = gcc
CFLAGS = -Wall -g

all: tokenizer

tokenizer: main.o token.o
	$(CC) $(CFLAGS) main.o token.o -o tokenizer

main.o: main.c token.h
	$(CC) $(CFLAGS) -c main.c

token.o: token.c token.h
	$(CC) $(CFLAGS) -c token.c

clean:
	rm -f *.o tokenizer
