# ══════════════════════════════════════════════════════════════════════
#  Mini OS — Makefile
#  Freestanding Systems Programming in C
# ══════════════════════════════════════════════════════════════════════

CC     = clang
CFLAGS = -Wall -Wextra -Werror -std=c99

SRC    = src/main.c src/shell.c src/memory.c src/math.c \
         src/string.c src/screen.c src/keyboard.c \
         src/vfs.c src/task.c
LIB    = src/memory.c src/math.c src/string.c src/screen.c \
         src/keyboard.c src/vfs.c src/task.c
TARGET = mini_os

.PHONY: all test clean

all:
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# Tests
test: test_memory test_math test_string
	@echo ""
	@echo "Running all tests..."
	@echo ""
	@./test_memory
	@echo ""
	@./test_math
	@echo ""
	@./test_string
	@echo ""
	@echo "All tests completed."

test_memory: tests/test_memory.c src/memory.c
	$(CC) $(CFLAGS) -o $@ $^

test_math: tests/test_math.c src/math.c
	$(CC) $(CFLAGS) -o $@ $^

test_string: tests/test_string.c src/string.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGET) test_memory test_math test_string test_shell
