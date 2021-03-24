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

/*
 * See [Elf64_Version1.5_Draft2].
 */

#include "elf64.h"

#include "string.h"
#include "stdio.h"

Elf64_Shdr *elf64_get_section(Elf64_Ehdr *elf64_header,
    Elf64_Word section_type) {
  Elf64_Shdr *section_header;
  Elf64_Half i;
  uintptr_t address;

  address = ((uintptr_t) elf64_header) + ((uintptr_t) elf64_header->e_shoff);
  for (i = 0; i < elf64_header->e_shnum; i++) {
    section_header = (Elf64_Shdr *) address;
    if (section_header->sh_type == section_type) {
       return section_header;
    }
    address += (uint64_t) elf64_header->e_shentsize;
  }
  return (Elf64_Shdr *) 0;
}

Elf64_Phdr *elf64_get_segment(Elf64_Ehdr *elf64_header, uint32_t index) {
  Elf64_Phdr *section_header;

  if (index > elf64_header->e_phnum) {
    return NULL;
  }

  section_header = (Elf64_Phdr *) (((uintptr_t) elf64_header) +
      ((uintptr_t) elf64_header->e_phoff));

  return &section_header[index];
}

void elf64_init_bss(Elf64_Ehdr *elf64_header) {
  Elf64_Shdr *bss_header;

  bss_header = elf64_get_section(elf64_header, SHT_NOBITS);
  if (bss_header != (Elf64_Shdr *) 0) {
    memset((void *)bss_header->sh_addr, 0, bss_header->sh_size);
  }

}
