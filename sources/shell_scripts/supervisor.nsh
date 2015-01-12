cd \EFI\drivers\82579LM
echo Loading ethernet driver
echo press any key
pause
load efi.efi
echo Ethernet runtime driver loaded !
cd \EFI\drivers\vmm_supervisor
echo Loading recursive vmm
echo press any key
pause
load efi.efi
echo Recursive vmm runtime driver loaded !
# cd \EFI\drivers\vmm_supervisor
# echo Loading recursive vmm
# echo press any key
# pause
# load efi.efi
# echo Recursive vmm runtime driver again loaded !
cd \
echo Loading recursive vmm
echo press any key
pause
load vmm_supervisor.efi
echo Recursive vmm runtime driver loaded !
