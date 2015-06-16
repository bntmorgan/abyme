cd \EFI\drivers\flash
echo Loading flash driver
echo press any key
pause
load efi.efi
echo FLash runtime driver loaded !
pause
cd \EFI\drivers\82579LM
echo Loading ethernet driver
echo press any key
pause
load efi.efi
echo Ethernet runtime driver loaded !
cd \EFI\drivers\uefi_vmm
echo Loading vmm
echo press any key
pause
load efi.efi
echo Vmm runtime driver loaded !
