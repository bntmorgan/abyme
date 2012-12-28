#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define MAX_PROCESSING 0x800
#define CR3_LOGS_SIZE 0x800

uint64_t cr3_logs[CR3_LOGS_SIZE];

int main() {
  uint32_t cr3_logs_base;

  // Enable CR3 logging and get buffer base address
  __asm__ __volatile__("vmcall" : "=b" (cr3_logs_base) : "a" (1));

  printf("cr3_logs_base = %08x\n", cr3_logs_base);

  int fd = open("/dev/fmem", O_RDONLY);
  if (fd < 0) {
    perror("open");
    return 1;
  }

  int log_fd = open("cr3.log", O_CREAT | O_WRONLY);
  if (log_fd < 0) {
    perror("open (log)");
    return 1;
  }

  uint32_t processed = 0;
  while (processed < MAX_PROCESSING) {
    uint8_t go;
    uint32_t nb_logs;
    do {
      // Ask for read permission and get size to read
      __asm__ __volatile__("vmcall" : "=a" (go), "=c" (nb_logs) : "a" (2));
      if (!go) {
        //printf("no go, nb_logs = %d\n", nb_logs);
        usleep(10000);
      }
    } while (!go);

    printf("go, nb_logs = %d\n", nb_logs);

    printf("seeking to %x\n", cr3_logs_base);
    if (lseek(fd, cr3_logs_base, SEEK_SET) < 0) {
      perror("lseek");
    }
    printf("reading %d bytes\n", nb_logs * sizeof(uint64_t));
    int lread = read(fd, cr3_logs, nb_logs * sizeof(uint64_t));
    if (lread != nb_logs * sizeof(uint64_t)) {
      perror("read");
      fprintf(stderr, "read %d bytes while expecting %d bytes.\n", lread, nb_logs * sizeof(uint64_t));
    }

    uint32_t missed_logs, new_nb_logs;
    // Signal end of reading
    __asm__ __volatile__("vmcall" : "=a" (missed_logs), "=c" (new_nb_logs) : "a" (3));

    printf("missed_logs = %d, new_nb_logs = %d (remaining = %d)\n", missed_logs, new_nb_logs, new_nb_logs - nb_logs);

    if (new_nb_logs != nb_logs) {
      printf("seeking to %x\n", cr3_logs_base + nb_logs * sizeof(uint64_t));
      if (lseek(fd, cr3_logs_base + nb_logs * sizeof(uint64_t), SEEK_SET) < 0) {
        perror("lseek (2)");
      }
      printf("reading %d bytes\n", (new_nb_logs - nb_logs) * sizeof(uint64_t));
      int lread = read(fd, cr3_logs + nb_logs, (new_nb_logs - nb_logs) * sizeof(uint64_t));
      if (lread != (new_nb_logs - nb_logs) * sizeof(uint64_t)) {
        perror("read (2)");
        fprintf(stderr, "read %d bytes while expecting %d bytes.\n", lread, (new_nb_logs - nb_logs) * sizeof(uint64_t));
      }
    }

    if (missed_logs != 0) {
      fprintf(stderr, "Warning: %d missed logs.\n", missed_logs);
    }

    char buf[20];
    int i;
    for (i = 0; i < new_nb_logs; i++) {
      sprintf(buf, "%08x\n", cr3_logs[i]);
      write(log_fd, buf, strlen(buf));
    }

    processed += new_nb_logs + missed_logs;
  }

  close(log_fd);
  close(fd);

  // Disable CR3 logging
  __asm__ __volatile__("vmcall" : "=b" (cr3_logs_base) : "a" (4));

  return 0;
}
