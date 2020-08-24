cd \EFI\applications\elf_loader
echo Running elf loader
echo press any key
pause
elf_loader.efi \EFI\smm_stage_2\smm_stage_2.elf
echo Elf loader executed !
echo press any key
pause
