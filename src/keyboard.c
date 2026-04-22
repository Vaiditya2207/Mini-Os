#include "../include/keyboard.h"
#include "../include/scheduler.h"
#include <termios.h> // Controls terminal behaviour (raw mode, echo, canonical mode)
#include <fcntl.h> // File descriptor control (blocking vs non-blocking)
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

/* ── Module State ───────────────────────────────────────── */
// Static because These are internal to module and Persist across function calls
static struct termios original_termios; // stores original terminal config
static int raw_mode_active = 0; // flag to prevent double init
static int original_flags  = 0; // stores blocking mode

/* ── Restore terminal to normal mode ───────────────────────────────────────────── */

void kb_restore(void)
{
    if (!raw_mode_active) return;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios); // restore everything, echo, cannoicnal
    fcntl(STDIN_FILENO, F_SETFL, original_flags); // Restore blocking mode Flush input buffer

    raw_mode_active = 0;
} // If you don’t restore → terminal becomes unusable after program exits

/* ── Init Switch terminal to raw + non-blocking mode ─────────────────────────────────────────────── */

void kb_init(void)
{
    if (raw_mode_active) return; // Prevent reinitialization

    if (tcgetattr(STDIN_FILENO, &original_termios) == -1) return; // Save current terminal settings

    original_flags = fcntl(STDIN_FILENO, F_GETFL, 0); // Save current file descriptor flags

    struct termios raw = original_termios; // Copy settings (important!)

    raw.c_lflag &= ~(ECHO | ICANON); // Disable: ECHO → typed characters won’t show. ICANON → disables line buffering
    raw.c_cc[VMIN]  = 0; // makes read Non-blocking and Returns immediately
    raw.c_cc[VTIME] = 0; // Input is read character-by-character

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); // Apply raw mode
    fcntl(STDIN_FILENO, F_SETFL, original_flags | O_NONBLOCK); // Set non-blocking I/O

    raw_mode_active = 1;
    atexit(kb_restore);
} // disable canonical mode and echo to read input character-by-character and use non-blocking I/O for real-time interaction.

/* ── Non-blocking key Detect key press without blocking─────────────────────────────────── */

int kb_key_pressed(void)
{
    unsigned char ch;
    int n = read(STDIN_FILENO, &ch, 1);

    if (n <= 0) return 0;

    if (ch == 27) { // Arrow keys send multi-byte sequences 27 -> ESC
        unsigned char seq[2];

        if (read(STDIN_FILENO, &seq[0], 1) <= 0) return KEY_ESCAPE;
        if (read(STDIN_FILENO, &seq[1], 1) <= 0) return KEY_ESCAPE; // Read next 2 bytes

        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A': return KEY_UP;
                case 'B': return KEY_DOWN;
                case 'C': return KEY_RIGHT;
                case 'D': return KEY_LEFT;
            }
        }
        return KEY_ESCAPE;
    }

    return (int)ch;
}

/* ── Blocking line read Mini shell input───────────────────────────────── */
// Read full line with:
// Editing
// Backspace support
// Echo manually
int kb_read_line(char *buf, int max_len)
{
    if (!buf || max_len <= 0) return 0;

    int pos = 0;
// Switch to blocking mode
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);

    while (1) {
        unsigned char ch;
        int n = read(STDIN_FILENO, &ch, 1);
        if (n < 0 && errno == EINTR) {
            /* SIGALRM interrupted the read — dispatch pending scheduler ticks */
            sched_dispatch();
            continue;
        }
        if (n <= 0) continue;

        if (ch == '\n' || ch == '\r') { // enter key - end input null terminate
            buf[pos] = '\0';
            putchar('\n'); // Move to next line
            fflush(stdout);
            break;
        }

        if (ch == KEY_BACKSPACE || ch == 8) {
            if (pos > 0) {
                pos--;
                printf("\b \b");
                fflush(stdout);
            }
            continue;
        }

        if (ch < 32 && ch != '\t') continue; // Skip: Ctrl keys & Non-printables

        if (pos < max_len - 1) {
            buf[pos++] = (char)ch;
            putchar(ch);
            fflush(stdout);
        }
    }

    fflush(stdout);
    fcntl(STDIN_FILENO, F_SETFL, flags);

    return pos;
}

// This module switches the terminal into raw, non-blocking mode using termios and fcntl. It allows real-time key detection including arrow keys by decoding escape sequences. It also implements a custom line editor that handles backspace, echo, and input buffering manually, similar to how a shell works internally.
