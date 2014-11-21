cd \EFI\uefi_drivers\pcie_root_port
echo Loading pcie_root_port driver
echo press any key
pause
load efi.efi
echo pcie_root_port runtime driver loaded !
cd \EFI\uefi_drivers\eric
echo Loading ethernet driver
echo press any key
pause
load efi.efi
echo ERIC runtime driver loaded !
cd \EFI\uefi_drivers\flash
echo Loading flash driver
echo press any key
pause
load efi.efi
echo FLash runtime driver loaded !
cd \EFI\uefi_drivers\82579LM
echo Loading ethernet driver
echo press any key
pause
load efi.efi
echo Ethernet runtime driver loaded !
# cd \EFI\uefi_drivers\uefi_vmm_supervisor
# echo Loading vmm
# echo press any key
# pause
# load efi.efi
# echo Vmm supervisor runtime driver loaded !
cd \EFI\uefi_drivers\uefi_vmm
echo Loading vmm
echo press any key
pause
load efi.efi
echo Vmm runtime driver loaded !
