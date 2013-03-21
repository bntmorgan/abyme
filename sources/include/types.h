#ifndef __TYPES_H__
#define __TYPES_H__

typedef unsigned long           size_t;

typedef char                    int8_t;
typedef short                   int16_t;
typedef int                     int32_t;
typedef long long int           int64_t;

typedef unsigned char           uint8_t;
typedef unsigned short int      uint16_t;
typedef unsigned int            uint32_t;
typedef unsigned long long int  uint64_t;

typedef signed char             sint8_t;
typedef signed long long int    sint64_t;

#ifdef __X86_64__
typedef long int                intptr_t;
typedef unsigned long int       uintptr_t;
#else
typedef int                     intptr_t;
typedef unsigned int            uintptr_t;
#endif

#endif
