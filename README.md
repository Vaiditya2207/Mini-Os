# Mini OS

A freestanding mini operating system written entirely in C — no standard library, no shortcuts. Every subsystem (memory allocator, string parser, virtual filesystem, preemptive scheduler, shell, script engine) is implemented from scratch using raw pointer arithmetic, POSIX signals, and terminal I/O.

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
  ║   Built with custom C libraries — no libc            ║
  ╚══════════════════════════════════════════════════════╝
```

---

## Table of Contents

- [What is this?](#what-is-this)
- [Our Approach](#our-approach)
- [Getting Started](#getting-started)
- [Architecture](#architecture)
- [Boot Sequence](#boot-sequence)
- [Subsystems (Deep Dive)](#subsystems-deep-dive)
  - [Memory Allocator](#1-memory-allocator-memoryc)
  - [String Library](#2-string-library-stringc)
  - [Keyboard Input](#3-keyboard-input-keyboardc)
  - [Screen Output](#4-screen-output-screenc)
  - [Math Engine](#5-math-engine-mathc)
  - [Virtual Filesystem](#6-virtual-filesystem-vfsc)
  - [Task Scheduler](#7-task-scheduler-taskc)
  - [Preemptive Scheduler](#8-preemptive-scheduler-schedulerc)
  - [Shell Script Engine](#9-shell-script-engine-scriptc)
  - [Shell REPL](#10-shell-repl-shellc)
- [Shell Commands Reference](#shell-commands-reference)
- [Shell Scripting Guide](#shell-scripting-guide)
- [Unit Tests](#unit-tests)
- [Project Structure](#project-structure)
- [Development Phases](#development-phases)
- [Design Principles](#design-principles)

---

## What is this?

Mini OS is a systems programming project that simulates real operating system internals inside a Unix terminal process. It is **not** a bootable kernel — it is a faithful simulation of OS concepts that you can run, poke, and break on any Mac or Linux machine.

The goal was to answer: *"What if you had to build an OS from nothing — no malloc, no printf, no string.h — what would each piece actually look like?"*

The result is a fully interactive shell with a real heap allocator, a real inode-based filesystem, a real preemptive scheduler driven by hardware timer signals, and a script engine that runs programs stored on the virtual filesystem — all wired together and talking through clean C interfaces.

---

## Our Approach

### No standard library for core logic

The rule throughout this project: `<string.h>`, `<stdlib.h>`, and `<math.h>` are banned. Every string operation (`strlen`, `strcpy`, `strcmp`, tokenization), every number conversion (`itoa`, `atoi`), and every math function (`abs`, `clamp`, `rand`) is implemented from scratch using raw pointer arithmetic and bitwise operations.

The only permitted standard headers are:
- `<stdio.h>` — for `printf` / `fflush` in debug output and terminal rendering
- `<stddef.h>` — for `size_t`
- `<signal.h>`, `<sys/time.h>` — for the preemptive scheduler
- `<termios.h>`, `<fcntl.h>`, `<unistd.h>`, `<errno.h>` — for raw keyboard I/O

### Everything is a real thing

Every concept maps directly to how a real OS works:

| Mini OS | Real OS equivalent |
|---|---|
| `static char virtual_ram[64KB]` | Physical RAM |
| `mem_alloc` / `mem_free` | `kmalloc` / `kfree` |
| `inode_table[64]` | Inode table on a disk partition |
| `SIGALRM` signal handler | Hardware timer interrupt (IRQ0) |
| `tick_pending` flag | Interrupt pending bit |
| `sched_dispatch()` | Scheduler running in kernel context after IRQ |
| `task_tick_all()` | Context switch loop |
| Background script task | OS process |

### One module, one job

Every `.c` file has a single, clearly defined responsibility. Modules talk to each other only through their `.h` interfaces. You can understand `vfs.c` without reading `shell.c`. You can test `memory.c` without touching `string.c`.

### Memory flows through one allocator

Every heap allocation in the system — VFS file data, task state structs, script line buffers — goes through `mem_alloc` / `mem_free`. There is no hidden `malloc` anywhere. This means `memmap` gives you a true picture of exactly what the system is doing with memory at any point.

### Signal safety is taken seriously

The `SIGALRM` handler sets exactly one `volatile sig_atomic_t` flag and nothing else. No `printf`, no `malloc`, no function calls. The actual scheduler work happens in `sched_dispatch()` in the normal execution context. This is exactly how production OS interrupt handlers work.

---

## Getting Started

**Requirements:** `clang` (or `gcc`), macOS or Linux, C99.

```bash
# Clone
git clone git@github.com:Vaiditya2207/Mini-Os.git
cd Mini-Os

# Build
make

# Run
./mini_os

# Run all unit tests
make test

# Clean build artifacts
make clean
```

Once running, type `help` for the full command reference.

---

## Architecture

```
main.c  (entry point + boot sequence)
  │
  ├── memory.c      Virtual heap allocator
  │     └── [all subsystems call mem_alloc/mem_free]
  │
  ├── keyboard.c    Raw-mode terminal input (EINTR-aware)
  │     └── scheduler.c   SIGALRM timer, dispatches task ticks
  │
  ├── screen.c      ANSI terminal renderer
  │
  └── shell.c       REPL: read → tokenize → dispatch → output
        │
        ├── string.c      Custom string library
        ├── math.c        Integer math + spatial + RNG
        ├── vfs.c         Virtual filesystem (inode table)
        ├── task.c        Task state machine
        ├── scheduler.c   Preemptive tick dispatch
        └── script.c      Script engine (VFS file → line executor)
```

### Data flow for a single command

```
User types "write notes.txt hello world"
         │
         ▼
  kb_read_line()          keyboard.c — raw read, EINTR restarts scheduler
         │
         ▼
  str_split(buf, ' ')     string.c — tokenize in-place
         │
         ▼
  shell_execute(tokens)   shell.c — match command string
         │
         ▼
  vfs_write("notes.txt")  vfs.c — find inode, mem_alloc data block
         │
         ▼
  mem_alloc(len+1)        memory.c — first-fit allocation from virtual RAM
         │
         ▼
  scr_print("Written...")  screen.c — fputs to stdout
         │
         ▼
  sched_dispatch()        scheduler.c — run any pending task ticks
```

---

## Boot Sequence

```c
int main(void)
{
    mem_init(virtual_ram, 64KB);  // 1. Map static array into heap allocator
    kb_init();                    // 2. Switch terminal to raw non-blocking mode
    scr_init(80, 24);             // 3. Initialize ANSI screen layer
    shell_init();                 // 4. Mount VFS root (/), init task table
    sched_init();                 // 5. Arm SIGALRM at 50ms intervals
    shell_run();                  // 6. Enter REPL loop (blocks until exit)
    kb_restore();                 // 7. Restore terminal settings on shutdown
}
```

---

## Subsystems (Deep Dive)

### 1. Memory Allocator (`memory.c`)

A **first-fit free list** allocator over a 64 KiB static array. No `malloc` anywhere.

```
virtual_ram[65536]
├── [BlockHeader | data...........] ← allocated block
├── [BlockHeader | data..] ← allocated block
├── [BlockHeader | ........................] ← free block
└── [BlockHeader | ........] ← free block
```

**Block header layout:**
```c
typedef struct BlockHeader {
    size_t              size;     // total size including header
    int                 is_free;
    struct BlockHeader *next;     // linked list pointer
} BlockHeader;
```

**Allocation — `mem_alloc(size)`:**
1. Round `size` up to 8-byte alignment: `ALIGN(size)`
2. Walk the linked list looking for first free block `>= total_needed`
3. If remainder is large enough (`>= MIN_BLOCK_SIZE`), split into two blocks
4. Mark block as used, return pointer past the header

**Deallocation — `mem_free(ptr)`:**
1. Recover header from `ptr - HEADER_SIZE`
2. Bounds-check: reject pointers outside the heap
3. Double-free guard: silently ignore if already free
4. Mark as free
5. **Forward coalescing**: scan the full list and merge adjacent free blocks

**Why coalescing matters:**
```
Before coalesce:  [USED][FREE 32B][FREE 64B][USED]
After coalesce:   [USED][FREE 96B          ][USED]
```
Without coalescing, the heap fragments and small freed blocks become unusable for larger allocations even though the total free memory is sufficient.

**Debug commands in shell:**
```
mini-os:/$ memmap       — print full heap map with block addresses and sizes
mini-os:/$ sysinfo      — show free/used totals and block count
```

---

### 2. String Library (`string.c`)

Complete string toolkit. Zero `<string.h>` usage. All operations use raw pointer arithmetic.

#### Core operations

```c
int  str_length(const char *s)
// Walk s until '\0', return count. NULL-safe (returns 0).

void str_copy(char *dst, const char *src, int max_len)
// Copy src to dst, stop at max_len-1. Always null-terminates.
// Safer than strncpy which does not guarantee termination.

int  str_compare(const char *a, const char *b)
// Lexicographic compare. Returns 0 (equal), <0 (a<b), >0 (a>b).
// Handles NULL pointers explicitly.

void str_concat(char *dst, const char *src, int max_len)
// Append src to dst without exceeding max_len. Bounds-safe strcat.

void str_reverse(char *s, int len)
// Reverse string in-place using two-pointer swap. O(n), O(1) space.
```

#### Numeric conversions

```c
int str_itoa(int num, char *buf, int buf_size)
// Integer to string. Handles negatives and zero.
// Builds digits in reverse, then calls str_reverse.

int str_atoi(const char *s)
// String to integer. Skips leading whitespace, handles +/- sign.
// Returns 0 for non-numeric input (not a crash).
```

#### Search

```c
int   str_starts_with(const char *s, const char *prefix)
// Returns 1 if s begins with prefix. Used in command matching.

char *str_find(const char *s, char ch)
// Returns pointer to first occurrence of ch in s, or NULL.
```

#### Tokenizer — the heart of the shell

```c
int str_split(char *input, char delimiter, char **tokens, int max_tokens)
```

This is the most important function in the library. It tokenizes `input` **in-place**: it walks the string, replaces delimiter characters with `'\0'`, and stores pointers to each token in the `tokens` array. No heap allocation, no copying — tokens point directly into the original buffer.

```
Input:  "write  notes.txt  hello world\0"
After:  "write\0 notes.txt\0 hello world\0"
         ↑          ↑         ↑
tokens:  [0]       [1]       [2]       count = 3
```

Handles multiple consecutive delimiters, leading delimiters, and trailing delimiters correctly — just like a real shell tokenizer.

---

### 3. Keyboard Input (`keyboard.c`)

Switches the terminal into **raw non-blocking mode** using `termios` and `fcntl`.

**What raw mode means:**
- `ICANON` disabled — no line buffering, characters available immediately
- `ECHO` disabled — we echo manually so we control what appears
- `VMIN = 0`, `VTIME = 0` — non-blocking reads

**Line reading — `kb_read_line()`:**
- Switches back to **blocking** mode for the read loop (so CPU doesn't spin)
- Handles backspace: moves cursor back, erases character, adjusts buffer
- Skips control characters (Ctrl+C etc.) except Tab
- On `EINTR` (signal interrupt from SIGALRM): calls `sched_dispatch()` then restarts the read — this is how background tasks tick even while you're mid-input
- Decodes arrow key escape sequences (`ESC [ A/B/C/D`)

**Restoration — `kb_restore()`:**
Registered with `atexit()` so the terminal is always restored even on abnormal exit. Without this, your terminal would be left in raw mode after the process ends.

---

### 4. Screen Output (`screen.c`)

A double-buffered ANSI terminal renderer with diff-based refresh.

**Two buffers:**
- `back[][]` — the buffer your code writes to (logical state)
- `front[][]` — what is currently displayed on the terminal

**`scr_refresh()`** compares every cell. If `back[y][x] != front[y][x]`, it emits the minimal ANSI sequence to update just that cell. This eliminates flicker and reduces terminal writes dramatically.

**Write buffer:** All output is accumulated into a local `write_buf[]` and flushed in a single `fputs()` call per refresh, avoiding the overhead of many small writes.

**Console wrappers used by the shell:**
```c
void scr_print(const char *s)    // fputs to stdout + fflush
void scr_println(const char *s)  // fputs + '\n' + fflush
void scr_clear(void)             // ESC[2J ESC[H
```

---

### 5. Math Engine (`math.c`)

Pure integer math. No `<math.h>`.

```c
// Arithmetic
int m_add(int a, int b)
int m_sub(int a, int b)
int m_mul(int a, int b)
int m_div(int a, int b)   // returns 0 on divide-by-zero (safe)
int m_mod(int a, int b)   // handles negative operands correctly
int m_abs(int x)
int m_min(int a, int b)
int m_max(int a, int b)
int m_clamp(int val, int lo, int hi)

// Spatial (for game/simulation use)
int m_aabb_intersect(...)    // axis-aligned bounding box collision
int m_point_in_rect(...)     // point-in-rectangle test
int m_distance(...)          // Manhattan distance

// LCG Random Number Generator
void m_srand(unsigned int seed)
int  m_rand(void)            // returns [0, 32767]
int  m_rand_range(int min, int max)
```

The RNG uses a linear congruential generator:
```c
state = state * 1103515245u + 12345u;
return (state >> 16) & 0x7FFF;
```
Same algorithm used in glibc's `rand()`. Seeded with `m_srand()`, deterministic for testing.

---

### 6. Virtual Filesystem (`vfs.c`)

A Unix-style filesystem backed by a **64-slot inode table**. File data is heap-allocated via `memory.c`. No real disk I/O.

**Inode structure:**
```c
typedef struct {
    char  name[32];    // filename or directory name
    int   size;        // data size in bytes (0 for directories)
    int   is_dir;      // 1 = directory, 0 = file
    int   is_used;     // slot in use
    char *data;        // heap-allocated file content (NULL for dirs)
    int   parent;      // parent inode index (-1 for root)
} Inode;
```

**Superblock:**
```c
typedef struct {
    int total_files;
    int total_dirs;
    int current_dir;   // current working directory inode index
} Superblock;
```

**How the directory tree works:**

Every inode stores its parent's index. Root is always inode 0. To list a directory, scan all inodes where `parent == current_dir`. To build a path, walk the parent chain upward.

```
inode[0]  name="/"     parent=-1   is_dir=1   ← root
inode[1]  name="data"  parent=0    is_dir=1   ← /data
inode[2]  name="notes" parent=1    is_dir=0   ← /data/notes
inode[3]  name="docs"  parent=0    is_dir=1   ← /docs
```

**Write creates a heap-allocated data block:**
```c
// vfs_write("notes.txt", "hello world")
char *block = (char *)mem_alloc(content_len + 1);
str_copy(block, content, content_len + 1);
inode_table[idx].data = block;
inode_table[idx].size = content_len;
```

**Remove frees the data block:**
```c
// vfs_rm("notes.txt")
if (inode_table[idx].data) {
    mem_free(inode_table[idx].data);  // returned to heap
}
inode_table[idx].is_used = 0;
```

This means every byte of file content is tracked by the memory allocator — `memmap` will show the file data as an allocated block.

---

### 7. Task Scheduler (`task.c`)

An 8-slot task table. Each task has a function pointer, a state pointer, and a `TaskState`.

**Task structure:**
```c
typedef struct {
    void      (*tick)(void *state);  // called on every READY tick
    void       *state;               // heap-allocated task-specific data
    int         active;              // slot in use
    TaskState   task_state;          // READY / SLEEPING / DONE
    int         sleep_ticks;         // countdown for SLEEPING state
    char        name[32];
} Task;
```

**State machine:**
```
          task_add()
              │
              ▼
          TASK_READY ──────────── tick() called every 50ms
              │
      task_sleep(id, n)
              │
              ▼
        TASK_SLEEPING ─── sleep_ticks-- each tick
              │
          reaches 0
              │
              ▼
          TASK_READY
              │
        tick() sets
        task_state = DONE
              │
              ▼
          TASK_DONE ─── slot reaped by scheduler on next pass
```

**`task_tick_all()`** is called by the scheduler on every SIGALRM tick:
```c
for each active task:
    SLEEPING → decrement sleep_ticks; if 0 → READY
    READY    → call tick(state)
    DONE     → mark active=0 (reap slot)
```

---

### 8. Preemptive Scheduler (`scheduler.c`)

This is the most OS-like part of the project. It uses `SIGALRM` to simulate a hardware timer interrupt.

**How real preemption works in an OS:**
```
Hardware timer fires every N ms
    → CPU receives IRQ (interrupt request)
    → CPU saves current register state
    → CPU jumps to ISR (interrupt service routine)
    → ISR saves context, picks next process, restores its context
    → CPU resumes execution in the new process
```

**How Mini OS does it:**
```
setitimer fires SIGALRM every 50ms
    → Unix delivers signal to process
    → Execution jumps to sigalrm_handler()
    → Handler sets tick_pending = 1  (that's ALL it does)
    → Execution returns to wherever it was interrupted
    → kb_read_line() gets EINTR, catches it, calls sched_dispatch()
    → sched_dispatch() clears flag, calls task_tick_all()
    → Background tasks run one tick each
    → kb_read_line() restarts the read()
```

**Why the handler only sets a flag:**

Signal handlers run in an async context — between any two instructions. Calling `printf`, `malloc`, or any non-reentrant function from a signal handler is undefined behavior and can corrupt state. Setting a `volatile sig_atomic_t` is the only guaranteed safe operation.

```c
static volatile sig_atomic_t tick_pending = 0;

static void sigalrm_handler(int sig)
{
    (void)sig;
    tick_pending = 1;   // this is it. nothing else.
}
```

**`SA_RESTART` flag:**

The `sigaction` call uses `SA_RESTART`, which tells the kernel to automatically restart certain syscalls that were interrupted by the signal (like `write`). For `read()`, we handle `EINTR` manually in `kb_read_line()` to dispatch the scheduler tick before restarting.

---

### 9. Shell Script Engine (`script.c`)

Scripts are plain files stored in the VFS — one shell command per line. Lines beginning with `#` are comments and are skipped.

**Loading a script:**

`load_script()` calls `vfs_read_to_buf()` to get the raw file content as a string, then splits it on `'\n'` into a line table:

```c
typedef struct {
    char lines[64][256];   // up to 64 lines, each up to 256 chars
    int  total_lines;
    int  current_line;
} ScriptState;
```

**Foreground execution — `script_run_fg(filename)`:**

Executes all lines synchronously. For each line, it calls `str_split()` and then `shell_execute()` — exactly the same path as interactive input. The shell is blocked until the script finishes.

```
run setup.sh
  Running 'setup.sh' (4 lines, foreground)...
  > mkdir data
  Created 'data'
  > cd data
  > touch readme.txt
  Created 'readme.txt'
  > write readme.txt hello
  Written to 'readme.txt' (5 bytes)
  Script completed.
```

**Background execution — `script_run_bg(filename)`:**

Allocates a `ScriptState` on the heap via `mem_alloc()`, loads the script into it, then calls `task_add()` to register a `script_bg_tick` function as a scheduler task. The shell returns immediately.

On each SIGALRM tick (every 50ms), `script_bg_tick()` executes **one line** and advances `current_line`. When the last line is done, it sets `task_state = TASK_DONE` and the scheduler reaps the slot automatically.

```
run setup.sh &
  Started 'setup.sh' in background (task 0, 4 lines)
mini-os:/$ tasks
  ID   Name             State
  0    script:setup.sh  running
```

---

### 10. Shell REPL (`shell.c`)

The REPL (Read-Evaluate-Print Loop) is the central nervous system of Mini OS. Every user interaction flows through it.

**REPL loop:**
```c
while (shell_running) {
    scr_print("mini-os:/path$ ");
    kb_read_line(cmd_buf, CMD_BUF_SIZE);      // blocking read (EINTR-aware)
    str_split(cmd_buf, ' ', tokens, MAX);     // tokenize in-place
    shell_execute(tokens, count);             // dispatch command
    sched_dispatch();                         // drain pending scheduler ticks
}
```

**Command dispatch** is a simple chain of `str_compare` checks against `tokens[0]`. No hash table, no function pointer table — just a readable if/else chain that maps directly to the command name.

---

## Shell Commands Reference

### Navigation & Filesystem

| Command | Usage | Description |
|---|---|---|
| `ls` | `ls` | List all files and directories in the current directory. Shows name and size in bytes for files, name with trailing `/` for directories. |
| `cd` | `cd <dir>` | Change current directory. Use `..` to go up, `/` to jump to root. |
| `touch` | `touch <filename>` | Create an empty file. Fails if the name already exists. |
| `mkdir` | `mkdir <dirname>` | Create a new directory. Fails if the name already exists. |
| `write` | `write <filename> <content...>` | Write content to a file. Creates the file if it doesn't exist. Overwrites existing content. All tokens after the filename are joined with spaces as the content. |
| `read` / `cat` | `cat <filename>` | Display the contents of a file. |
| `rm` | `rm <name>` | Remove a file or an empty directory. Refuses to remove non-empty directories. |

**Examples:**
```
mini-os:/$ mkdir projects
  Created directory 'projects'

mini-os:/$ cd projects
mini-os:/projects$ touch notes.txt
  Created 'notes.txt'

mini-os:/projects$ write notes.txt this is my first note
  Written to 'notes.txt' (20 bytes)

mini-os:/projects$ cat notes.txt
  this is my first note

mini-os:/projects$ ls

  notes.txt  20 B

  1 item(s)

mini-os:/projects$ cd ..
mini-os:/$ rm projects
  Error: directory 'projects' is not empty

mini-os:/$ cd projects && rm notes.txt
  Removed 'notes.txt'

mini-os:/projects$ cd .. && rm projects
  Removed 'projects'
```

---

### Shell Utilities

| Command | Usage | Description |
|---|---|---|
| `echo` | `echo <text...>` | Print all arguments to the console, joined by spaces. |
| `clear` | `clear` | Clear the terminal screen. |
| `help` | `help` | Display the full command reference with formatting. |
| `exit` | `exit` | Gracefully shut down Mini OS (restores terminal, frees resources). |

**Examples:**
```
mini-os:/$ echo hello world
hello world

mini-os:/$ echo Mini OS is running
Mini OS is running
```

---

### Memory

| Command | Usage | Description |
|---|---|---|
| `memmap` | `memmap` | Print the full heap memory map: every block with its address, size, and status (FREE/USED). Shows fragmentation state in real time. |
| `sysinfo` | `sysinfo` | System summary: virtual RAM total, free memory, heap block count, file count, directory count, max inodes. |

**Example output:**
```
mini-os:/$ memmap

=== HEAP MEMORY MAP ===
Heap base: 0x104d38080 | Total size: 65536 bytes

  Block  0 | USED | size:    40 B (data:    16 B) | addr: 0x104d38080
  Block  1 | FREE | size: 65496 B (data: 65472 B) | addr: 0x104d380a8

  ─────────────────────────────────────────────────
  Blocks: 2 | Used: 16 B | Free: 65472 B
===========================
```

---

### Tasks & Scheduler

| Command | Usage | Description |
|---|---|---|
| `tasks` | `tasks` | List all active background tasks with their ID, name, and current state (running / sleeping / done). |
| `kill` | `kill <id>` | Kill a background task by its numeric ID. Frees the task's heap-allocated state. |
| `startcounter` | `startcounter` | Start a demo background task that increments a counter on every scheduler tick (every 50ms). Useful to verify the preemptive scheduler is working. |

**Example:**
```
mini-os:/$ startcounter
  Started counter task (ID: 0)

mini-os:/$ tasks

  Active Background Tasks:
  ID   Name             State
  ──── ──────────────── ────────
  0    counter          running

mini-os:/$ kill 0
  Killed task 'counter'
```

---

### Scripts

| Command | Usage | Description |
|---|---|---|
| `run <file>` | `run setup.sh` | Execute a script stored in the VFS. Runs in **foreground** — shell blocks until every line has executed. Each line is printed before execution. |
| `run <file> &` | `run setup.sh &` | Execute a script in **background** — registers it as a scheduler task, shell returns immediately. The script executes one line every 50ms tick. Monitor with `tasks`. |

---

## Shell Scripting Guide

A Mini OS script is a plain VFS file with one shell command per line. Lines starting with `#` are comments.

### Writing a script

Use `write` to store commands in a VFS file:

```
mini-os:/$ write build.sh mkdir output
mini-os:/$ write build.sh touch output/result.txt
mini-os:/$ write build.sh write output/result.txt build complete
mini-os:/$ write build.sh ls
```

> **Note:** Each `write` call overwrites the file completely. To simulate a multi-line file, write the full content as a single `write` call with `\n` separating lines, or build the content before running.

### Running in foreground

```
mini-os:/$ run build.sh
  Running 'build.sh' (4 lines, foreground)...
  > mkdir output
  Created directory 'output'
  > touch output/result.txt
  Created 'output/result.txt'
  > write output/result.txt build complete
  Written to 'output/result.txt' (13 bytes)
  > ls
  ...
  Script completed.
```

The shell is blocked for the entire duration. Good for setup scripts where you need the result before continuing.

### Running in background

```
mini-os:/$ run build.sh &
  Started 'build.sh' in background (task 1, 4 lines)

mini-os:/$ echo still interactive!
still interactive!

mini-os:/$ tasks

  Active Background Tasks:
  ID   Name              State
  1    script:build.sh   running
```

The script runs one line every 50ms in the background. The shell stays interactive. You can run multiple background scripts simultaneously (up to 8 total tasks).

### Script format

```bash
# This is a comment — ignored by the engine

mkdir data
cd data
touch log.txt
write log.txt system initialized
ls
```

Blank lines and lines with only whitespace are skipped automatically.

---

## Unit Tests

Four test suites cover the core libraries:

```bash
make test
```

| Test file | Covers |
|---|---|
| `tests/test_memory.c` | Basic alloc, free and reuse, block splitting, forward coalescing, edge cases (NULL/double-free/zero-size/OOM), 1000-cycle stress test |
| `tests/test_math.c` | abs, min/max, clamp, mod/div (including divide-by-zero safety), AABB intersection, point-in-rect, Manhattan distance, RNG determinism |
| `tests/test_string.c` | str_length, str_copy (with truncation), str_compare, str_concat, str_itoa, str_atoi (with sign/whitespace), str_starts_with, str_find, str_split (multiple delimiters), str_reverse |
| `tests/test_shell.c` | Integration test: full input → tokenize → execute → output pipeline using all libraries together |

All tests use a simple `ASSERT(cond, msg)` macro that prints `[PASS]` or `[FAIL]` with line numbers. Tests return exit code 1 on any failure, 0 on full pass.

---

## Project Structure

```
Mini-OS/
├── src/
│   ├── main.c         Entry point — boot sequence, subsystem init
│   ├── memory.c       First-fit free list heap allocator
│   ├── string.c       String library — no <string.h>
│   ├── screen.c       Double-buffered ANSI terminal renderer
│   ├── keyboard.c     Raw-mode terminal input, EINTR-aware
│   ├── math.c         Integer arithmetic, spatial helpers, LCG RNG
│   ├── vfs.c          Virtual filesystem — inode table + superblock
│   ├── task.c         Task state machine (READY/SLEEPING/DONE)
│   ├── scheduler.c    Preemptive scheduler — SIGALRM @ 50ms
│   ├── shell.c        REPL loop + command dispatch
│   └── script.c       Shell script engine (fg + bg execution)
│
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
│
├── tests/
│   ├── test_memory.c
│   ├── test_math.c
│   ├── test_string.c
│   └── test_shell.c
│
└── Makefile
```

---

## Development Phases

### Phase 1 — Apr 7 to Apr 14

Building the foundation from scratch.

| Date | Commit |
|---|---|
| Apr 7 | Initial scaffold: Makefile, .gitignore |
| Apr 7 | Virtual heap allocator: first-fit free list, block splitting |
| Apr 8 | String library: length, copy, compare, concat, reverse |
| Apr 8 | Screen renderer (double-buffered ANSI) and keyboard driver (raw mode) |
| Apr 9 | Math engine: arithmetic, spatial helpers, LCG random |
| Apr 9 | Entry point and shell skeleton: bare REPL loop |
| Apr 10 | String module extended: str_split tokenizer, itoa, atoi, find |
| Apr 10 | Shell REPL: help, echo, clear, exit — str_split-based dispatch |
| Apr 11 | Memory allocator upgraded: forward coalescing, mem_dump, memmap command |
| Apr 12 | Virtual filesystem: inode table, touch, mkdir, write, read, rm, ls, cd |
| Apr 12 | VFS integrated into shell |
| Apr 13 | Cooperative task scheduler: task table, tick loop, add/kill/list |
| Apr 13 | Tasks wired into shell: tasks, kill, startcounter, sysinfo, boot banner |
| Apr 14 | Unit tests: memory, math, string modules |
| Apr 14 | Shell integration tests + finalized Makefile |

### Phase 2 — Apr 22 to Apr 24

Preemptive scheduling and shell scripting.

| Date | Commit |
|---|---|
| Apr 22 | Task state machine: TaskState enum, task_sleep, task_get |
| Apr 22 | Preemptive scheduler: SIGALRM at 50ms, signal-safe ISR, sched_dispatch |
| Apr 22 | Keyboard fix: EINTR handling, dispatch ticks during blocking input |
| Apr 23 | VFS: vfs_read_to_buf for raw content access by script engine |
| Apr 23 | Shell script engine: load VFS file, fg blocks / bg runs as scheduler task |
| Apr 24 | Shell + main updated: run/run& commands, sched_init at boot |
| Apr 24 | Makefile updated with new modules |

---

## Design Principles

**No stdlib for core logic** — every string op, math function, and memory allocation is implemented from scratch. This forces you to understand what the library is actually doing.

**One module, one job** — each `.c` file has a single responsibility. `vfs.c` knows nothing about the shell. `memory.c` knows nothing about strings. Clean interfaces, no entanglement.

**Memory flows through one allocator** — every heap allocation in the system goes through `mem_alloc`. `memmap` gives you a complete, accurate picture of system memory at any moment.

**Signal safety is non-negotiable** — the SIGALRM handler sets exactly one flag. All scheduler work happens in `sched_dispatch()` in normal execution context. This is how production kernels handle timer interrupts.

**Every concept maps to something real** — there is no magic. The static RAM array is physical memory. The inode table is a disk partition's inode table. The signal handler is a hardware ISR. The task state machine is a process scheduler. The shell tokenizer is what every Unix shell does internally.
