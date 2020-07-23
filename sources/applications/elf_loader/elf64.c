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

void elf64_load_segment(Elf64_Ehdr *elf64_header, uint8_t *destination) {
  Elf64_Phdr *program_header;
  Elf64_Shdr *bss_header;
  uint8_t *source;

  program_header = (Elf64_Phdr *) (((uintptr_t) elf64_header) +
      ((uintptr_t) elf64_header->e_phoff));
  if ((elf64_header->e_machine != EM_X86_64) ||
      (!elf64_header->e_phnum) || (program_header->p_type != PT_LOAD)) {
    ERROR("invalid module");
  }
  source = (uint8_t *) (((uintptr_t) elf64_header) +
      ((uintptr_t)program_header->p_offset));
  /*
   * TODO: we must check if the destination overlap other information (loader,
   *       multiboot, etc.).
   */
  memcpy(destination, source, program_header->p_filesz);
  if (program_header->p_memsz > program_header->p_filesz) {
    memset(destination + program_header->p_filesz, 0,
        program_header->p_memsz - program_header->p_filesz);
  }
  bss_header = elf64_get_section(elf64_header, SHT_NOBITS);
  if (bss_header != (Elf64_Shdr *) 0) {
    memset(destination + bss_header->sh_addr, 0, bss_header->sh_size);
  }
}

void elf64_load_relocatable_segment(void *header, void *destination) {
  Elf64_Ehdr *elf64_header;
  Elf64_Shdr *relocation_header;
  Elf64_Rela *relocation;
  Elf64_Xword i;
  uintptr_t relocation_address;
  uintptr_t relocation_address_end;
  uint64_t base_address = (uint64_t) (uintptr_t) destination;

  elf64_header = (Elf64_Ehdr *) header;
  elf64_load_segment(elf64_header, destination);
  relocation_header = elf64_get_section(elf64_header, SHT_RELA);
  if (relocation_header == (Elf64_Shdr *) 0) {
    ERROR("empty relocation section");
  }
  relocation_address = ((uintptr_t) header) + relocation_header->sh_offset;
  relocation_address_end = relocation_address + relocation_header->sh_size;
  for (i = 0; relocation_address < relocation_address_end; i++) {
    relocation = (Elf64_Rela *) relocation_address;
    if ((ELF64_R_TYPE(relocation->r_info) != R_X86_64_RELATIVE) ||
        (relocation->r_addend < 0)) {
      ERROR("bad relocation entry %x", ELF64_R_TYPE(relocation->r_info));
    }
    *((uint64_t *) (destination + relocation->r_offset)) =
        base_address + relocation->r_addend;
    relocation_address = relocation_address + relocation_header->sh_entsize;
  }
}

uint64_t elf64_get_entry(void *header) {
  Elf64_Ehdr *elf64_header;
  elf64_header = (Elf64_Ehdr *) header;
  return (uint64_t) elf64_header->e_entry;
}

uint64_t elf64_get_alignment(void *header) {
  Elf64_Ehdr *elf64_header;
  Elf64_Phdr *program_header;
  elf64_header = (Elf64_Ehdr *) header;
  program_header = (Elf64_Phdr *) (((uintptr_t) elf64_header) +
      ((uintptr_t) elf64_header->e_phoff));
  return (uint64_t) program_header->p_align;
}

uint64_t elf64_get_size(void *header) {
  Elf64_Ehdr *elf64_header;
  Elf64_Phdr *program_header;
  elf64_header = (Elf64_Ehdr *) header;
  program_header = (Elf64_Phdr *) (((uintptr_t) elf64_header) +
      ((uintptr_t) elf64_header->e_phoff));
  return (uint64_t) program_header->p_memsz;
}
