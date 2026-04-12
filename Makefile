# Mini OS — Makefile
CC     = clang
CFLAGS = -Wall -Wextra -Werror -std=c99

SRC    = src/main.c src/shell.c src/memory.c src/math.c \
         src/string.c src/screen.c src/keyboard.c src/vfs.c
TARGET = mini_os

.PHONY: all clean

all:
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
