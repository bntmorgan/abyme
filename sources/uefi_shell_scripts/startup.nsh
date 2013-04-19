echo changing terminal mode
mode 100 31
echo Startup !
fs0:
cd EFI
cd uefi_drivers
cd uefi_vmm
echo press any key
pause
load efi.efi
echo Vmm runtime driver loaded !
fs0:
cd ..
cd ..
cd svc_grub
echo Launching grub
echo press any key
pause
grubx64.efi
