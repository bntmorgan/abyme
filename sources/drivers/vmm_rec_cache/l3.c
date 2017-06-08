#include "stdio.h"
#include "string.h"
#include "efiw.h"

struct probed_line {
  char *line;
  int count;
};

int probe(char **set, int ss, char *candidate);
void randomize_lines(char **ls, int s);
void minus(char **s1, int ss1, char **s2, int ss2, char **so, int *sso);
void print_line(char *l, int cr);

#define L3_PD_OFFSET_WIDTH 21

/**
 * See
 * /sys/devices/system/cpu/cpu0/cache/index3/
 */

#define L3_LINE_WIDTH 6 // Line size log2
#define L3_SETS_WIDTH 13 // Number of sets log2
#define L3_ASSOC 16 // Lines per set
#define L3_RTAG_WIDTH (L3_PD_OFFSET_WIDTH - L3_SETS_WIDTH - \
    L3_LINE_WIDTH) // Number of visible tags log2
#define L3_CACHE_SIZE (((1 << L3_LINE_WIDTH) << L3_SETS_WIDTH) \
    * L3_ASSOC) // Total size
#define L3_2MB_TAG_NB (1 << \
    (L3_PD_OFFSET_WIDTH - L3_LINE_WIDTH - L3_SETS_WIDTH))

/**
 * Algorithm parameters
 */

#define L3_ASSOC_FACTOR_WIDTH 3 // times associativity log2
#define L3_LINES_NB (L3_ASSOC << L3_ASSOC_FACTOR_WIDTH)
#define L3_BUF_SIZE ((L3_LINES_NB / L3_2MB_TAG_NB) << 21)
#define L3_TARGET_SET (0x666 & ((1 << L3_SETS_WIDTH) - 1))
#define L3_TIME_THRESHOLD 200 // 40 for L2 !!!!! 200 for L3 !!!

static char **buf;

int l3_init(void) {
  // The uefi allocator allocates 4kB ID mapped pages
  // Our hypervisor is reallocating 4kB id mapped memory too.
  // Then we can use the algorithm as if 2MB non ID mapped pages where used
  // We force the allocation to be 2MB aligned by multiplying allocation size by
  //    2 and shifting the base allocated pointer to the next 2MB page
  buf = efi_allocate_pages((L3_BUF_SIZE >> 12) << 1);
  buf = (char **)(((uint64_t)buf + (1 << L3_PD_OFFSET_WIDTH)) &
      ~((1 << L3_PD_OFFSET_WIDTH) - 1));
  return 0;
}

int l3(void) {
  int p, t, l, lc, i;
  uintptr_t pa, ta;
  char *lines[L3_LINES_NB];
  char *cset[L3_LINES_NB];
  char *eset[L3_LINES_NB];
  char *tset[L3_LINES_NB];
  char *mset[L3_LINES_NB];
  int css = 0, ess = 0, tss = 0, mss = 0;

  INFO("Processor specification:\n"
      "  Line width 0x%x\n"
      "  Sets 0x%x\n"
      "  Associativity 0x%x\n"
      "  Total L3 cache size 0x%x\n",
      L3_LINE_WIDTH, 1 << L3_SETS_WIDTH, L3_ASSOC, L3_CACHE_SIZE);

  INFO("Algorithm parameters:\n"
      "  Associativity factor 0x%x\n"
      "  Searching eviction set in 0x%x lines\n"
      "  Target set 0x%x of 0x%x\n",
      1 << L3_ASSOC_FACTOR_WIDTH, L3_LINES_NB, L3_TARGET_SET, 1 << L3_SETS_WIDTH);

  INFO("Allocated pages @0x%016X of size 0x%08x\n", (uintptr_t)buf,
      L3_BUF_SIZE);

  /**
   * First creating the cache lines
   */

  INFO("-== Generating parameters ==-\n");

  // Iterate over the 2 MB pages
  for (p = 0; p < (L3_BUF_SIZE >> L3_PD_OFFSET_WIDTH); p++) {
    pa = (uintptr_t)(buf + (p << L3_PD_OFFSET_WIDTH));
    INFO("Page %d, @0x%016X\n", p, pa);
    // Iterating over available tags in the same 2MB page
    for (t = 0; t < L3_2MB_TAG_NB; t++) {
      ta = pa | (t << (L3_LINE_WIDTH + L3_SETS_WIDTH));
      INFO("Tag (Lines) %d, @0x%016X\n", t, ta);
      // We add the line for the targeted set
      lines[p * L3_2MB_TAG_NB + t] =
          (char *)(ta | (L3_TARGET_SET << L3_LINE_WIDTH));
    }
  }

  /**
   * Displays the generated lines
   */
  for (l = 0; l < L3_LINES_NB; l++) {
    INFO("Line %d @0x%016X\n", l, (uintptr_t)lines[l]);
  }

  /**
   * Creating the eviction sets
   */

  // Set random number generator seed
//  now = time(NULL);
//  srand48(now);

  // randomize_lines(&lines[0], L3_LINES_NB);

  INFO("-== Step 1 : Global conflicting set ==-\n");

  // Step 1 : conflict set
  for (l = 0; l < L3_LINES_NB; l++) {
    if (!probe(&cset[0], css, lines[l])) {
      INFO("Adding @0x%016X\n", lines[l]);
      cset[css] = lines[l];
      css++;
    }
  }

  /**
   * Displays the conflict set
   */
  INFO("Size of conflict set 0x%x\n", css);
  for (l = 0; l < css; l++) {
    INFO("Line @0x%016X\n", (uintptr_t)lines[l]);
  }

  /**
   * Compute lines = lines - conflict set
   */
  minus(lines, L3_LINES_NB, cset, css, tset, &tss);
  memcpy(&mset[0], &tset[0], sizeof(char *) * tss);
  mss = tss;
  INFO("Size of lines - conflict set 0x%x\n", mss);
  for (l = 0; l < mss; l++) {
    INFO("Line @0x%016X\n", (uintptr_t)mset[l]);
  }

  INFO("-== Step 2 : Iterative eviction set building ==-\n");

  /**
   * Step 2 : creating eviction sets
   */
  for (l = 0; l < mss; l++) {
    INFO("Eviction set for line @0x%016X\n", mset[l]);
    if (probe(&cset[0], css, mset[l])) {
      ess = 0;
      for (lc = 0; lc < css; lc++) {
        INFO("Candidate @0x%016X\n", cset[lc]);
        // Creating cset - {lc}
        tss = 0;
        for (i = 0; i < css; i++) {
          if (cset[i] != cset[lc]) {
            tset[tss] = cset[i];
            tss++;
          }
        }
        if (!probe(&tset[0], tss, mset[l])) {
          INFO("Adding @0x%016X\n", cset[lc]);
          eset[ess] = cset[lc];
          ess++;
        }
      }
      // Print eviction set
      if (ess > 0) {
        INFO("New eviction for line @0x%016X set of size 0x%x:\n", l, ess);
        for (lc = 0; lc < ess; lc++) {
          INFO("  Line @0x%016X\n", (uintptr_t)eset[lc]);
        }
        // Remove eviction set from conflict set
        minus(cset, css, eset, ess, tset, &tss);
        memcpy(&cset[0], &tset[0], sizeof(char *) * tss);
        css = tss;
      } else {
        INFO("Eviction set is null\n");
      }
    }
  }

  return 0;
}

/**
 * TODO Write it in assembly
 */
int probe(char **set, int ss, char *candidate) {
  uint64_t time;
  char **llp;
  INFO("Probing ");
  print_line(candidate, 1);
  // printk(", set size 0x%x\n", ss);
  __asm__ __volatile__("callq asm_probe"
      : "=a"(time), "=c"(llp) : "a"(set), "b"(ss), "c"(candidate));
  INFO("Candidate access time %d\n", time);
  // INFO("Last line pointer accessed @0x%016X of base @0x%016X\n", llp, set);
  if (llp != NULL) {
    // INFO("Last line accessed @0x%016X\n", *llp);
  }
  return time > L3_TIME_THRESHOLD;
}

//void randomize_lines(char **ls, int s) {
//  int i;
//  long int r;
//  char *l;
//  // Shuffle array
//  for (i = s - 1; i > 0; i--) {
//    r = lrand48() % (i + 1);
//    // INFO("RANDOM %x\n", r);
//    l = ls[i];
//    ls[i] = ls[r];
//    ls[r] = l;
//  }
//}

int in(char **s, int ss, char *e) {
  int i;
  for (i = 0; i < ss; i++) {
    if (s[i] == e) {
      return 1;
    }
  }
  return 0;
}

void minus(char **s1, int ss1, char **s2, int ss2, char **so, int *sso) {
  int i;
  *sso = 0;
  for (i = 0; i < ss1; i++) {
    // Search s1[i] in s2, if not, then add it to so
    if (!in(s2, ss2, s1[i])) {
      so[*sso] = s1[i];
      (*sso)++;
    }
  }
}

void print_line(char *l, int cr) {
  printk("Line @0x%016X : rtag 0x%x, set 0x%x, line 0x%x", (uintptr_t)l,
      (((uintptr_t)l) >> (L3_SETS_WIDTH + L3_LINE_WIDTH)) &
          ((1 << L3_RTAG_WIDTH) - 1),
      (((uintptr_t)l) >> L3_LINE_WIDTH) & ((1 << L3_SETS_WIDTH) - 1),
      ((uintptr_t)l) & ((1 << L3_LINE_WIDTH) - 1));
  if (cr) {
    printk("\n");
  }
}
