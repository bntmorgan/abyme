\subsection{Bootsrap processor}

\subsubsection{introduction}

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
struct ap_param {
  // 0x00
  struct gdt_ptr_32 gdt_ptr_pm;
  // 0x06
  struct gdt_ptr_64 gdt_ptr_lm;
  // 0x10
  uint32_t cr3_lm;
  // 0x14
  uint64_t vmm_next;
} __attribute__((packed));
@-

Ainsi nous pourrons écrire facilement du code indépendant de la position de
chargement pour l'initialisation d'un AP.

Le code de l'initialisation de l'AP est chargé dans un zone inférieure au
premier méga par le BSP.

\subsubsection{Chargement du code d'initialisation d'un AP}

Dans l'image binaire UEFI de notre VMM, nous avons inclus un bout de code PIC
correspondant à la séquence d'initialisation d'un coeur, le trampoline. Cette séquence doit
être recopiée en mémoire dans un espace $<$ au premier méga octet, un cœur se
démarrant en mode réel. 

Pour identifier le code trampoline, nous avons crée deux variables globales,
donts les adresses correspondent respectivement au débutr et à la fin de
cette section de code.

\begin{itemize}
\item trampoline\_start;
\item trampoline\_end;
\end{itemize}

Avant de copier le code de ce trampoline, il faut s'assurer d'avoir une zone
mémoire réservée à cet effet. Nous nous appuyons sur les services offerts par
UEFI pour cette allocation de mémoire. Grâce au bootservice AllocatePages, nous
ponvons demander au firmware de nous réserver des pages dans une zone inférieure
au premier méga, mais aussi de type BootService. Ce qui signifie que quand le
firwmare aura exécuté l'appel transitif, ExitBootServices, entre le firwmare et
l'OS, cette zone mémoir ene sera plus marquée comme réservé et pourra être
utilisée par le système d'exploitation, ce qui nous convient parfaitement.

@+ smp allocate trampoline
uint8_t smp_allocate_trampoline() {
  EFI_PHYSICAL_ADDRESS *address = (EFI_PHYSICAL_ADDRESS *)0x100000;
  uint32_t pages = (&trampoline_end - &trampoline_start) / 0x1000 +
    (((&trampoline_end - &trampoline_start) * 1. / 0x1000 > 0) ? 1 : 0 );
  INFO("Number of pages to allocate for trampoline 0x%x\n", pages);
  uint32_t code = uefi_call_wrapper(systab->BootServices->AllocatePages, 4
      AllocateMaxAddress, EfiBootServicesCode, pages, address);
  // XXX ULTRA DIRTY
  // http://wiki.osdev.org/Memory_Map_(x86)
  address = (EFI_PHYSICAL_ADDRESS *)0x80000;
  code = 0;
  if(code != EFI_SUCCESS) {
    INFO("Failed to allocate memory for trampoline 0x%x, address 0x%x, type\
        0x%x, memory_type 0x%x, max_memory_type 0x%x\n", code, address,
        AllocateMaxAddress, EfiBootServicesData, EfiMaxMemoryType);
    return code;
  } else {
    INFO("Allocated address 0x%x\n", address);
    ap_trampoline_start = address;
  }
  return 0;
}
@-

Nous devons allouer une GDT disponible dans l'espace d'adressage 16-bit pour le
mode protégé, et une GDT disponible dans l'espace 32-bit pour le mode long.

@+ smp allocate gdt pm
uint8_t *ap_gdt_pm;

uint8_t smp_allocate_gdt_pm() {
  ap_gdt_pm = (uint8_t *)0x90000;
  return 0;
}
@-

@+ smp allocate gdt lm
uint8_t *ap_gdt_lm;

uint8_t smp_allocate_gdt_lm() {
  ap_gdt_lm = (void *)0x91000;
  return 0;
}
@-

\subsubsection{Préparation des paramètres}

Nous devosn préparer les paramètres du trampoline pour l'activation des APs.
Nous verrons plus tard qu'il nous faut créer une GDT telle que ds et cs
pointent vers des descripteurs de segments tels que rip \symbol{64} protected\_mode. Nous
devons donc tout simplement prévoir une adresse de base pour ces descripteurs
tel que adresse logique : $$ \texttt{adresse logique 0} = \texttt{adresse physique de chargement} +
\texttt{protected mode} $$

@+ smp create ap gdt pm
#define SMP_AP_GDT_PM_SIZE 0x24
extern uint8_t protected_mode;

void smp_create_ap_gdt_pm() {
  struct gdt_entry e;
  struct ap_param *p = (struct ap_param *)&ap_param;
  INFO("ap_param address: 0x%x, value 0x%x\n", p, ap_param);
  // Create GDT ptr
  p->gdt_ptr_pm.base = (uint32_t)(uint64_t)ap_gdt_pm;
  p->gdt_ptr_pm.limit = SMP_AP_GDT_PM_SIZE;
  INFO("ap_get_pm adress: 0x%x\n", ap_gdt_pm);
  // Initialize GDT
  memset(ap_gdt_pm, 0, SMP_AP_GDT_PM_SIZE);
  // Entry 0 is null
  // We compute the base of the segments regarding physical loading address and
  // protected\_mode adress
  e.base = (uint32_t)((uint64_t)&protected_mode + (uint64_t)ap_trampoline_start);
  INFO("Computed base of mp segments : 0x%x\n", e.base);
  e.limit = 0xffffffff;
  // Entry 1 is for code
  // G, D
  e.granularity = 0xc0;
  // P, DPL = 0, not system, r-x
  e.access = 0x96;
  gdt_copy_desc(&e, ap_gdt_pm + 0x8);
  printk("Code 0x%X\n", *((uint64_t *)(ap_gdt_pm + 0x8)));
  printk_bin(8, " ", ap_gdt_pm + 0x8);
  // Entry 2 is for data
  // G, D
  e.granularity = 0xc0;
  // P, DPL = 0, not system, r-x
  e.access = 0x92;
  gdt_copy_desc(&e, ap_gdt_pm + 0x10);
  printk("Data 0x%X\n", *((uint64_t *)(ap_gdt_pm + 0x10)));
  printk_bin(8, " ", ap_gdt_pm + 0x10);
}
@-

Nous disposons déjà d'une gdt correcte pour l'exécution courante de l'UEFI, nous
allons l'utiliser.

@+ smp create ap gdt lm
void smp_create_ap_gdt_lm() {
  struct ap_param *p = (struct ap_param *)&ap_param;
  // Create GDT ptr
  p->gdt_ptr_lm.base = (uint64_t)ap_gdt_lm;
  p->gdt_ptr_lm.limit = gdt_get_host_limit();
  // Copy the gdt
  memcpy(ap_gdt_lm, (uint8_t *)gdt_get_host_base(), p->gdt_ptr_lm.limit);
}
@-

Nous devons ensuite copier trampoline dans la mémoire allouée.

@+ smp install trampoline
void smp_install_trampoline() {
  INFO("Trampoline installation : 0x%x (0x%x octets) -> 0x%x\n", &trampoline_start,
      &trampoline_end - &trampoline_start, ap_trampoline_start);
  memcpy(ap_trampoline_start, &trampoline_start, &trampoline_end -
      &trampoline_start);
}
@-

\subsubsection{Configuration et activation du multicœur}

Le firmware d'un machine donne la configuration du multicœur via un jeu de
structures spéficiées par Intel en 1997. Le code écrit dans ce cahpitre est tiré de
cette spécification : "Intel MultiProcessor Specification v1.4, 1997".

Cette configuration est donc écrite en mémoire par le firwmare avant de donner
la main à l'OS. La table principale, point d'entrée de cette spécification, est
la Floating Pointer Structure.

@+ floating point structure
/*
 * See [Multiprocessor_Version1.4_May_1997], section 4.1.
 */
typedef struct {
  uint32_t signature;
  uint32_t physical_address_pointer;
  uint8_t length;
  uint8_t spec_rev;
  uint8_t checksum;
  uint8_t features1;
  uint8_t features2;
} __attribute__((packed)) smp_mp_floating_pointer_structure_t;
@-

Cette structure donne globalement l'adresse d'une table de configuration du
multicœur, la MP Configuration Table. Le champ features1 est important car il
indique s'il y a ou pas de table MP.

@+ mp configuration table
typedef struct {
  uint32_t signature;
  uint16_t base_table_length;
  uint8_t spec_rev;
  uint8_t checksum;
  uint8_t oemid[8];
  uint8_t prodid[12];
  uint32_t oem_table_pointer;
  uint16_t oem_table_size;
  uint16_t entry_count;
  uint32_t memory_mapped_address_of_local_apic;
  uint16_t extended_table_length;
  uint8_t extended_table_checksum;
  uint8_t reserved;
} __attribute__((packed)) smp_mp_configuration_table_header_t;
@-

La table MP contient 5 types d'entrées différentes : 
\begin{itemize}
\item Processor entry : 0;
\item Bus entry : 1;
\item IO apic entry : 2;
\item IO interrupt : 3;
\item local interrupt : 4;
\end{itemize}

@+ mp configuration table entries
/*
 * See [Multiprocessor_Version1.4_May_1997], section 4.3.1.
 */
typedef struct {
  uint8_t entry_type;
  uint8_t local_apic_id;
  uint8_t local_apic_version;
  uint8_t cpu_flags;
  uint32_t cpu_signature;
  uint32_t feature_flags;
  uint32_t reserved_1;
  uint32_t reserved_2;
} __attribute__((packed)) smp_mp_processor_entry_t;

/*
 * See [Multiprocessor_Version1.4_May_1997], section 4.3.2.
 */
typedef struct {
  uint8_t entry_type;
  uint8_t bus_id;
  uint8_t bus_type_string[6];
} __attribute__((packed)) smp_mp_bus_entry_t;

/*
 * See [Multiprocessor_Version1.4_May_1997], section 4.3.3.
 */
typedef struct {
  uint8_t entry_type;
  uint8_t io_apic_id;
  uint8_t io_apic_version;
  uint8_t io_apic_flags;
  uint32_t memory_mapped_address_of_io_apic;
} __attribute__((packed)) smp_mp_io_apic_entry_t;

/*
 * See [Multiprocessor_Version1.4_May_1997], section 4.3.4.
 */
typedef struct {
  uint8_t entry_type;
  uint8_t interrupt_type;
  uint16_t io_interrupt_flag;
  uint8_t source_bus_id;
  uint8_t source_bus_irq;
  uint8_t destination_io_apic_id;
  uint8_t destination_io_apic_intin;
} __attribute__((packed)) smp_mp_io_interrupt_entry_t;

/*
 * See [Multiprocessor_Version1.4_May_1997], section 4.3.5.
 */
typedef struct {
  uint8_t entry_type;
  uint8_t interrupt_type;
  uint16_t local_interrupt_flag;
  uint8_t source_bus_id;
  uint8_t source_bus_irq;
  uint8_t destination_local_apic_id;
  uint8_t destination_local_apic_lintin;
} __attribute__((packed)) smp_mp_local_interrupt_entry_t;
@-

Les entrées de type processor contiennent des informations importantes
concernant le processeur référencé. Elles contiennent l'identifiant APIC unique
du processeur. Le champ CPU signature contient le résultat du cpuid si possible.
Un champ indique si le processeur est utilisable. Un autre champ indique si le
processeur courant est le BSP, tous les autres étant alors de APs. Nous laissons
pour l'instant de coté les autres type d'entrées.
La première étape de l'activation du multi-coeur est donc d'activer l'APIC.

@+ smp activate apic
void smp_activate_apic(void) {
  uint64_t v = msr_read(MSR_ADDRESS_IA32_APIC_BASE);
  msr_write(MSR_ADDRESS_IA32_APIC_BASE, v | 0x800);
}
@-

Nous devons ensuite chercher les tables et structures de configuration de la
spécification MP. Les addresses de recherches sont spécifiées dans la
documentation.

@+ smp search mp configuration table header
uint64_t smp_search_mp_floating_pointer_structure(uint64_t address, uint64_t length) {
  uint8_t mp_fps_signature[] = "_MP_";
  uint32_t i;
  for (i = 0; i < length - sizeof(mp_fps_signature); i++) {
    if ( *((uint32_t *) (address + i)) == *((uint32_t *) &mp_fps_signature[0])) {
      INFO("MP Floating Pointer Structure FOUND !\n");
      return address + i;
    }
    if (i % 0x100 == 0) {
      INFO("I : %x\n", i);
    }
  }
  return 0;
}

void smp_search_mp_configuration_table_header(void) {
  /*
   * See [Multiprocessor_Version1.4_May_1997], section 4.
   */
  uint64_t area_start = ((uint64_t) *((uint32_t *) SMP_ADDRESS_EBDA_ADDRESS));
  INFO("search smp_mp_floating_pointer_structure into EBDA (%x obtained from %x)\n",
      area_start, SMP_ADDRESS_EBDA_ADDRESS);
  uint64_t ret = smp_search_mp_floating_pointer_structure(area_start, 1024);
  if (ret == 0) {
    area_start = ((uint64_t) *((uint32_t *) SMP_ADDRESS_BASE_MEMORY_SIZE)) << 10;
    INFO("search smp_mp_floating_pointer_structure into base memory (%x obtained from %x)\n", area_start,
        SMP_ADDRESS_BASE_MEMORY_SIZE);
    ret = smp_search_mp_floating_pointer_structure(area_start - 1024, 1024);
    if (ret == 0) {
      INFO("search smp_mp_floating_pointer_structure into rom (%x)\n", SMP_ADDRESS_ROM_START);
      ret = smp_search_mp_floating_pointer_structure(SMP_ADDRESS_ROM_START,
          SMP_ADDRESS_ROM_END - SMP_ADDRESS_ROM_START);
    }
  }
  smp_mp_floating_pointer_structure = (smp_mp_floating_pointer_structure_t *) ret;
  if (ret != 0 && smp_mp_floating_pointer_structure->physical_address_pointer != 0) {
    INFO("found mp floating pointer structure at %08X\n", (uint64_t) ret);
    smp_mp_configuration_table_header = (smp_mp_configuration_table_header_t *)
        ((uint64_t) smp_mp_floating_pointer_structure->physical_address_pointer);
  }
}
@-

Une fois que nous avons trouvé le header de la table de configuration MP. Nous
allons parcourir ses entrées une par une et traiter les entrées de type
processor. Nous récupérons le nombre de processeurs et leur identifiant APIC.

@+ smp process entries
void smp_process_processor_entry(smp_mp_processor_entry_t *smp_mp_processor_entry) {
  if ((smp_mp_processor_entry->cpu_flags & SMP_MP_CPU_FLAGS_EN) == SMP_MP_CPU_FLAGS_EN) {
    INFO("processor local_apic_id=%x\n", (uint32_t) smp_mp_processor_entry->local_apic_id);
    INFO("          local_apic_version=%x\n", (uint32_t) smp_mp_processor_entry->local_apic_version);
    INFO("          cpu_flags=%x\n", (uint32_t) smp_mp_processor_entry->cpu_flags);
    INFO("          cpu_signature=%x\n", smp_mp_processor_entry->cpu_signature);
    INFO("          feature_flags=%x\n", smp_mp_processor_entry->feature_flags);
    INFO("          is bsp=%d\n", (uint32_t) (smp_mp_processor_entry->cpu_flags & SMP_MP_CPU_FLAGS_BP));
    /*
     * Local APIC IDs must be unique, and need not be consecutive.
     * See [Multiprocessor_Version1.4_May_1997], section 3.6.6.
     */
    smp_cpu_ids[smp_nb_cpus] = smp_mp_processor_entry->local_apic_id;
    smp_nb_cpus = smp_nb_cpus + 1;
    if ((smp_mp_processor_entry->cpu_flags & SMP_MP_CPU_FLAGS_BP) == SMP_MP_CPU_FLAGS_BP) {
      smp_bsp_id = smp_nb_cpus - 1;
    }
  }
}

void smp_process_entries(void) {
  uint8_t *entry_ptr = ((uint8_t *) smp_mp_configuration_table_header) +
      sizeof(smp_mp_configuration_table_header_t);
  uint32_t i;
  for (i = 0; i < smp_mp_configuration_table_header->entry_count; i++) {
    uint8_t entry_type = *entry_ptr;
    if (entry_type == SMP_MP_PROCESSOR_ENTRY_TYPE) {
      smp_process_processor_entry((smp_mp_processor_entry_t *) entry_ptr);
      entry_ptr = entry_ptr + sizeof(smp_mp_processor_entry_t);
    } else if (entry_type == SMP_MP_BUS_ENTRY_TYPE) {
      entry_ptr = entry_ptr + sizeof(smp_mp_bus_entry_t);
    } else if (entry_type == SMP_MP_IO_APIC_ENTRY_TYPE) {
      entry_ptr = entry_ptr + sizeof(smp_mp_io_apic_entry_t);
    } else if (entry_type == SMP_MP_IO_INTERRUPT_ENTRY_TYPE) {
      entry_ptr = entry_ptr + sizeof(smp_mp_io_interrupt_entry_t);
    } else if (entry_type == SMP_MP_LOCAL_INTERRUPT_ENTRY_TYPE) {
      entry_ptr = entry_ptr + sizeof(smp_mp_local_interrupt_entry_t);
    } else {
      ERROR("invalid entry type");
    }
  }
}
@-

Nous disposons de toutes les informations nécessaires pour activer les coeur.

@+ smp activate ap
void smp_activate_ap(void) {
  /*
   * See [Intel_August_2012], volume 3, section 8.4.4.1.
   */
  /*
   * Point 11: "Enable the local APIC by setting bit 8 of the APIC spurious
   * vector register (SVR)".
   * TODO: 0x20 is the "interruption". We should activate IDT!
   */
  uint32_t *svr_address = (uint32_t *) SMP_APIC_SVR;
  *svr_address = 0x20 | SMP_APIC_APIC_ENABLED;
  /*
   * Point 12.
   */
  uint32_t *lvt3_address = (uint32_t *) SMP_APIC_LVT3;
  *lvt3_address = ((*lvt3_address) & 0xffffff00) | 0x3;
  /*
   * Point 13: TODO.
   */
  /*
   * Point 14-18.
   */
  uint32_t *icr_address_low = (uint32_t *) SMP_APIC_ICR_LOW;
  uint32_t *icr_address_high = (uint32_t *) SMP_APIC_ICR_HIGH;
  uint32_t i;
  for (i = 0; i < smp_nb_cpus; i++) {
    if (i != smp_bsp_id) {
      INFO("activating cpu %d having local apic id %x\n", i, smp_cpu_ids[i]);
      /*
       * See [Intel_August_2012], volume 3, section 10.6.1, figure 10.12.
       */
      *icr_address_high = ((uint32_t) smp_cpu_ids[i]) << 24;
      *icr_address_low = 0x00004500;
      while ((*icr_address_low & SMP_ICR_DELIVERY_STATUS) == SMP_ICR_DELIVERY_STATUS_SEND_PENDING);
      *icr_address_high = ((uint32_t) smp_cpu_ids[i]) << 24;
      *icr_address_low = 0x00004600 | SMP_AP_VECTOR;
      while ((*icr_address_low & SMP_ICR_DELIVERY_STATUS) == SMP_ICR_DELIVERY_STATUS_SEND_PENDING);
      /*
       * TODO!
       */
      INFO("launch only one core!\n");
      break;
    }
  }
}
@-

La séquence d'installation du multi-cœur est la suivante :

@+ smp setup
void smp_setup(void) {
  /*
   * TODO: check the APIC physical address equals the virtual address!
   * NOTE : We are loaded in a UEFI environment where physical addr equals
   * logical one.
   */
  smp_print_info();
  smp_default_setup();
  smp_activate_apic();
  if(!smp_allocate_trampoline() && !smp_allocate_gdt_pm() &&
      !smp_allocate_gdt_lm()) {
    smp_create_ap_gdt_pm();
    smp_create_ap_gdt_lm();
    smp_install_trampoline();
  }
}
@-

Une fois le mode smp préparé, nous allons entamer la séquence d'initialisation
des coeurs.

\begin{figure}[h]
  \centering
  \includegraphics[width=\figwidth]{figures/smp_setup.pdf}
  \caption{Séquence d'installation et d'activation de smp}
  \label{fig:ap_init}
\end{figure}

@+ smp activate cores
void smp_activate_cores() {
  smp_print_trampoline((uint8_t *) &trampoline_start); // TODO
  smp_prepare_trampoline(); // TODO
  smp_print_trampoline((uint8_t *) (SMP_AP_VECTOR << 12)); // TODO
  smp_search_mp_configuration_table_header();
  if (smp_mp_configuration_table_header != 0) {
    smp_print_mp_configuration_table_header(); // TODO
    smp_process_entries();
    smp_activate_ap(); // TODO
  }
}
@-

\section{Fichiers}

@++ smp.c
#include "smp.h"

#include "stdio.h"
#include "msr.h"
#include "cpu.h"
#include "systab.h"
#include "string.h"

/*
 * See [Intel_August_2012], volume 3, section 8.4.4.
 * See [Multiprocessor_Version1.4_May_1997].
 */

#define SMP_ADDRESS_EBDA_ADDRESS	0x00400e
#define SMP_ADDRESS_BASE_MEMORY_SIZE	0x004013
#define SMP_ADDRESS_ROM_START		0xf0000
#define SMP_ADDRESS_ROM_END		0xfffff

#define SMP_APIC_ICR_LOW	0xfee00300
#define SMP_APIC_ICR_HIGH	0xfee00310
#define SMP_APIC_SVR		0xfee000f0
#define SMP_APIC_APIC_ID	0xfee00020
#define SMP_APIC_LVT3		0xfee00370
#define SMP_APIC_APIC_ENABLED	0x100

#define SMP_ICR_DELIVERY_STATUS (1 << 11)
#define SMP_ICR_DELIVERY_STATUS_SEND_PENDING (1 << 11)

#define SMP_AP_VECTOR 0x1

#define SMP_MP_PROCESSOR_ENTRY_TYPE		0
#define SMP_MP_BUS_ENTRY_TYPE			1
#define SMP_MP_IO_APIC_ENTRY_TYPE		2
#define SMP_MP_IO_INTERRUPT_ENTRY_TYPE		3
#define SMP_MP_LOCAL_INTERRUPT_ENTRY_TYPE	4

#define SMP_MP_CPU_FLAGS_EN	0x1
#define SMP_MP_CPU_FLAGS_BP	0x2

@< floating point structure >
@< mp configuration table >
@< mp configuration table entries >

uint8_t smp_nb_cpus;
uint8_t smp_bsp_id;
smp_mp_floating_pointer_structure_t *smp_mp_floating_pointer_structure;
smp_mp_configuration_table_header_t *smp_mp_configuration_table_header;

uint8_t smp_cpu_ids[16];
uint8_t smp_nb_cpus;
uint8_t smp_bsp_id;

/* trampoline locations */
extern uint8_t trampoline_start;
extern uint8_t trampoline_end;
extern uint8_t ap_gdtptr;
extern uint8_t ap_param;

/* installed trampoline physical address */
void *ap_trampoline_start;

void smp_print_info(void) {
  INFO("apic physical address: %016x\n", msr_read(MSR_ADDRESS_IA32_APIC_BASE));
}

void smp_print_trampoline(uint8_t *area) {
  uint8_t *ptr_start = area;
  uint8_t *ptr_end = (uint8_t *) (((uint64_t) &trampoline_end) - ((uint64_t) &trampoline_start) + area);
  uint8_t line_size = 16;
  uint8_t previous_line_hidden = 0;
  uint8_t j;
  while (ptr_start < ptr_end) {
    uint8_t must_print_line = 0;
    for (j = 0; j < line_size && (&ptr_start[j] < ptr_end); j++) {
      if (ptr_start[j] != 0x0) {
        must_print_line = 1;
        break;
      }
    }
    if (must_print_line == 1) {
      if (previous_line_hidden == 1) {
        printk("...\n");
      }
      previous_line_hidden = 0;
      for (j = 0; j < line_size && (&ptr_start[j] < ptr_end); j++) {
        printk("%02x ", ptr_start[j]);
      }
      printk("\n");
    } else {
      previous_line_hidden = 1;
    }
    ptr_start = &ptr_start[line_size];
  }
  printk("\n");
}

void smp_print_mp_configuration_table_header(void) {
  uint8_t oemid[5];
  oemid[4] = 0;
  uint8_t i;
  for (i = 0; i < 4; i++) {
    oemid[i] = smp_mp_configuration_table_header->oemid[i];
  }
  uint8_t prodid[13];
  prodid[12] = 0;
  for (i = 0; i < 12; i++) {
    prodid[i] = smp_mp_configuration_table_header->prodid[i];
  }
  INFO("smp_mp_floating_pointer_structure: oemid='%s' prodid='%s'\n", oemid, prodid);
  INFO("  entry count='%d'\n", smp_mp_configuration_table_header->entry_count);
}

void smp_default_setup(void) {
  smp_nb_cpus = 1;
  smp_mp_configuration_table_header = 0;
  smp_mp_floating_pointer_structure = 0;
  smp_nb_cpus = 0;
  smp_bsp_id = 255;
}

void smp_ap_fcn(void) {
  uint64_t p = 0;
  uint16_t i, k;
  uint8_t j, l;
  for (k = 0; k < 0x40; k++)
    for (i = 0; i < 0xffff; i++)
      p += i;
  printk("I am an AP! %08X\n", (uint64_t) p);
  uint32_t counter_x = 74;
  uint32_t counter_y = 0;
  uint32_t nb_columns = 80;
  uint16_t *pos = (uint16_t *) ((counter_x + counter_y * nb_columns) * sizeof(uint16_t) + 0xb8000);
  uint16_t number = 0;
  while (1) {
    /*
     * A 16 bits number contains 5 digits in decimal.
     */
    for (j = 0; j < 5; j++) {
      uint16_t tmp = number;
      for (l = 0; l < j; k++) {
        tmp = tmp / 10;
      }
      pos[5 - j] = (uint16_t) (((uint16_t) '0' + (tmp % 10))) | 0x3d00;
    }
    for (k = 0; k < 0xf; k++)
      for (i = 0; i < 0xffff; i++);
    number = number + 1;
  }
  while (1);
}

void smp_prepare_trampoline(void) {
  uint8_t *ptr_start = &trampoline_start;
  uint8_t *ptr_end = &trampoline_end;
  uint8_t *ptr_dst = (uint8_t *) (SMP_AP_VECTOR << 12);
  while (ptr_start < ptr_end) {
    *ptr_dst = *ptr_start;
    ptr_start++;
    ptr_dst++;
  }
  /*
   * TODO: add a signature into the trampoline to identify the header!
   */
  uint64_t *trampoline_cr3 = (uint64_t *) ((uint8_t *) ((SMP_AP_VECTOR << 12) + 12));
  uint8_t *trampoline_gdt = ((uint8_t *) ((SMP_AP_VECTOR << 12) + 4));
  uint64_t *trampoline_fcn = ((uint64_t *) ((SMP_AP_VECTOR << 12) + 20));
  cpu_read_gdt(trampoline_gdt);
  *trampoline_cr3 = cpu_read_cr3();
  *trampoline_fcn = (uint64_t) smp_ap_fcn;
  INFO("trampoline cr3: %08X\n", (uint64_t) *trampoline_cr3);
}

@< smp allocate trampoline >
@< smp allocate gdt pm >
@< smp allocate gdt lm >
@< smp install trampoline >
@< smp create ap gdt pm >
@< smp create ap gdt lm >
@< smp activate apic >
@< smp search mp configuration table header >
@< smp process entries >
@< smp activate ap >
@< smp activate cores >
@< smp setup >
@-

@++ smp.h
#ifndef __SMP_H__
#define __SMP_H__
#include <efi.h>
#include "gdt.h"

@< smp ap param >
void smp_setup(void);

#endif//__SMP_H__
@-
