# Mini OS

A freestanding mini operating system written in C — built from scratch with no standard library beyond `<stdio.h>` for terminal I/O. Every subsystem (memory, strings, filesystem, scheduler, shell) is implemented from first principles.

```
  ╔══════════════════════════════════════════════════════╗
  ║   ███╗   ███╗██╗███╗   ██╗██╗     ██████╗ ███████╗   ║
  ║   ████╗ ████║██║████╗  ██║██║    ██╔═══██╗██╔════╝   ║
  ║   ██╔████╔██║██║██╔██╗ ██║██║    ██║   ██║███████╗   ║
  ║   ██║╚██╔╝██║██║██║╚██╗██║██║    ██║   ██║╚════██║   ║
  ║   ██║ ╚═╝ ██║██║██║ ╚████║██║    ╚██████╔╝███████║   ║
  ║   ╚═╝     ╚═╝╚═╝╚═╝  ╚═══╝╚═╝     ╚═════╝ ╚══════╝   ║
  ║                                                      ║
  ║   Freestanding Mini Operating System v1.0            ║
  ╚══════════════════════════════════════════════════════╝
```

---

## What is this?

Mini OS is a systems programming project that simulates core OS concepts inside a single Unix process. It is not a bootable kernel — it runs in your terminal and demonstrates how real operating systems work under the hood:

- How memory allocators manage a heap with free lists and coalescing
- How a shell tokenizes and dispatches commands
- How a virtual filesystem tracks files and directories with inodes
- How a preemptive scheduler uses hardware timer interrupts (SIGALRM) to context-switch between tasks
- How shell scripts are parsed and executed, both blocking and in the background

Everything is implemented in plain C99, compiled with `-Wall -Wextra -Werror`.

---

## Architecture

```
main.c
  ├── memory.c     Virtual heap allocator (first-fit free list, 8-byte aligned)
  ├── keyboard.c   Raw-mode terminal input with EINTR-safe read loop
  ├── screen.c     ANSI terminal renderer (double-buffered, diff-based refresh)
  ├── scheduler.c  Preemptive scheduler — SIGALRM @ 50ms, signal-safe ISR
  └── shell.c      REPL loop — tokenize → dispatch → output
        ├── string.c   Custom string library (no <string.h>)
        ├── math.c     Integer arithmetic, spatial helpers, LCG RNG
        ├── vfs.c      Virtual filesystem (64-inode table, nested directories)
        ├── task.c     Task state machine (READY / SLEEPING / DONE)
        └── script.c   Shell script engine (VFS file → line-by-line execution)
```

### Boot sequence

```
main()
  1. mem_init()      — map virtual RAM (64 KiB static array) into the heap allocator
  2. kb_init()       — switch terminal to raw non-blocking mode
  3. scr_init()      — initialize ANSI screen layer
  4. shell_init()    — mount VFS root, initialize task table
  5. sched_init()    — arm SIGALRM timer (50ms intervals)
  6. shell_run()     — enter REPL loop
  7. kb_restore()    — restore terminal on exit
```

---

## Subsystems

### Memory Allocator (`memory.c`)

A first-fit free list allocator over a 64 KiB static array — no `malloc`.

- **Block splitting** — large free blocks are split to fit exact allocation sizes
- **Forward coalescing** — adjacent free blocks are merged on every `mem_free()` to prevent fragmentation
- **8-byte alignment** — all allocations are aligned via `ALIGN(size)`
- **Double-free protection** — ignores attempts to free an already-free block
- **Bounds checking** — rejects pointers outside the heap region

```c
mem_init(ram, VIRTUAL_RAM_SIZE);   // initialize over a static array
void *p = mem_alloc(128);          // allocate 128 bytes
mem_free(p);                       // free and coalesce
mem_dump();                        // print heap map (shell: memmap)
```

### String Library (`string.c`)

Complete string toolkit with zero `<string.h>` dependency.

| Function | Description |
|---|---|
| `str_length` | strlen equivalent |
| `str_copy` | Safe strncpy (always null-terminates) |
| `str_compare` | Lexicographic comparison |
| `str_concat` | Safe strncat |
| `str_reverse` | In-place reversal |
| `str_itoa` | Integer → string |
| `str_atoi` | String → integer (handles sign, whitespace) |
| `str_starts_with` | Prefix check |
| `str_find` | Character search |
| `str_split` | In-place tokenizer — splits on delimiter, returns pointer array |

### Virtual Filesystem (`vfs.c`)

A Unix-style filesystem backed by a 64-slot inode table. File data is heap-allocated via `memory.c`.

- Root directory at inode 0
- Nested directories supported (parent inode index tracked per node)
- File data stored as heap-allocated strings, freed on `rm`
- Current working directory tracked in a superblock

```
mini-os:/$ mkdir projects
mini-os:/$ cd projects
mini-os:/projects$ touch notes.txt
mini-os:/projects$ write notes.txt this is my note
mini-os:/projects$ cat notes.txt
  this is my note
mini-os:/projects$ ls
  notes.txt  15 B
  1 item(s)
```

### Preemptive Scheduler (`scheduler.c` + `task.c`)

Background tasks run concurrently with the interactive shell using `SIGALRM`.

**How it works:**
1. `sched_init()` arms `setitimer(ITIMER_REAL, 50ms)`
2. Every 50ms, the OS delivers `SIGALRM` to the process
3. The signal handler (the "ISR") does one thing: set `tick_pending = 1`
4. `sched_dispatch()` — called from the REPL loop and `kb_read_line` on `EINTR` — drains the flag and calls `task_tick_all()`
5. This mirrors real OS preemption: hardware IRQ → ISR → scheduler

**Task states:**

```
TASK_READY    → tick function runs on every scheduler tick
TASK_SLEEPING → countdown decrements; transitions to READY at zero
TASK_DONE     → slot is reaped automatically by the scheduler
```

```
mini-os:/$ startcounter
  Started counter task (ID: 0)
mini-os:/$ tasks
  ID   Name             State
  ──── ──────────────── ────────
  0    counter          running
mini-os:/$ kill 0
  Killed task 'counter'
```

### Shell Script Engine (`script.c`)

Scripts are plain VFS files — one shell command per line, `#` for comments.

**Foreground** (`run script.sh`) — executes all lines synchronously, shell blocks until done.  
**Background** (`run script.sh &`) — registers a scheduler task; each tick executes one line. Shell stays interactive.

```bash
# Example: write a script into the VFS
mini-os:/$ write setup.sh mkdir data
mini-os:/$ write setup.sh cd data
# Note: use write to append lines to build multi-line scripts

mini-os:/$ run setup.sh       # blocks until done
mini-os:/$ run setup.sh &     # runs in background, returns immediately
mini-os:/$ tasks              # monitor background scripts
```

---

## Shell Commands

| Command | Description |
|---|---|
| `help` | Show command reference |
| `echo <text>` | Print text to console |
| `clear` | Clear the screen |
| `ls` | List files in current directory |
| `touch <name>` | Create an empty file |
| `mkdir <name>` | Create a directory |
| `cd <dir>` | Change directory (`..` and `/` supported) |
| `write <file> <text>` | Write content to a file |
| `read` / `cat <file>` | Display file contents |
| `rm <name>` | Remove a file or empty directory |
| `run <file>` | Run a shell script (foreground) |
| `run <file> &` | Run a shell script (background) |
| `tasks` | List all background tasks and their state |
| `kill <id>` | Kill a background task by ID |
| `startcounter` | Start a background counter task (demo) |
| `memmap` | Print the heap memory map |
| `sysinfo` | Show system statistics |
| `exit` | Shutdown Mini OS |

---

## Getting Started

**Requirements:** `clang` (or `gcc`), macOS or Linux, C99.

```bash
# Build
make

# Run
./mini_os

# Run tests
make test

# Clean
make clean
```

### Quick demo session

```
mini-os:/$ mkdir projects
  Created directory 'projects'

mini-os:/$ cd projects
mini-os:/projects$ touch readme.txt
  Created 'readme.txt'

mini-os:/projects$ write readme.txt hello from mini os
  Written to 'readme.txt' (17 bytes)

mini-os:/projects$ cat readme.txt
  hello from mini os

mini-os:/projects$ ls

  readme.txt  17 B

  1 item(s)

mini-os:/projects$ sysinfo

  ╔═══════════════════════════════════╗
  ║       System Information          ║
  ╠═══════════════════════════════════╣
  ║  Virtual RAM:  65536 bytes        ║
  ║  Free Memory:  65480 bytes        ║
  ║  Files:        1                  ║
  ║  Directories:  2                  ║
  ╚═══════════════════════════════════╝

mini-os:/projects$ exit
```

---

## Project Structure

```
Mini-OS/
├── src/
│   ├── main.c        Entry point and boot sequence
│   ├── memory.c      Virtual heap allocator
│   ├── string.c      String utilities (no stdlib)
│   ├── screen.c      ANSI terminal renderer
│   ├── keyboard.c    Raw-mode keyboard input
│   ├── math.c        Integer math and RNG
│   ├── vfs.c         Virtual filesystem
│   ├── task.c        Task scheduler
│   ├── scheduler.c   Preemptive timer (SIGALRM)
│   └── shell.c       Shell REPL and commands
│   └── script.c      Shell script engine
├── include/
│   ├── memory.h
│   ├── string.h
│   ├── screen.h
│   ├── keyboard.h
│   ├── math.h
│   ├── vfs.h
│   ├── task.h
│   ├── scheduler.h
│   ├── script.h
│   └── shell.h
├── tests/
│   ├── test_memory.c
│   ├── test_math.c
│   ├── test_string.c
│   └── test_shell.c
└── Makefile
```

---

## Development Phases

| Phase | Dates | What was built |
|---|---|---|
| Phase 1 | Apr 7 – Apr 14 | Memory allocator, string library, screen/keyboard drivers, math engine, shell REPL, VFS, cooperative task scheduler, unit tests |
| Phase 2 | Apr 22 – Apr 24 | Preemptive scheduler (SIGALRM), task state machine, shell script engine (foreground + background execution) |

---

## Design Principles

- **No stdlib for core logic** — `<string.h>`, `<stdlib.h>`, `<math.h>` are not used. Every function is implemented from scratch.
- **One responsibility per module** — each `.c` file does exactly one thing and communicates through a clean header interface.
- **Memory goes through `memory.c`** — every heap allocation in the OS (VFS file data, task state, script buffers) uses `mem_alloc` / `mem_free`.
- **Signal safety** — the SIGALRM handler only sets a flag. All real work happens in `sched_dispatch()` in the normal execution context.
- **Everything is testable** — modules have no hidden dependencies, making unit testing straightforward.
