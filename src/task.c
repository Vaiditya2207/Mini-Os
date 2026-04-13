#include "../include/task.h"
#include "../include/memory.h"
#include "../include/string.h"
#include "../include/screen.h"

#define MAX_NAME_LEN 32

typedef struct {
    void (*tick)(void *state);
    void  *state;
    int    active;
    char   name[MAX_NAME_LEN];
} Task;

static Task task_table[MAX_TASKS];

void task_init(void)
{
    for (int i = 0; i < MAX_TASKS; i++) {
        task_table[i].active = 0;
        task_table[i].tick   = NULL;
        task_table[i].state  = NULL;
    }
}

int task_add(const char *name, void (*tick_fn)(void *), void *state)
{
    for (int i = 0; i < MAX_TASKS; i++) {
        if (!task_table[i].active) {
            str_copy(task_table[i].name, name, MAX_NAME_LEN);
            task_table[i].tick   = tick_fn;
            task_table[i].state  = state;
            task_table[i].active = 1;
            return i;
        }
    }
    scr_print("  Error: task table full\n");
    return -1;
}

void task_tick_all(void)
{
    for (int i = 0; i < MAX_TASKS; i++) {
        if (task_table[i].active && task_table[i].tick) {
            task_table[i].tick(task_table[i].state);
        }
    }
}

void task_list(void)
{
    int count = 0;
    scr_print("\n  Active Background Tasks:\n");
    scr_print("  ID   Name             Status\n");
    scr_print("  ──── ──────────────── ────────\n");

    for (int i = 0; i < MAX_TASKS; i++) {
        if (task_table[i].active) {
            char id_buf[8];
            str_itoa(i, id_buf, 8);
            scr_print("  ");
            scr_print(id_buf);
            scr_print("    ");
            scr_print(task_table[i].name);
            scr_print("           running\n");
            count++;
        }
    }

    if (count == 0) {
        scr_print("  (no active tasks)\n");
    }
    scr_print("\n");
}

void task_kill(int id)
{
    if (id < 0 || id >= MAX_TASKS || !task_table[id].active) {
        scr_print("  Error: invalid task ID\n");
        return;
    }

    /* Free state via memory.c */
    if (task_table[id].state) {
        mem_free(task_table[id].state);
    }

    scr_print("  Killed task '");
    scr_print(task_table[id].name);
    scr_print("'\n");
    task_table[id].active = 0;
    task_table[id].tick   = NULL;
    task_table[id].state  = NULL;
}
