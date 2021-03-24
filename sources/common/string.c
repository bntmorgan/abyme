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

#include "string.h"

// void memset(void *dst, uint8_t c, size_t size) {
//   __asm__ __volatile__(
//       "pushf     ;"
//       "cld       ;"
//       "rep stosb ;"
//       "popf      ;"
//     : : "D"(dst), "a"(c), "c"(size));
// }
// 
// void memcpy(void *dst, void *src, size_t size) {
//   __asm__ __volatile__(
//       "pushf     ;"
//       "cld       ;"
//       "rep movsb ;"
//       "popf      ;"
//     : : "D"(dst), "S"(src), "c"(size));
// }

unsigned int strlen(char *s) {
  unsigned int size = 0, i;
  for (i = 0; s[i] != '\0'; ++i) {
    ++size;
  }
  return size;
}
