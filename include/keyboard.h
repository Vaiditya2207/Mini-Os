#ifndef KEYBOARD_H
#define KEYBOARD_H

/*
 * keyboard.h — Non-Blocking Input Handler Interface
 *
 * Raw terminal mode for real-time key detection.
 */

#define KEY_UP        1000
#define KEY_DOWN      1001
#define KEY_LEFT      1002
#define KEY_RIGHT     1003
#define KEY_ESCAPE    27
#define KEY_ENTER     10
#define KEY_BACKSPACE 127

void kb_init(void);
void kb_restore(void);
int  kb_key_pressed(void);
int  kb_read_line(char *buf, int max_len);

#endif /* KEYBOARD_H */
