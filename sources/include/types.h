/*
Copyright (C) 2021  Benoît Morgan

This file is part of abyme

abyme is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

abyme is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with abyme.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __TYPES_H__
#define __TYPES_H__

#include <efi.h>

typedef unsigned long           size_t;

typedef uint64_t                uintptr_t;

#if 0

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

#endif
