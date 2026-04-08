/*
 * string.c — String Utilities
 *
 * All operations use raw pointer arithmetic. No <string.h>.
 */

#include "../include/string.h"

int str_length(const char *s)
{
    if (!s) return 0;
    int len = 0;
    while (s[len] != '\0') len++;
    return len;
}

void str_copy(char *dst, const char *src, int max_len)
{
    if (!dst || !src || max_len <= 0) return;
    int i = 0;
    while (i < max_len - 1 && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

int str_compare(const char *a, const char *b)
{
    if (!a && !b) return  0;
    if (!a)       return -1;
    if (!b)       return  1;

    while (*a && *b && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

void str_concat(char *dst, const char *src, int max_len)
{
    if (!dst || !src || max_len <= 0) return;
    int dst_len = str_length(dst);
    int i = 0;
    while (dst_len + i < max_len - 1 && src[i] != '\0') {
        dst[dst_len + i] = src[i];
        i++;
    }
    dst[dst_len + i] = '\0';
}

void str_reverse(char *s, int len)
{
    if (!s || len <= 1) return;
    int left = 0, right = len - 1;
    while (left < right) {
        char tmp = s[left];
        s[left]  = s[right];
        s[right] = tmp;
        left++; right--;
    }
}
