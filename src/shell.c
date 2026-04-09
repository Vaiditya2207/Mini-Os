/*
 * shell.c — Shell Skeleton
 *
 * Basic REPL loop. Command dispatch to be filled in next.
 */

#include "../include/shell.h"
#include "../include/keyboard.h"
#include "../include/screen.h"
#include "../include/string.h"

static int shell_running = 1;

static void print_banner(void)
{
    scr_clear();
    scr_print("\033[36m");
    scr_print("  Mini OS v0.1\n");
    scr_print("  Freestanding shell — built with custom C libraries\n");
    scr_print("\033[0m\n");
}

void shell_init(void)
{
    shell_running = 1;
}

int shell_is_running(void)
{
    return shell_running;
}

void shell_execute(char **tokens, int count)
{
    if (count == 0) return;

    if (str_compare(tokens[0], "exit") == 0) {
        shell_running = 0;
    } else {
        scr_print("  Unknown command: '");
        scr_print(tokens[0]);
        scr_print("'\n");
    }
}

void shell_run(void)
{
    char  cmd_buf[CMD_BUF_SIZE];
    char *tokens[MAX_TOKENS];

    print_banner();

    while (shell_running) {
        scr_print("$ ");

        int len = kb_read_line(cmd_buf, CMD_BUF_SIZE);
        if (len == 0) continue;

        /* Naive split on first space — tokenizer not yet available */
        int count = 0;
        tokens[count++] = cmd_buf;
        for (int i = 0; cmd_buf[i] && count < MAX_TOKENS; i++) {
            if (cmd_buf[i] == ' ') {
                cmd_buf[i] = '\0';
                if (cmd_buf[i + 1]) tokens[count++] = &cmd_buf[i + 1];
            }
        }

        shell_execute(tokens, count);
    }
}
