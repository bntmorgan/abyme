# cd \EFI\drivers\82579LM
# echo Loading ethernet driver
# echo press any key
# # pause
# load efi.efi
# echo Ethernet runtime driver loaded !
# cd \EFI\drivers\vmm_rec_none
# echo Loading recursive vmm
# echo press any key
# # pause
# load efi.efi
# echo Recursive vmm runtime driver loaded !
cd \EFI\drivers\pcie_root_port
echo Loading pcie root port driver
echo press any key
# pause
load efi.efi
echo loaded !
cd \EFI\drivers\eric
echo Loading eric driver
echo press any key
pause
load efi.efi
echo loaded !

