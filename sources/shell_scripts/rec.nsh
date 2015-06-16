cd \EFI\drivers\82579LM
echo Loading ethernet driver
echo press any key
# pause
load efi.efi
echo Ethernet runtime driver loaded !
cd \EFI\drivers\vmm_rec_0
echo Loading recursive vmm
echo press any key
# pause
load efi.efi
echo Recursive vmm runtime driver loaded !
echo Eric runtime driver loaded !
cd \EFI\drivers\vmm_rec_1
echo Loading recursive vmm
echo press any key
# pause
load efi.efi
echo Recursive vmm runtime driver again loaded !
echo Eric runtime driver loaded !
cd \EFI\drivers\vmm_rec_2
echo Loading recursive vmm
echo press any key
# pause
load efi.efi
echo Recursive vmm runtime driver again again loaded !
