cd \EFI\drivers\eric
echo Loading ERIC driver
echo press any key
# pause
load efi.efi
echo ERIC runtime driver loaded !
cd \EFI\drivers\82579LM
echo Loading ethernet driver
echo press any key
# pause
load efi.efi
echo Ethernet runtime driver loaded !
cd \EFI\drivers\vmm_rec_none
echo Loading recursive vmm
echo press any key
# pause
load efi.efi
echo Recursive vmm runtime driver loaded !
cd \EFI\drivers\vmm_rec_eric
echo Loading recursive vmm
echo press any key
# pause
load efi.efi
echo Recursive vmm runtime driver loaded !
