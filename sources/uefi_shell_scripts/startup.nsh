echo changing terminal mode
mode 100 31
echo Startup !
fs0:
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
cd \EFI\svc_grub
echo Launching grub
echo press any key
pause
grubx64.efi
