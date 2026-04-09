#ifndef SHELL_H
#define SHELL_H

/*
 * shell.h — Command Line Interface
 *
 * REPL loop: keyboard → string → execute → screen
 */

#define MAX_TOKENS    16
#define CMD_BUF_SIZE  256

void shell_init(void);
void shell_run(void);
void shell_execute(char **tokens, int count);
int  shell_is_running(void);

#endif /* SHELL_H */
