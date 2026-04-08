#ifndef STRING_H
#define STRING_H

/*
 * string.h — String Utilities (No <string.h>)
 *
 * Core string operations using raw pointer arithmetic.
 */

int  str_length(const char *s);
void str_copy(char *dst, const char *src, int max_len);
int  str_compare(const char *a, const char *b);
void str_concat(char *dst, const char *src, int max_len);
void str_reverse(char *s, int len);

#endif /* STRING_H */
