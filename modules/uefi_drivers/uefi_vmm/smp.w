\subsection{Bootsrap processor}

Une fois que le VMM du bootstrap processor (BSP) c'est installé correctement, il
va activer un par un, en série les application processors (AP), qui vont
exécuter une séquence d'initialisation particulière, activation la virtualisation
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
petit bout de code (trampoline) chargé de passer en mode protégé, puis long,
d'exécuter du code de l'hyperviseur qui installera la virtualisation et
haltera le coeur jusqu'à son utilisation par l'OS.

Les informations de configuration du coeur seront donnée par le BSP à l'AP qu'il
active. dans une structure de la forme suivante :

\begin{itemize}
  \item Le pointeur de GDT protégé;
  \item Le pointeur de GDT long;
  \item Le CR3 long;
  \item Le pointeur de fonction 64 AMD-64 ABI pour l'exécution de la suite;
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
