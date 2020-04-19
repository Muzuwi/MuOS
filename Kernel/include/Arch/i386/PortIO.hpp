#ifndef PORTIO_KERNEL_H
#define PORTIO_KERNEL_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void out(uint16_t, uint8_t);
void outw(uint16_t port, uint16_t value);
void outd(uint16_t, uint32_t);

uint8_t in(uint16_t);
uint16_t inw(uint16_t);
uint32_t ind(uint16_t);

void lgdt(uint32_t);

#ifdef __cplusplus
}
#endif


#endif
