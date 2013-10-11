\section{Gestion du multi-cœur}

Cette section décrit la séquence d'initialisation des coœurs sans entrer dans le
détail de l'initialisation de la virtualisation.

@i smp.w

@i trampoline.w

\section{Fichiers}

@++ smp.h
#ifndef __SMP_H__
#define __SMP_H__
#include <efi.h>

@< smp ap param >

#endif//__SMP_H__
@-
