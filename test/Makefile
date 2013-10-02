EFI_LDS 				:= efi.lds
EFI_CRT_OBJS 		:= crt0-efi.o
LD_FLAGS_ALL		:= -nostdlib -T $(EFI_LDS) -Bsymbolic $(EFI_CRT_OBJS) -znocombreloc --no-undefined
CC_FLAGS_ALL		:= -Wall -Werror -Werror -O2 -fno-stack-protector -fno-strict-aliasing -fshort-wchar $(EFI_INCLUDES) -fno-builtin -fno-PIC -O0
OBJCPY_FLAGS_TARGET	:= --target=efi-app-x86_64

all:
	@echo COMPILATION DU STEAK
	$(CC) $(CC_FLAGS_ALL) -c crt0-efi.S
	$(CC) $(CC_FLAGS_ALL) -c reloc.c 
	$(CC) $(CC_FLAGS_ALL) -c efiapi.c
	$(LD) $(LD_FLAGS_ALL) efiapi.o reloc.o -o efiapi.elf
	@objcopy -j .padding_begin -j .text -j .sdata -j .data \
		-j .dynamic -j .dynsym  -j .rel \
		-j .rela -j .reloc -j .padding_end \
		$(OBJCPY_FLAGS_TARGET) \
		efiapi.elf efiapi.efi
	cp efiapi.efi ../binary
