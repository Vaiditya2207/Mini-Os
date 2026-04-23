#ifndef SCRIPT_H
#define SCRIPT_H

/*
 * script.h — Shell Script Engine Interface
 *
 * Scripts are plain VFS files: one shell command per line.
 * Lines starting with '#' are treated as comments and skipped.
 *
 * Foreground: run_script_fg() executes all lines synchronously,
 *             blocking the shell until the script finishes.
 *
 * Background: run_script_bg() creates a task in the scheduler.
 *             The task executes one line per tick (50ms intervals).
 *             The shell returns immediately; use 'tasks' to monitor.
 *
 * Example script stored in VFS:
 *   # setup.sh
 *   mkdir data
 *   cd data
 *   touch readme.txt
 *   write readme.txt hello world
 *   ls
 */

/* Run script from VFS file in foreground (blocking) */
int script_run_fg(const char *filename);

/* Run script from VFS file in background (non-blocking, returns task id) */
int script_run_bg(const char *filename);

#endif /* SCRIPT_H */
