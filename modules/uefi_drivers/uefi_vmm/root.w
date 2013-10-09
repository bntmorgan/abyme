\documentclass{article}
\usepackage{listings}
\usepackage{amsfonts}
\usepackage{hyperref}
\usepackage{graphicx}
\usepackage[all]{hypcap}
\usepackage[utf8]{inputenc}
\usepackage[francais]{babel}

\hypersetup{
  colorlinks,
  citecolor=black,
  filecolor=black,
  linkcolor=black,
  urlcolor=black
}

\title{SVC-hyperviseur documentation développeur}

\begin{document}

\maketitle 

\tableofcontents 

\def\figwidth{12cm}

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

Ainsi nous pourrons écrire facilement du code indépendant de la position de
chargement pour l'initialisation d'un AP.

Le code de l'initialisation de l'AP est chargé dans un zone inférieure au
premier méga par le BSP.

\subsection{Application processor}

Voici le schéma d'initlisation des APs par le BSP.

\begin{figure}[h]
  \centering
  \includegraphics[width=\figwidth]{figures/ap_init.pdf}
  \caption{Schéma initialisation des APs par le BSP}
  \label{fig:unix_memory}
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
       .long 0

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
  /*
   * Update data segment.
   */
  mov %cx, %ds
@-

Nous active

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
@+ gdt pm
  /*
   * We can't use the vmm gdt yet: we have not access to the upper memory.
   * We use our own gdt until we reach the protected mode.
   * Update the gdt pointer (we didn't know where we are in memory).
   */
  //lea gdt_start(%edx), %eax
  //mov %eax, (gdt_desc + 2)
  //lgdt gdt_desc
  /*
   * Go to protected mode.
   */
  //mov %cr0, %eax
  //or $0x1, %eax
  //mov %eax, %cr0
  /*
   * Again, jump using a long return.
   */
  //lea protected_mode(%edx), %ebx
  //pushw $0x08
  //push %bx
  //lret
@-
@+ pm
  protected_mode:
@-
@+ gdt lm
@-
@+ cr3 lm
@-
@+ lm
@-
@+ call vmm
@-

\end{document}
