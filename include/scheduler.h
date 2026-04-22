#ifndef SCHEDULER_H
#define SCHEDULER_H

/*
 * scheduler.h — Preemptive Scheduler Interface
 *
 * Uses SIGALRM + setitimer to fire every 50ms, simulating
 * a hardware timer interrupt. The signal handler sets a flag;
 * the main loop dispatches task_tick_all() when the flag is set.
 *
 * This keeps the signal handler signal-safe (no malloc/printf).
 */

/* Initialize: arm SIGALRM at SCHED_INTERVAL_MS intervals */
void sched_init(void);

/* Call from main loop / blocking ops to drain pending ticks */
void sched_dispatch(void);

/* Returns 1 if a tick is pending (used by keyboard.c) */
int sched_tick_pending(void);

#define SCHED_INTERVAL_MS 50

#endif /* SCHEDULER_H */
