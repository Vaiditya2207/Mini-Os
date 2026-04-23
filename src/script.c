/*
 * script.c — Shell Script Engine
 *
 * Reads a VFS file line by line and dispatches each line through
 * shell_execute() using the str_split tokenizer — exactly the same
 * path as interactive input.
 *
 * Background scripts are registered as scheduler tasks. Each tick
 * advances by one line, so they run at 50ms / line without blocking
 * the interactive shell.
 */

#include "../include/script.h"
#include "../include/shell.h"
#include "../include/task.h"
#include "../include/memory.h"
#include "../include/string.h"
#include "../include/screen.h"
#include "../include/vfs.h"

/* ── Script content cache (used by both fg and bg) ───────────────────── */

#define SCRIPT_MAX_LINES  64
#define SCRIPT_LINE_LEN   CMD_BUF_SIZE   /* reuse shell buffer size */

/* ── Background script state (heap-allocated per task) ───────────────── */

typedef struct {
    char  lines[SCRIPT_MAX_LINES][SCRIPT_LINE_LEN];
    int   total_lines;
    int   current_line;
} ScriptState;

/* ── Helpers ──────────────────────────────────────────────────────────── */

/*
 * Load a VFS file into a ScriptState line table.
 * Splits on '\n', skips blank lines and '#' comments.
 * Returns number of lines loaded, or -1 on error.
 */
static int load_script(const char *filename, ScriptState *ss)
{
    /* vfs_read_raw: we need the raw content, not printed output.
     * We use vfs_get_content() which we expose via a thin helper below.
     * Since vfs.c doesn't expose raw content directly, we read via
     * the internal copy the engine stores. We work around this by
     * using our own VFS helper declared at the bottom. */

    /* Retrieve raw file content from VFS */
    char buf[SCRIPT_MAX_LINES * SCRIPT_LINE_LEN];
    if (vfs_read_to_buf(filename, buf, sizeof(buf)) < 0) {
        return -1;
    }

    ss->total_lines  = 0;
    ss->current_line = 0;

    /* Split on '\n' */
    int i   = 0;
    int col = 0;
    char line[SCRIPT_LINE_LEN];

    while (buf[i] != '\0' && ss->total_lines < SCRIPT_MAX_LINES) {
        if (buf[i] == '\n' || buf[i] == '\r') {
            line[col] = '\0';

            /* Skip blank lines and comments */
            int j = 0;
            while (line[j] == ' ') j++;   /* skip leading spaces */

            if (col > 0 && line[j] != '#') {
                str_copy(ss->lines[ss->total_lines], line, SCRIPT_LINE_LEN);
                ss->total_lines++;
            }

            col = 0;
        } else {
            if (col < SCRIPT_LINE_LEN - 1) {
                line[col++] = buf[i];
            }
        }
        i++;
    }

    /* Last line (no trailing newline) */
    if (col > 0 && ss->total_lines < SCRIPT_MAX_LINES) {
        line[col] = '\0';
        int j = 0;
        while (line[j] == ' ') j++;
        if (line[j] != '#') {
            str_copy(ss->lines[ss->total_lines], line, SCRIPT_LINE_LEN);
            ss->total_lines++;
        }
    }

    return ss->total_lines;
}

/* Execute a single line through the shell */
static void exec_line(char *line)
{
    char buf[SCRIPT_LINE_LEN];
    str_copy(buf, line, SCRIPT_LINE_LEN);

    char *tokens[MAX_TOKENS];
    int count = str_split(buf, ' ', tokens, MAX_TOKENS);
    if (count > 0) {
        shell_execute(tokens, count);
    }
}

/* ── Background tick function ─────────────────────────────────────────── */

static void script_bg_tick(void *state)
{
    ScriptState *ss = (ScriptState *)state;

    if (ss->current_line >= ss->total_lines) {
        /* Script finished — mark done so the scheduler reaps the slot */
        /* Find our own task ID and mark TASK_DONE */
        for (int i = 0; i < MAX_TASKS; i++) {
            Task *t = task_get(i);
            if (t && t->active && t->state == state) {
                t->task_state = TASK_DONE;
                break;
            }
        }
        return;
    }

    /* Execute one line per tick */
    exec_line(ss->lines[ss->current_line]);
    ss->current_line++;
}

/* ── Public API ───────────────────────────────────────────────────────── */

int script_run_fg(const char *filename)
{
    ScriptState ss;

    int n = load_script(filename, &ss);
    if (n < 0) {
        scr_print("  Error: script '");
        scr_print(filename);
        scr_print("' not found or unreadable\n");
        return -1;
    }

    if (n == 0) {
        scr_print("  (script is empty)\n");
        return 0;
    }

    char n_buf[8];
    str_itoa(n, n_buf, 8);
    scr_print("  Running '");
    scr_print(filename);
    scr_print("' (");
    scr_print(n_buf);
    scr_print(" lines, foreground)...\n");

    for (int i = 0; i < ss.total_lines; i++) {
        /* Print the command being run */
        scr_print("  \033[90m> ");
        scr_print(ss.lines[i]);
        scr_print("\033[0m\n");
        exec_line(ss.lines[i]);
    }

    scr_print("  \033[32mScript completed.\033[0m\n");
    return 0;
}

int script_run_bg(const char *filename)
{
    /* Allocate persistent state on the heap — freed when task is reaped */
    ScriptState *ss = (ScriptState *)mem_alloc(sizeof(ScriptState));
    if (!ss) {
        scr_print("  Error: out of memory for script state\n");
        return -1;
    }

    int n = load_script(filename, ss);
    if (n < 0) {
        mem_free(ss);
        scr_print("  Error: script '");
        scr_print(filename);
        scr_print("' not found or unreadable\n");
        return -1;
    }

    if (n == 0) {
        mem_free(ss);
        scr_print("  (script is empty)\n");
        return 0;
    }

    /* Build task name: "script:<filename>" truncated to fit */
    char task_name[MAX_NAME_LEN];
    str_copy(task_name, "script:", MAX_NAME_LEN);
    str_concat(task_name, filename, MAX_NAME_LEN);

    int id = task_add(task_name, script_bg_tick, ss);
    if (id < 0) {
        mem_free(ss);
        return -1;
    }

    char id_buf[8];
    str_itoa(id, id_buf, 8);
    char n_buf[8];
    str_itoa(n, n_buf, 8);

    scr_print("  Started '");
    scr_print(filename);
    scr_print("' in background (task ");
    scr_print(id_buf);
    scr_print(", ");
    scr_print(n_buf);
    scr_print(" lines)\n");

    return id;
}
