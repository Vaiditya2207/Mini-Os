#ifndef TASK_H
#define TASK_H

/*
 * task.h — Task Scheduler Interface
 *
 * Task states:
 *   TASK_READY    → tick_fn will be called on next dispatch
 *   TASK_SLEEPING → waiting for sleep_ticks to reach 0
 *   TASK_DONE     → finished, slot available for reuse
 */

#define MAX_TASKS    8
#define MAX_NAME_LEN 32

typedef enum {
    TASK_READY    = 0,
    TASK_SLEEPING = 1,
    TASK_DONE     = 2
} TaskState;

typedef struct {
    void      (*tick)(void *state);
    void       *state;
    int         active;
    TaskState   task_state;
    int         sleep_ticks;   /* countdown when TASK_SLEEPING */
    char        name[MAX_NAME_LEN];
} Task;

void task_init(void);
int  task_add(const char *name, void (*tick_fn)(void *), void *state);
void task_tick_all(void);
void task_list(void);
void task_kill(int id);
void task_sleep(int id, int ticks);   /* put task to sleep for N ticks */
Task *task_get(int id);               /* get raw task slot (for script engine) */

#endif /* TASK_H */
