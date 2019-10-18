#ifndef PORTIO_KERNEL_H
#define PORTIO_KERNEL_H

#include <stddef.h>
#include <stdint.h>

extern "C" void out(uint16_t, uint8_t);
extern "C" uint8_t in(uint16_t);
extern "C" void lgdt(uint32_t);

#endif