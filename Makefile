# Mini OS — Makefile
CC     = clang
CFLAGS = -Wall -Wextra -Werror -std=c99
TARGET = mini_os

.PHONY: all clean

all:
	$(CC) $(CFLAGS) -o $(TARGET) src/main.c

clean:
	rm -f $(TARGET)
