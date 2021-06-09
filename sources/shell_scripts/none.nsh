# unload 88
# unload 89
cd \EFI\applications\hello_world
echo Loading Hello world
echo press any key
# pause
hello_world.efi
echo Hello world executed !
cd \EFI\drivers\82579LM
echo Loading ethernet driver
echo press any key
# pause
load 82579LM.efi
echo Ethernet runtime driver loaded !
cd \EFI\drivers\vmm_rec_none
echo Loading recursive vmm
echo press any key
# pause
load vmm_rec_none.efi
echo Recursive vmm runtime driver loaded !
pause
