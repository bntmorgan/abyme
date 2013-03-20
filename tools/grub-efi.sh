sudo grub-install --target=x86_64-efi --efi-directory=/mnt --bootloader-id=svc_grub --boot-directory=/mnt/EFI --recheck --debug
sudo grub-mkconfig -o /mnt/EFI/grub/grug.cfg
