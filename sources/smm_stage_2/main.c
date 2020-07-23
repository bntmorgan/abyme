#include <stdint.h>

struct kernel_state {
  uint64_t cr3;
  uint64_t rbp;
  uint64_t rsp;
};

int initialized = 0;

// Initialize kernel
void kernel_init(void) {
}

// Set kernel configuration as memory for instance
void kernel_set(void) {
}

// Restore caller's configuration as memory for instance
void kernel_restore(void) {
}

void kernel_start(void) {

  // First call by the loader
  if (initialized == 0) {
    initialized = 1;
    kernel_init();
    return;
  }

  // Else we have to restore virtual memory and play !
  kernel_set();
  kernel_restore();
}
