/*
 * libc.h - macros per fer els traps amb diferents arguments
 *          definició de les crides a sistema
 */
 
#ifndef __LIBC_H__
#define __LIBC_H__

#include <stats.h>

extern int errno;

int write(int fd, char *buffer, int size);

int read(char *b, int maxChar);

void itoa(int a, char *b);

int strlen(char *a);

int gotoxy(int x, int y);

int set_color(int fg, int bg);

void* shmat(int pos, void* addr);

void perror();

int getpid();

int fork();

void exit();

int yield();

int get_stats(int pid, struct stats *st);

#endif  /* __LIBC_H__ */
