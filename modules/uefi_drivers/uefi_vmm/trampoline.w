\subsection{Application processor}

Voici le schéma d'initlisation des APs par le BSP (figure \ref{fig:ap_init}).

\begin{figure}[h]
  \centering
  \includegraphics[width=\figwidth]{figures/ap_init.pdf}
  \caption{Schéma initialisation des APs par le BSP}
  \label{fig:ap_init}
\end{figure}

Le code d'initialisation de l'AP est la routine assembleur suivante.

@++ trampo.s
trampoline_start:
.global trampoline_start

@< get address >
@< a20 >
@< gdt pm >
@< pm >
@< gdt lm >
@< cr3 lm >
@< lm >
@< call vmm >
@< globals >

trampoline_end:
.global trampoline_end
@-

Les globales trampoline\_start et trampoline\_end servant a marquer le début et
la fin de trampoline pour pouvoir le recopier dans une zone $<$ à 1 Mo (mode
réel).
Ce code s'apelle trampoline parcequ'il passe du mode réel au mode long en très
peu de temps.

Le BSP passe des paramètres à l'AP via un pointeur sur une structure de données
mise en place au préalable. La première étape consiste donc à
récupérer ce pointeur. Ensuite nous devons récupérer RIP de manière à pouvoir
nous adresser relativement à lui. Enfin pour pouvoir exécuter correctement
l'initialisation du coeur, nous devons modifier cs en mode réel pour que rip
soit virtuellement à zéro. Nous faisons la même chose avec le segment de
données, sinon le rip relative adressing ne marcherait pas.

@+ get address
.code16
start16:
  jmp start16_cont

start16_cont:
  /*
   * Set the stack. The vmm will change the $0x50 in this code with a different
   * value in order to be thread safe for multi-core (the stacks must not
   * overlap).
   */
  mov $0x50, %ax
  mov %ax, %ss
  mov $0xfe, %sp
  /*
   * Change cs:ip in order to have ip = 0 for the first byte of this code.
   * If this code is loaded at the physical address 0x1000, the address of
   * the first byte can be 0x100:0x0000 or 0x000:0x10000. We change into
   * 0x100:0x0000 because the compiler set the addresses relative to the first
   * byte of this binary (org directive).
   */
  xor %edx, %edx
  xor %ebx, %ebx
  call get_address
get_address:
  pop %bx
  /* ax = get_address */
  mov %bx, %ax
  /* edx == (cs << 4) */
  mov %cs, %dx
  shl $0x4, %edx
  /* edx == (cs << 4) + ip_xxx[get_address] */
  add %ebx, %edx
  /* edx == (cs << 4) + ip_xxx[get_address] - $get_address */
  sub %eax, %edx
  /* edx == (new_cs << 4) */
  /* ecx == new_cs */
  mov %edx, %ecx
  shr $0x4, %ecx
  /*
   * Simulate a long jump to get_address using long ret.
   */
  pushw %cx
  /* rip relative cs_adjusted address */
  add $(cs_adjusted - get_address), %ax
  pushw %ax
  lret
cs_adjusted:
  /*
   * Update data segment.
   */
  mov %cx, %ds
@-

Nous activons ensuite la gate a20

@+ a20
  /*
   * Enable a20.
   */
a20.1:
  in $0x64, %al
  test $2, %al
  jnz a20.1
  mov $0xd1, %al
  out %al, $0x64
a20.2:
  in $0x64, %al
  test $2, %al
  jnz a20.2
  mov $0xdf, %al
  out %al, $0x60
@-

Une fois que la gate A20 à été exécutée, nous pouvons tenter d'entrer en mode
protégé. Nous devons pour cela créer une zone mémoire d'adresse bien connue
contenant le pointeur de GDT. Le pointeur de GDT est déjà préparé par le BSP et
présent dans la structure AP param. Nous la recopions dans cette zone mémoire et
chargeons GDTR avec le contenu de celle-ci. Enfin nous pouvons modifier le CR0
et passer en mode protégé avec un long jump.
Il est important de noter que la GDT que nous chargeons pour le mode protégé
doit être telle que rip == 0 à l'addresse protected mode!!

@+ gdt pm
  /*
   * We can't use the vmm gdt yet: we have not access to the upper memory.
   * We use our own gdt until we reach the protected mode.
   * Update the gdt pointer (we didn't know where we are in memory).
   */
  /* Get ap param structure pointer rip is zero at cs_adjusted !*/
  movl $(param - cs_adjusted), %eax
  /* gdt size */
  movw (%eax), %bx
  movw %bx, gdtptr - cs_adjusted
  /* gdt address */
  movl 2(%eax), %ebx
  movl %ebx, gdtptr - cs_adjusted + 2
  /* We know that gdt is at 0x0 address cs => rip 0*/
  lgdt gdtptr - cs_adjusted
  /*
   * Go to protected mode.
   */
  mov %cr0, %eax
  or $0x1, %eax
  mov %eax, %cr0
  /*
   * Again, jump using a long return.
   */
  /* Get protected_mode address, cs_adjusted rip is zero !*/
  mov $(protected_mode - cs_adjusted), %ebx
  /* Prepare lret stack */
  pushw $0x8
  push %bx
  lret
@-

Pour commencer nous mettons à jour les selecteurs de segments de donnée et de
pile. Nous savons que notre rip == 0 à partir de protected mode.

@+ pm
protected_mode:
  /*
   * Update segment selectors.
   */
  mov $0x18, %ax
  mov %ax, %ds
  mov %ax, %ss
@-

Ensuite nous préparons la GDT lm de la même manière que pour la GDT pm.

@+ gdt lm
  /* gdt size */
  movw param - protected_mode + 6, %bx
  movw %bx, gdtptr - protected_mode
  /* gdt address */
  movl param - protected_mode + 8, %ebx
  movl %ebx, gdtptr - protected_mode + 2
  /* We know that gdt is at 0x0 address cs => rip 0*/
  lgdt gdtptr - protected_mode
@-

Une fois GDTR chargé, nous chargeons le CR3 pour le contexte de pagination mode
long. Ce contexte sera celui de l'hyperviseur (de manière à pouvoir sauter
diretement dans le code de celui-ci).

@+ cr3 lm
  /* CR3 */
  movl 12(%eax), %ebx
  movl %ebx, %cr3
@-

Il est temps d'enter en mode long en activant la pagination. après avoir faire
la requète d'entrée DANS IA32\_EFER. Nous devons pas oublier d'activer PAE
(cr4.PAE).

@+ lm
  /*
   * Request long mode (setup the EFER MSR).
   */
  mov $0xc0000080, %ecx
  rdmsr
  or $0x00000100, %eax
  wrmsr
  /*
   * Enable paging (and so long mode)
   */
  mov %cr0, %ebx
  or $0x80000000, %ebx
  mov %ebx, %cr0
  /*
   * Jump into 64 mode using far ret.
   * TODO: adjust 0x10 (depend on gdt).
   */
  /* Get protected_mode address */
  pushl $(start64 - protected_mode)
  pushl %ebx
  lret
@-

Mettons en place les segments lm. Nous n'avons plus de gdt telle que rip == à
start64. Nous devons retrouver la structure param de manière relative à rip.

@+ call vmm
.code64
start64:
  mov $0x20, %ax
  mov %ax, %ds
  mov %ax, %es
  mov %ax, %gs
  mov %ax, %fs
  mov %ax, %ss
  mov %ax, %ss
  callq get_address_lm
get_address_lm:
  pop %rax
  /* Get ap param pointer */
  add $(param - start64), %rax
  /* Get vmm_next address */
  movq 16(%eax), %rbx
  /* jumps into the vmm_next function ! */
  callq *%rbx
end:
  jmp end
@-

@++ globals
param: 
  /* AP params */
  .long 0x90909090
gdtptr:
  /* GDT pointer */
  .long 0x90909090
  .long 0x90909090
@-

