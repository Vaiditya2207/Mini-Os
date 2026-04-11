/*
 * shell.c — Interactive Shell (+ memmap)
 *
 * Input: keyboard.c  → kb_read_line()
 * Parse: string.c    → str_split()
 * Memory: memory.c   → mem_alloc() / mem_free() / mem_dump()
 * Output: screen.c   → scr_print()
 */

#include "../include/shell.h"
#include "../include/keyboard.h"
#include "../include/screen.h"
#include "../include/string.h"
#include "../include/memory.h"

#include <stdio.h>

static int shell_running = 1;

static void cmd_help(void)
{
    scr_print("\n");
    scr_print("  Available commands:\n");
    scr_print("    help          Show this help message\n");
    scr_print("    echo <text>   Print text to console\n");
    scr_print("    clear         Clear the screen\n");
    scr_print("    memmap        Show heap memory map\n");
    scr_print("    exit          Shutdown Mini OS\n\n");
}

static void cmd_echo(char **tokens, int count)
{
    for (int i = 1; i < count; i++) {
        scr_print(tokens[i]);
        if (i < count - 1) scr_print(" ");
    }
    scr_print("\n");
}

static void print_banner(void)
{
    scr_clear();
    scr_print("\033[36m");
    scr_print("  ==============================\n");
    scr_print("  Mini OS — Freestanding Shell\n");
    scr_print("  Type 'help' for commands\n");
    scr_print("  ==============================\n");
    scr_print("\033[0m\n");
}

void shell_init(void) { shell_running = 1; }
int  shell_is_running(void) { return shell_running; }

void shell_execute(char **tokens, int count)
{
    if (count == 0) return;
    const char *cmd = tokens[0];

    if      (str_compare(cmd, "help")   == 0) cmd_help();
    else if (str_compare(cmd, "echo")   == 0) cmd_echo(tokens, count);
    else if (str_compare(cmd, "clear")  == 0) scr_clear();
    else if (str_compare(cmd, "memmap") == 0) mem_dump();
    else if (str_compare(cmd, "exit")   == 0) shell_running = 0;
    else {
        scr_print("  Unknown command: '");
        scr_print(cmd);
        scr_print("'. Type 'help' for commands.\n");
    }
}

void shell_run(void)
{
    char  cmd_buf[CMD_BUF_SIZE];
    char *tokens[MAX_TOKENS];

    print_banner();

    while (shell_running) {
        scr_print("\033[32mmini-os\033[0m$ ");
        fflush(stdout);

        int len = kb_read_line(cmd_buf, CMD_BUF_SIZE);
        if (len == 0) continue;

        int count = str_split(cmd_buf, ' ', tokens, MAX_TOKENS);
        if (count == 0) continue;

        shell_execute(tokens, count);
    }
}
