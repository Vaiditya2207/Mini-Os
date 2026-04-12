/*
 * shell.c — Interactive Shell (+ VFS commands)
 *
 * Input: keyboard.c  → kb_read_line()
 * Parse: string.c    → str_split()
 * Memory: memory.c   → mem_alloc() / mem_free()
 * VFS: vfs.c         → file system operations
 * Output: screen.c   → scr_print()
 */

#include "../include/shell.h"
#include "../include/keyboard.h"
#include "../include/screen.h"
#include "../include/string.h"
#include "../include/memory.h"
#include "../include/vfs.h"

#include <stdio.h>

static int shell_running = 1;

static void cmd_help(void)
{
    scr_print("\n");
    scr_print("  \033[36m================================\033[0m\n");
    scr_print("  \033[33mMini OS — Command Reference\033[0m\n");
    scr_print("  \033[36m================================\033[0m\n");
    scr_print("  \033[32mhelp\033[0m                 Show this help\n");
    scr_print("  \033[32mecho\033[0m <text>          Print text\n");
    scr_print("  \033[32mclear\033[0m                Clear screen\n");
    scr_print("  \033[32mls\033[0m                   List directory\n");
    scr_print("  \033[32mtouch\033[0m <name>         Create file\n");
    scr_print("  \033[32mmkdir\033[0m <name>         Create directory\n");
    scr_print("  \033[32mcd\033[0m <dir>             Change directory\n");
    scr_print("  \033[32mwrite\033[0m <name> <text>  Write to file\n");
    scr_print("  \033[32mread\033[0m / \033[32mcat\033[0m <name>    Read file\n");
    scr_print("  \033[32mrm\033[0m <name>            Remove entry\n");
    scr_print("  \033[32mmemmap\033[0m               Heap memory map\n");
    scr_print("  \033[32mexit\033[0m                 Shutdown\n\n");
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
    scr_print("  ======================================================\n");
    scr_print("  Mini OS — Freestanding Shell with Virtual Filesystem\n");
    scr_print("  Type 'help' for available commands\n");
    scr_print("  ======================================================\n");
    scr_print("\033[0m\n");
}

void shell_init(void)
{
    vfs_init();
    shell_running = 1;
}

int shell_is_running(void) { return shell_running; }

void shell_execute(char **tokens, int count)
{
    if (count == 0) return;
    const char *cmd = tokens[0];

    if (str_compare(cmd, "help") == 0) {
        cmd_help();
    } else if (str_compare(cmd, "echo") == 0) {
        cmd_echo(tokens, count);
    } else if (str_compare(cmd, "clear") == 0) {
        scr_clear();
    } else if (str_compare(cmd, "ls") == 0) {
        vfs_ls();
    } else if (str_compare(cmd, "touch") == 0) {
        if (count < 2) scr_print("  Usage: touch <filename>\n");
        else if (vfs_touch(tokens[1]) >= 0) {
            scr_print("  Created '"); scr_print(tokens[1]); scr_print("'\n");
        }
    } else if (str_compare(cmd, "mkdir") == 0) {
        if (count < 2) scr_print("  Usage: mkdir <dirname>\n");
        else if (vfs_mkdir(tokens[1]) >= 0) {
            scr_print("  Created directory '"); scr_print(tokens[1]); scr_print("'\n");
        }
    } else if (str_compare(cmd, "cd") == 0) {
        vfs_cd(count >= 2 ? tokens[1] : "/");
    } else if (str_compare(cmd, "write") == 0) {
        if (count < 3) {
            scr_print("  Usage: write <filename> <content>\n");
        } else {
            char content[MAX_DATA_SIZE];
            content[0] = '\0';
            for (int i = 2; i < count; i++) {
                str_concat(content, tokens[i], MAX_DATA_SIZE);
                if (i < count - 1) str_concat(content, " ", MAX_DATA_SIZE);
            }
            vfs_write(tokens[1], content);
        }
    } else if (str_compare(cmd, "read") == 0 || str_compare(cmd, "cat") == 0) {
        if (count < 2) scr_print("  Usage: read <filename>\n");
        else vfs_read(tokens[1]);
    } else if (str_compare(cmd, "rm") == 0) {
        if (count < 2) scr_print("  Usage: rm <name>\n");
        else vfs_rm(tokens[1]);
    } else if (str_compare(cmd, "memmap") == 0) {
        mem_dump();
    } else if (str_compare(cmd, "exit") == 0) {
        shell_running = 0;
    } else {
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
        char path[128];
        vfs_get_current_path(path, 128);
        scr_print("\033[32mmini-os\033[0m:\033[34m");
        scr_print(path);
        scr_print("\033[0m$ ");
        fflush(stdout);

        int len = kb_read_line(cmd_buf, CMD_BUF_SIZE);
        if (len == 0) continue;

        int count = str_split(cmd_buf, ' ', tokens, MAX_TOKENS);
        if (count == 0) continue;

        shell_execute(tokens, count);
    }
}
