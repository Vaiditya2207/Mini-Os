/*
 * shell.c — Interactive Shell Implementation
 *
 * Clean shell abstraction with proper library integration:
 *   - Input:   keyboard.c   → kb_read_line()
 *   - Parsing: string.c     → str_split() custom tokenizer
 *   - Memory:  memory.c     → mem_alloc() / mem_free()
 *   - Output:  screen.c     → scr_print() / scr_clear()
 *   - Math:    math.c       → used in calculator command
 *
 * No standard library string functions (strcmp, strlen, etc.) are used.
 * All command dispatch is token-based via the custom str_split() tokenizer.
 */

#include "../include/shell.h"
#include "../include/memory.h"
#include "../include/math.h"
#include "../include/string.h"
#include "../include/screen.h"
#include "../include/keyboard.h"
#include "../include/vfs.h"
#include "../include/task.h"

#include <stdio.h>   /* printf for formatted output only */

/* Counter task state */
typedef struct {
    int count;
} CounterState;

/* ── Shell State ──────────────────────────────────────────────────────── */

static int shell_running = 1;

/* ── Background Counter Task ─────────────────────────────────────────── */

static void counter_tick(void *state)
{
    CounterState *cs = (CounterState *)state;
    cs->count++;
}

/* ══════════════════════════════════════════════════════════════════════ */
/*                     SHELL COMMANDS                                    */
/* ══════════════════════════════════════════════════════════════════════ */

static void cmd_help(void)
{
    scr_print("\n");
    scr_print("  \033[36m╔══════════════════════════════════════════════════╗\033[0m\n");
    scr_print("  \033[36m║\033[0m         \033[33mMini OS — Command Reference\033[0m            \033[36m║\033[0m\n");
    scr_print("  \033[36m╠══════════════════════════════════════════════════╣\033[0m\n");
    scr_print("  \033[36m║\033[0m  \033[32mhelp\033[0m                 Show this help message    \033[36m║\033[0m\n");
    scr_print("  \033[36m║\033[0m  \033[32mecho\033[0m <text>          Print text to console     \033[36m║\033[0m\n");
    scr_print("  \033[36m║\033[0m  \033[32mclear\033[0m                Clear the screen          \033[36m║\033[0m\n");
    scr_print("  \033[36m║\033[0m  \033[32mls\033[0m                   List files in directory   \033[36m║\033[0m\n");
    scr_print("  \033[36m║\033[0m  \033[32mtouch\033[0m <name>         Create empty file         \033[36m║\033[0m\n");
    scr_print("  \033[36m║\033[0m  \033[32mmkdir\033[0m <name>         Create directory          \033[36m║\033[0m\n");
    scr_print("  \033[36m║\033[0m  \033[32mcd\033[0m <dir>             Change directory          \033[36m║\033[0m\n");
    scr_print("  \033[36m║\033[0m  \033[32mwrite\033[0m <name> <text>  Write content to file     \033[36m║\033[0m\n");
    scr_print("  \033[36m║\033[0m  \033[32mread\033[0m / \033[32mcat\033[0m <name>    Display file contents     \033[36m║\033[0m\n");
    scr_print("  \033[36m║\033[0m  \033[32mrm\033[0m <name>            Remove file or directory  \033[36m║\033[0m\n");
    scr_print("  \033[36m║\033[0m  \033[32mmemmap\033[0m               Show heap memory map      \033[36m║\033[0m\n");
    scr_print("  \033[36m║\033[0m  \033[32mtasks\033[0m                List background tasks     \033[36m║\033[0m\n");
    scr_print("  \033[36m║\033[0m  \033[32mkill\033[0m <id>            Kill a background task    \033[36m║\033[0m\n");
    scr_print("  \033[36m║\033[0m  \033[32mstartcounter\033[0m         Start counter task        \033[36m║\033[0m\n");
    scr_print("  \033[36m║\033[0m  \033[32msysinfo\033[0m              System information        \033[36m║\033[0m\n");
    scr_print("  \033[36m║\033[0m  \033[32mexit\033[0m                 Shutdown Mini OS           \033[36m║\033[0m\n");
    scr_print("  \033[36m╚══════════════════════════════════════════════════╝\033[0m\n");
    scr_print("\n");
}

static void cmd_echo(char **tokens, int count)
{
    for (int i = 1; i < count; i++) {
        scr_print(tokens[i]);
        if (i < count - 1) scr_print(" ");
    }
    scr_print("\n");
}

static void cmd_sysinfo(void)
{
    char buf[16];

    scr_print("\n");
    scr_print("  \033[33m╔═══════════════════════════════════╗\033[0m\n");
    scr_print("  \033[33m║       System Information          ║\033[0m\n");
    scr_print("  \033[33m╠═══════════════════════════════════╣\033[0m\n");

    str_itoa((int)VIRTUAL_RAM_SIZE, buf, 16);
    scr_print("  \033[33m║\033[0m  Virtual RAM:  ");
    scr_print(buf);
    scr_print(" bytes     \033[33m║\033[0m\n");

    str_itoa((int)mem_available(), buf, 16);
    scr_print("  \033[33m║\033[0m  Free Memory: ");
    scr_print(buf);
    scr_print(" bytes     \033[33m║\033[0m\n");

    str_itoa(mem_block_count(), buf, 16);
    scr_print("  \033[33m║\033[0m  Heap Blocks: ");
    scr_print(buf);
    scr_print("           \033[33m║\033[0m\n");

    str_itoa(vfs_get_total_files(), buf, 16);
    scr_print("  \033[33m║\033[0m  Files:       ");
    scr_print(buf);
    scr_print("           \033[33m║\033[0m\n");

    str_itoa(vfs_get_total_dirs(), buf, 16);
    scr_print("  \033[33m║\033[0m  Directories: ");
    scr_print(buf);
    scr_print("           \033[33m║\033[0m\n");

    str_itoa(MAX_INODES, buf, 16);
    scr_print("  \033[33m║\033[0m  Max Inodes:  ");
    scr_print(buf);
    scr_print("           \033[33m║\033[0m\n");

    scr_print("  \033[33m╚═══════════════════════════════════╝\033[0m\n\n");
}

/* ══════════════════════════════════════════════════════════════════════ */
/*                     COMMAND EXECUTION                                 */
/* ══════════════════════════════════════════════════════════════════════ */

void shell_execute(char **tokens, int count)
{
    if (count == 0) return;

    const char *cmd = tokens[0];

    if (str_compare(cmd, "help") == 0) {
        cmd_help();
    }
    else if (str_compare(cmd, "echo") == 0) {
        cmd_echo(tokens, count);
    }
    else if (str_compare(cmd, "clear") == 0) {
        scr_clear();
    }
    else if (str_compare(cmd, "ls") == 0) {
        vfs_ls();
    }
    else if (str_compare(cmd, "touch") == 0) {
        if (count < 2) {
            scr_print("  Usage: touch <filename>\n");
        } else {
            if (vfs_touch(tokens[1]) >= 0) {
                scr_print("  Created '");
                scr_print(tokens[1]);
                scr_print("'\n");
            }
        }
    }
    else if (str_compare(cmd, "mkdir") == 0) {
        if (count < 2) {
            scr_print("  Usage: mkdir <dirname>\n");
        } else {
            if (vfs_mkdir(tokens[1]) >= 0) {
                scr_print("  Created directory '");
                scr_print(tokens[1]);
                scr_print("'\n");
            }
        }
    }
    else if (str_compare(cmd, "cd") == 0) {
        vfs_cd(count >= 2 ? tokens[1] : "/");
    }
    else if (str_compare(cmd, "write") == 0) {
        if (count < 3) {
            scr_print("  Usage: write <filename> <content...>\n");
        } else {
            char content[MAX_DATA_SIZE];
            content[0] = '\0';
            for (int i = 2; i < count; i++) {
                str_concat(content, tokens[i], MAX_DATA_SIZE);
                if (i < count - 1) str_concat(content, " ", MAX_DATA_SIZE);
            }
            if (vfs_write(tokens[1], content) == 0) {
                char len_buf[16];
                str_itoa(str_length(content), len_buf, 16);
                scr_print("  Written to '");
                scr_print(tokens[1]);
                scr_print("' (");
                scr_print(len_buf);
                scr_print(" bytes)\n");
            }
        }
    }
    else if (str_compare(cmd, "read") == 0 || str_compare(cmd, "cat") == 0) {
        if (count < 2) {
            scr_print("  Usage: ");
            scr_print(cmd);
            scr_print(" <filename>\n");
        } else {
            vfs_read(tokens[1]);
        }
    }
    else if (str_compare(cmd, "rm") == 0) {
        if (count < 2) {
            scr_print("  Usage: rm <name>\n");
        } else {
            vfs_rm(tokens[1]);
        }
    }
    else if (str_compare(cmd, "memmap") == 0) {
        mem_dump();
    }
    else if (str_compare(cmd, "sysinfo") == 0) {
        cmd_sysinfo();
    }
    else if (str_compare(cmd, "tasks") == 0) {
        task_list();
    }
    else if (str_compare(cmd, "kill") == 0) {
        if (count < 2) {
            scr_print("  Usage: kill <task_id>\n");
        } else {
            task_kill(str_atoi(tokens[1]));
        }
    }
    else if (str_compare(cmd, "startcounter") == 0) {
        CounterState *cs = (CounterState *)mem_alloc(sizeof(CounterState));
        if (cs) {
            cs->count = 0;
            int id = task_add("counter", counter_tick, cs);
            if (id >= 0) {
                char id_buf[8];
                str_itoa(id, id_buf, 8);
                scr_print("  Started counter task (ID: ");
                scr_print(id_buf);
                scr_print(")\n");
            }
        } else {
            scr_print("  Error: out of memory\n");
        }
    }
    else if (str_compare(cmd, "exit") == 0) {
        shell_running = 0;
    }
    else {
        scr_print("  Unknown command: '");
        scr_print(cmd);
        scr_print("'. Type 'help' for commands.\n");
    }
}

/* ══════════════════════════════════════════════════════════════════════ */
/*                     BOOT BANNER                                       */
/* ══════════════════════════════════════════════════════════════════════ */

static void print_banner(void)
{
    scr_clear();

    scr_print("\033[36m");
    scr_print("  ╔══════════════════════════════════════════════════════╗\n");
    scr_print("  ║                                                      ║\n");
    scr_print("  ║   ███╗   ███╗██╗███╗   ██╗██╗     ██████╗ ███████╗   ║\n");
    scr_print("  ║   ████╗ ████║██║████╗  ██║██║    ██╔═══██╗██╔════╝   ║\n");
    scr_print("  ║   ██╔████╔██║██║██╔██╗ ██║██║    ██║   ██║███████╗   ║\n");
    scr_print("  ║   ██║╚██╔╝██║██║██║╚██╗██║██║    ██║   ██║╚════██║   ║\n");
    scr_print("  ║   ██║ ╚═╝ ██║██║██║ ╚████║██║    ╚██████╔╝███████║   ║\n");
    scr_print("  ║   ╚═╝     ╚═╝╚═╝╚═╝  ╚═══╝╚═╝     ╚═════╝ ╚══════╝   ║\n");
    scr_print("  ║                                                      ║\n");
    scr_print("  ║   Freestanding Mini Operating System v1.0            ║\n");
    scr_print("  ║   Built with custom C libraries — no libc            ║\n");
    scr_print("  ║                                                      ║\n");
    scr_print("  ║   Type 'help' for available commands                 ║\n");
    scr_print("  ║                                                      ║\n");
    scr_print("  ╚══════════════════════════════════════════════════════╝\n");
    scr_print("\033[0m\n");
}

/* ══════════════════════════════════════════════════════════════════════ */
/*                     SHELL PUBLIC API                                  */
/* ══════════════════════════════════════════════════════════════════════ */

void shell_init(void)
{
    vfs_init();
    task_init();
    shell_running = 1;
}

int shell_is_running(void)
{
    return shell_running;
}

void shell_run(void)
{
    char cmd_buf[CMD_BUF_SIZE];
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

        task_tick_all();
    }
}
