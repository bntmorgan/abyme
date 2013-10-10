\section{Gestion du multi-cœur}

Cette section décrit la séquence d'initialisation des coœurs sans entrer dans le
détail de l'initialisation de la virtualisation.

\subsection{Bootsrap processor}

Une fois que le VMM du bootstrap processor (BSP) c'est installé correctement, il
va activer un par un, en série les application processors (AP), qui vont
exécuter un séquence d'initialisation particulière, activation la virtualisation
et installer le VMM. Nous n'entrerons pas dans les détails de l'activation de la
virtualisation et de l'installation du VMM ici.

Le BSP va préparer des structures concernant tous les AP et lui-même contenant
les infos suivantes :

\begin{itemize}
  \item Localisation de la pile;
  \item Localisation de la section données;
  \item Le pointeur de GDT protégé;
  \item La GDT mode protégé;
  \item La GDT utilisée par le BSP;
  \item Le CR3 mode long;
  \item Autres ???;
\end{itemize}

Un AP s'exécute en mode réel lorsqu'il est activé. Nous avons donc écrit un
petit bout de code (trampoline) chargé de passé en mode protégé, puis long,
d'exécuter du code de l'hyperviseur qui installera la virtualisation et
haltera le coeur jusqu'à son utilisation par l'OS.

Les informations de configuration du coeur seront donnée par le BSP à l'AP qu'il
active. dans une structure de la forme suivante :

\begin{itemize}
  \item Le pointeur de GDT protégé;
  \item Le pointeur de GDT long;
  \item Le CR3 long;
  \item Le pointeur de foction 64 AMD-64 ABI pour l'exécution de la suite;
\end{itemize}

@+ smp ap param
struct ap_param{
  uint16_t gdt_pm_size;
  uint32_t gdt_pm_start;
  uint16_t gdt_lm_size;
  uint32_t gdt_lm_start;
  uint32_t cr3_lm;
  uint64_t vmm_next;
} __attribute__((packed));
@-

Ainsi nous pourrons écrire facilement du code indépendant de la position de
chargement pour l'initialisation d'un AP.

Le code de l'initialisation de l'AP est chargé dans un zone inférieure au
premier méga par le BSP.

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
@< get bsp parameters >
@< a20 >
@< gdt pm >
@< pm >
@< gdt lm >
@< cr3 lm >
@< lm >
@< call vmm >

trampoline_end:
.global trampoline_end
@-

Les glonales trampoline\_start et trampoline\_end servant a marquer le début et
la fin de trampoline pour pouvoir le recopier dans une zone < à 1 Mo (mode
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

param: .long 0

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
  test %ax, %ax
  jnz cs_adjusted 
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
  pushw %ax
  lret
cs_adjusted:
  /* GDT pointer */
  .long 0x90909090
  .long 0x90909090
  /*
   * Update data segment.
   */
  mov %cx, %ds
@-

Nous allons récupérer l'adresse de la structure params et l'empiler pour pouvoir
l'utiliser plus tard. nous allons aussi empiler l'adresse de get\_address, qui
pourra nous servir pour la suite.

@+ get bsp parameters
  calll bsp_parameters
  bsp_parameters:
  pop %ebx
  mov %ebx, %eax
  sub $(bsp_parameters - param), %eax
  /* BSP parameters label address */
  pushl %ebx
  /* Params label address */
  pushl %eax
@-

Ce qui nous donne la stack représentée par la figure
\ref{fig:stak_trampo_bsp_params}.

\begin{figure}[h]
  \centering
  \includegraphics[width=\figwidth]{figures/stak_trampo_bsp_params.pdf}
  \caption{Pile APs après récupération des paramètres}
  \label{fig:stak_trampo_bsp_params}
\end{figure}

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

@+ gdt pm
  /*
   * We can't use the vmm gdt yet: we have not access to the upper memory.
   * We use our own gdt until we reach the protected mode.
   * Update the gdt pointer (we didn't know where we are in memory).
   */
  /* Get ap param structure pointer */
  movl (%esp), %eax
  /* gdt size */
  movw (%eax), %bx
  mov %bx, 0x0
  /* gdt address */
  movl 2(%eax), %ebx
  movl %ebx, 0x0
  /* We know that gdt is at 0x0 address cs => rip 0*/
  lgdt 0x0
  /*
   * Go to protected mode.
   */
  mov %cr0, %eax
  or $0x1, %eax
  mov %eax, %cr0
  /*
   * Again, jump using a long return.
   */
  /* Get cs_adjusted rip */
  movl 2(%esp), %eax
  /* Get protected_mode address */
  add $(protected_mode - bsp_parameters), %ebx
  /* Prepare lret stack */
  pushw $0x8
  push %bx
  lret
@-

Pour commencer nous metto sà jour les selecteurs de segments de donnée et de
pile.

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
  /* Get ap param structure pointer */
  movl (%esp), %eax
  /* gdt size */
  movw 6(%eax), %bx
  mov %bx, 0x0
  /* gdt address */
  movl 8(%eax), %ebx
  movl %ebx, 0x0
  /* We know that gdt is at 0x0 address cs => rip 0*/
  lgdt 0x0
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
  /* Get cs_adjusted rip */
  movl 2(%esp), %eax
  /* Get protected_mode address */
  add $(start64 - bsp_parameters), %ebx
  pushl $0x10
  pushl %ebx
  lret
@-

Sets the lm segments, jump into the VMM code and that's it !

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
  /* Get ap param pointer */
  movl (%esp), %eax
  /* Get vmm_next address */
  movq 16(%eax), %rbx
  /* jumps into the vmm_next function ! */
  callq *%rbx
end:
  jmp end
@-

\section{Fichiers}

@++ smp.h
#ifndef __SMP_H__
#define __SMP_H__
#include <efi.h>

@< smp ap param >

#endif//__SMP_H__
@-
