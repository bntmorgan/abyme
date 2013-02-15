# Generated with make config.py script : DO NOT MODIFY
ui menu.c32
menu title Boot Menu

${for k in pm_kernels :}$
  label protected_$[k[0]]$_$[k[1]]$
  menu label $[k[0]]$ $[k[1]]$ without tinyvisor
  kernel mboot.c32
  append $[boot_directory]$/pm_kernels/$[k[0]]$/$[k[1]]$
${#end}$

${for k in pm_kernels :}$
  label tinyvisor_$[k[0]]$_$[k[1]]$
  menu label $[k[0]]$ $[k[1]]$ with tinyvisor
  kernel mboot.c32
  append $[boot_directory]$/loader/loder.bin --- $[boot_directory]$/vmm/vmm.bin --- $[boot_directory]$/pm_kernels/$[k[0]]$/$[k[1]]$
${#end}$
