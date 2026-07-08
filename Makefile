CC = gcc

CFLAGS = -Wall -Wextra -std=c11 -Iinclude

SRC = src/arena.c
EXAMPLE = examples/main.c

all:
	$(CC) $(CFLAGS) $(SRC) $(EXAMPLE) -o arena

clean:
	rm -f arena