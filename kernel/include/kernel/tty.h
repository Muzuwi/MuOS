#ifndef KERNEL_TTY_H
#define KERNEL_TTY_H

void tty_init();
void tty_prints(const char* data);
void tty_printch(char data);

#endif