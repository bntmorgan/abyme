cd \EFI\uefi_drivers\82579LM
echo Loading ethernet driver
echo press any key
pause
load efi.efi
echo Ethernet runtime driver loaded !
cd \EFI\uefi_drivers\uefi_vmm
echo Loading vmm
echo press any key
pause
load efi.efi
echo Vmm runtime driver loaded !
