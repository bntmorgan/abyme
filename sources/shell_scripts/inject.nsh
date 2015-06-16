cd \EFI\drivers\82579LM
echo Loading ethernet driver
echo press any key
# pause
load efi.efi
echo Ethernet runtime driver loaded !
cd \EFI\drivers\inject
echo Loading Inject runtime driver
echo press any key
# pause
load efi.efi
echo Inject runtime driver loaded !
