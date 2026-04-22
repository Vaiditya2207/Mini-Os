/*
 * task.c — Cooperative + Preemptive Task Scheduler
 *
 * task_tick_all() is called by sched_dispatch() on every SIGALRM tick.
 * Tasks with state TASK_READY get their tick_fn called.
 * Tasks with state TASK_SLEEPING decrement sleep_ticks first.
 * Tasks with state TASK_DONE are skipped and their slot is reusable.
 */

#include "../include/task.h"
#include "../include/memory.h"
#include "../include/string.h"
#include "../include/screen.h"

static Task task_table[MAX_TASKS];

void task_init(void)
{
    for (int i = 0; i < MAX_TASKS; i++) {
        task_table[i].active      = 0;
        task_table[i].tick        = NULL;
        task_table[i].state       = NULL;
        task_table[i].task_state  = TASK_DONE;
        task_table[i].sleep_ticks = 0;
        task_table[i].name[0]     = '\0';
    }
}

int task_add(const char *name, void (*tick_fn)(void *), void *state)
{
    for (int i = 0; i < MAX_TASKS; i++) {
        if (!task_table[i].active) {
            str_copy(task_table[i].name, name, MAX_NAME_LEN);
            task_table[i].tick        = tick_fn;
            task_table[i].state       = state;
            task_table[i].active      = 1;
            task_table[i].task_state  = TASK_READY;
            task_table[i].sleep_ticks = 0;
            return i;
        }
    }
    scr_print("  Error: task table full\n");
    return -1;
}

void task_tick_all(void)
{
    for (int i = 0; i < MAX_TASKS; i++) {
        if (!task_table[i].active) continue;

        switch (task_table[i].task_state) {
            case TASK_SLEEPING:
                if (task_table[i].sleep_ticks > 0) {
                    task_table[i].sleep_ticks--;
                } else {
                    task_table[i].task_state = TASK_READY;
                }
                break;

            case TASK_READY:
                if (task_table[i].tick) {
                    task_table[i].tick(task_table[i].state);
                }
                break;

            case TASK_DONE:
                /* Reap the slot */
                task_table[i].active = 0;
                break;
        }
    }
}

void task_list(void)
{
    int count = 0;

    scr_print("\n  Active Background Tasks:\n");
    scr_print("  ID   Name             State\n");
    scr_print("  ──── ──────────────── ────────\n");

    for (int i = 0; i < MAX_TASKS; i++) {
        if (task_table[i].active) {
            char id_buf[8];
            str_itoa(i, id_buf, 8);
            scr_print("  ");
            scr_print(id_buf);
            scr_print("    ");
            scr_print(task_table[i].name);

            /* Pad to column */
            int pad = 16 - str_length(task_table[i].name);
            for (int p = 0; p < pad; p++) scr_print(" ");

            switch (task_table[i].task_state) {
                case TASK_READY:    scr_print(" \033[32mrunning\033[0m\n");  break;
                case TASK_SLEEPING: scr_print(" \033[33msleeping\033[0m\n"); break;
                case TASK_DONE:     scr_print(" \033[31mdone\033[0m\n");     break;
            }
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

    if (task_table[id].state) {
        mem_free(task_table[id].state);
    }

    scr_print("  Killed task '");
    scr_print(task_table[id].name);
    scr_print("'\n");

    task_table[id].active     = 0;
    task_table[id].tick       = NULL;
    task_table[id].state      = NULL;
    task_table[id].task_state = TASK_DONE;
}

void task_sleep(int id, int ticks)
{
    if (id < 0 || id >= MAX_TASKS || !task_table[id].active) return;
    task_table[id].task_state  = TASK_SLEEPING;
    task_table[id].sleep_ticks = ticks;
}

Task *task_get(int id)
{
    if (id < 0 || id >= MAX_TASKS) return (Task *)0;
    return &task_table[id];
}
