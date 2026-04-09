/*
 * main.c — Mini OS Entry Point
 *
 * Bootstraps all subsystems and enters the shell REPL.
 */

#include "../include/memory.h"
#include "../include/keyboard.h"
#include "../include/shell.h"
#include "../include/screen.h"

static char virtual_ram[VIRTUAL_RAM_SIZE];

int main(void)
{
    mem_init(virtual_ram, VIRTUAL_RAM_SIZE);
    kb_init();
    scr_init(80, 24);
    shell_init();
    shell_run();
    kb_restore();
    return 0;
}
