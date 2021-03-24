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

struct list_head {
  struct list_head *next;
};

#define STATIC_LINKED_LIST_INIT(mem, type, field, size) \
  do {                                                  \
    type *elements = (type *) mem;                      \
    uint32_t nb_elements = size / sizeof(type);         \
    uint32_t i;                                         \
    for (i = 0; i < nb_elements; i++) {                 \
      elements[i].field.next = &elements[i + 1].field;  \
    }                                                   \
    elements[nb_elements - 1].field.next = NULL;        \
  } while (0);
