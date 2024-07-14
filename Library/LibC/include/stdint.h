#ifndef __LIBC_TIME_H
#define __LIBC_TIME_H

typedef unsigned char __u_char;
typedef unsigned short int __u_short;
typedef unsigned int __u_int;
typedef unsigned long int __u_long;

typedef signed char __int8_t;
typedef unsigned char __uint8_t;
typedef signed short int __int16_t;
typedef unsigned short int __uint16_t;
typedef signed int __int32_t;
typedef unsigned int __uint32_t;
typedef signed long int __int64_t;
typedef unsigned long int __uint64_t;

/*
 *  Exact-width integer types
 */
typedef __int8_t int8_t;
typedef __int16_t int16_t;
typedef __int32_t int32_t;
typedef __int64_t int64_t;

typedef __uint8_t uint8_t;
typedef __uint16_t uint16_t;
typedef __uint32_t uint32_t;
typedef __uint64_t uint64_t;

/*
 *  Minimum-width integer types
 */
typedef __uint8_t uint_least8_t;
typedef __uint16_t uint_least16_t;
typedef __uint32_t uint_least32_t;
typedef __uint64_t uint_least64_t;

typedef __int8_t int_least8_t;
typedef __int16_t int_least16_t;
typedef __int32_t int_least32_t;
typedef __int64_t int_least64_t;

/*
 *  "Fastest" minimum-width integer types
 */
typedef __uint8_t uint_fast8_t;
typedef __uint64_t uint_fast16_t;
typedef __uint64_t uint_fast32_t;
typedef __uint64_t uint_fast64_t;

typedef __int8_t int_fast8_t;
typedef __int64_t int_fast16_t;
typedef __int64_t int_fast32_t;
typedef __int64_t int_fast64_t;

/*
 *  Integer types capable of holding object pointers
 */
typedef long int intptr_t;
typedef unsigned long int uintptr_t;

#endif
