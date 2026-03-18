# Makefile for myshell

CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O2
DEBUGFLAGS = -g -O0 -fsanitize=address,undefined -fno-omit-frame-pointer
TARGET = myshell

SRC = myshell.c
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

debug: CFLAGS += $(DEBUGFLAGS)
debug: clean $(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all debug clean run

