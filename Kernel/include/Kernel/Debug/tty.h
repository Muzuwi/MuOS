#ifndef KERNEL_TTY_H
#define KERNEL_TTY_H

#ifdef __cplusplus
extern "C" {
#endif

void tty_init();
void tty_prints(const char* data);
void tty_printch(char data);

#ifdef __cplusplus
}
#endif

#endif