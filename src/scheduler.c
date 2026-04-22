/*
 * scheduler.c — Preemptive Scheduler Implementation
 *
 * Architecture:
 *   setitimer(ITIMER_REAL, 50ms) fires SIGALRM repeatedly.
 *   The signal handler (the "ISR") does one thing only: set a flag.
 *   sched_dispatch() is called from the main loop and kb_read_line
 *   to actually run task_tick_all() — keeping the handler signal-safe.
 *
 * This mirrors how real OSes work:
 *   Hardware timer IRQ → ISR sets flag → scheduler runs in kernel context
 */

#include "../include/scheduler.h"
#include "../include/task.h"

#include <signal.h>
#include <sys/time.h>

/* ── Volatile flag set by the signal handler ──────────────────────────── */

static volatile sig_atomic_t tick_pending = 0;

/* ── Signal Handler (ISR) ─────────────────────────────────────────────── */

static void sigalrm_handler(int sig)
{
    (void)sig;
    tick_pending = 1;
}

/* ── Public API ───────────────────────────────────────────────────────── */

void sched_init(void)
{
    struct sigaction sa;
    sa.sa_handler = sigalrm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART; /* restart interrupted syscalls where possible */
    sigaction(SIGALRM, &sa, NULL);

    /* Arm the interval timer: fire every SCHED_INTERVAL_MS milliseconds */
    struct itimerval timer;
    timer.it_value.tv_sec     = 0;
    timer.it_value.tv_usec    = SCHED_INTERVAL_MS * 1000;
    timer.it_interval.tv_sec  = 0;
    timer.it_interval.tv_usec = SCHED_INTERVAL_MS * 1000;
    setitimer(ITIMER_REAL, &timer, NULL);
}

void sched_dispatch(void)
{
    if (tick_pending) {
        tick_pending = 0;
        task_tick_all();
    }
}

int sched_tick_pending(void)
{
    return tick_pending;
}
