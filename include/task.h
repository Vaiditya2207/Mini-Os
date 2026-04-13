#ifndef TASK_H
#define TASK_H

#define MAX_TASKS      8

void task_init(void);
int  task_add(const char *name, void (*tick_fn)(void *), void *state);
void task_tick_all(void);
void task_list(void);
void task_kill(int id);

#endif /* TASK_H */
