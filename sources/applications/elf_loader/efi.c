#include <efi.h>
#include <efilib.h>

#include "stdio.h"
#include "shell.h"
#include "string.h"
#include "debug.h"
#include "efiw.h"
#include "elf64.h"

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *st) {

  // Initialize gnuefi lib
  InitializeLib(image_handle, st);

  // Print to shell
  putc = &shell_print;

#ifdef _QEMU
  putc = &qemu_putc;

  qemu_send_address("elf_loader.efi");
#endif

  struct efi_loaded_image eli;

  if (efi_loaded_image_info(image_handle, &eli) !=
      EFI_SUCCESS) {
    Print(L"ZIZI\n");
    return EFI_NOT_FOUND;
  }

  Print(L"Command line option : %s, size 0x%x\n", eli.options,
      eli.options_size);

  int i;

  // This is null terminated, ignore last character
  for (i = 0; i < eli.options_size; i++) {
    if ((eli.options[i] & (CHAR16) 0x00ff) == ' ') {
      Print(L"Space found at index 0x%x\n", i);
      i++;
      break;
    }
  }

  if (i == eli.options_size) {
    Print(L"Missing filepath argument\n");
    return EFI_NOT_FOUND;
  } else {
    Print(L"Trying to load %s\n", &eli.options[i]);
  }

  EFI_STATUS status;
  EFI_GUID guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;

  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *protocol_file;

  // Get ethernet driver protocol interface
  status = LibLocateProtocol(&guid, (void **)&protocol_file);
  if (EFI_ERROR(status)) {
    INFO("Failed to locate simple file system protocol\n");
    return status;
  } else {
    printk("Protocol file 0x%X\n", protocol_file);
  }

  // Open current volume
  EFI_FILE_PROTOCOL *root = NULL, *file_elf = NULL;

  status = uefi_call_wrapper(protocol_file->OpenVolume, 2,
      protocol_file, &root);
  if (EFI_ERROR(status)) {
    INFO("Failed to locate open volume\n");
    return status;
  } else {
    INFO("Volume opened\n");
  }

  // Open target elf file
  status = uefi_call_wrapper(root->Open, 5,
      root, &file_elf, &eli.options[i], EFI_FILE_MODE_READ,
      EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
  if (EFI_ERROR(status)) {
    INFO("Failed to open file\n");
    return EFI_NOT_FOUND;
  } else {
    INFO("File opened\n");
  }

  // Read file to compute its total size
  uint64_t size = 0x100, total_size = 0;
  char buf[size];
  while (size == 0x100) {
    size = 0x100;
    INFO("Reading...\n");
    status = uefi_call_wrapper(file_elf->Read, 3, file_elf, &size, &buf[0]);
    if (EFI_ERROR(status)) {
      INFO("Failed to read file\n");
      return status;
    } else {
      INFO("File read 0x%x\n", size);
    }
    total_size += size;
  }

  INFO("Total file size 0x%08x\n", total_size);

  // Reset file position to zero to read the whole file
  status = uefi_call_wrapper(file_elf->SetPosition, 2, file_elf, 0);
  if (EFI_ERROR(status)) {
    INFO("Failed to set position of file\n");
    return status;
  } else {
    INFO("File position set\n");
  }

  // Allocate memory to hold file
  char *elf_file_buffer = efi_allocate_pool(total_size);
  if (elf_file_buffer == NULL) {
    INFO("Failed to allocate memory\n");
    return 1;
  }

  // Read the whole file
  size = total_size;
  status = uefi_call_wrapper(file_elf->Read, 3, file_elf, &size,
      &elf_file_buffer[0]);
  if (EFI_ERROR(status)) {
    INFO("Failed to read file\n");
    return status;
  } else {
    INFO("File read 0x%x\n", size);
  }

  // Check if file has been read up to total size
  if (size < total_size) {
    INFO("File read total file...\n");
    return 1;
  }

  // Close target elf file
  status = uefi_call_wrapper(file_elf->Close, 1, file_elf);
  if (EFI_ERROR(status)) {
    INFO("Failed to close file\n");
    return EFI_NOT_FOUND;
  } else {
    INFO("File closed\n");
  }

  // Load `static' program segments. Non dynamic (nopie, nopic) .text .bss .data
  // Sections are aligned to 4k
  Elf64_Ehdr *elf64_header = (Elf64_Ehdr *)&elf_file_buffer[0];
  for (i = 0; i < elf64_header->e_phnum; i++) {
    Elf64_Phdr *phdr = elf64_get_segment(elf64_header, i);
    INFO("Header[0x%x]\n", i);
    INFO("  type: 0x%x\n", phdr->p_type);
    INFO("  flags: 0x%x\n", phdr->p_flags);
    INFO("  offset: 0x%x\n", phdr->p_offset);
    INFO("  vaddr: 0x%x\n", phdr->p_vaddr);
    INFO("  paddr: 0x%x\n", phdr->p_paddr);
    INFO("  filesz: 0x%x\n", phdr->p_filesz);
    INFO("  memsz: 0x%x\n", phdr->p_memsz);
    INFO("  align: 0x%x\n", phdr->p_align);

    if (phdr->p_type != PT_LOAD) {
      INFO("Ignoring non PT_LOAD program header 0x%x\n", i);
      continue;
    }

    if (phdr->p_align != 0x1000) {
      INFO("Ignoring non 0x1000 aligned program header 0x%x\n", i);
      continue;
    }

    INFO("Loading program header 0x%x\n", i);

    // Allocate loading destination pages
    INFO("Allocate virtual memory...\n");
    uint64_t page_number = (phdr->p_memsz >> 12) +
      (((phdr->p_memsz & 0xfff) == 0) ? 0 : 1);
    INFO("Allocating 0x%016X pages for program segment @0x%016X\n", page_number,
        phdr->p_vaddr);
    // Select associated memory type
    uint8_t type;
    if (phdr->p_flags == 05) { // R X
      type = EfiBootServicesCode;
    } else if (phdr->p_flags == 06) { // RW
      type = EfiBootServicesData;
    } else if (phdr->p_flags == 04) { // R
      type = EfiBootServicesData;
    } else {
      INFO("Fatal error: failed to set memory type...\n");
      return 1;
    }
    INFO("EFI memory type is %d (3 rt code, 4 rt data)\n", type);
    char *location = efi_allocate_pages_at((void *)phdr->p_vaddr, page_number,
        type);
    if (location == NULL) {
      INFO("Failed to allocate loading destination memory\n");
      continue;
    }

    // Copy segment data or code
    uintptr_t src = ((uintptr_t)elf64_header + (uintptr_t)phdr->p_offset);
    INFO("Copy program segment @x%016X <- @x%016x, size 0x%x\n", location, src,
        phdr->p_filesz);
    memcpy(location, (char *)src, phdr->p_filesz);
  }

  // Initialize bss
  INFO("Initializing bss section...\n");
  elf64_init_bss(elf64_header);

  // Call entry point
  INFO("Entry point is @0x%016X\n", elf64_header->e_entry);
  void (*kernel_start)(void) = (void *)elf64_header->e_entry;
  INFO("Calling entry point...\n");
  kernel_start();
  INFO("Done !\n");

  // XXX maybe to remove
  INFO("Calling entry point again to check prepared execution state...\n");
  kernel_start();
  INFO("Done !\n");

  // Free file buffer memory
  efi_free_pool(elf_file_buffer);

  return EFI_SUCCESS;
}
