#include <alloca.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cpuid.h>
#include <unistd.h>
#include <syslinux/boot.h>
#include <com32.h>
#include <consoles.h>

/*#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
*/

unsigned int hex_to_uint(const char *nptr) {
    return (unsigned int) strntoumax(nptr, (char **)NULL, 16, ~(size_t) 0);
}

unsigned char hex_to_uchar(const char *nptr) {
    return (unsigned char) strntoumax(nptr, (char **)NULL, 16, ~(size_t) 0);
}

static inline void error(const char *msg) {
  fputs(msg, stderr);
}

static void usage(void) {
  error("Run one command if system match some CPU features, another if it doesn't. \n"
    "Usage: \n"
    "   label writemem \n"
    "       com32 writemem.c32 \n"
    "       append address value address value -- boot_entry\n"
    "   label boot_entry \n"
    "       kernel vmlinuz_entry \n"
    "    append ... \n"
    "\n");
}

static void boot_args(char **args) {
    int len = 0, a = 0;
    char **pp;
    const char *p;
    char c, *q, *str;

    for (pp = args; *pp; pp++)
  len += strlen(*pp) + 1;

    q = str = alloca(len);
    for (pp = args; *pp; pp++) {
  p = *pp;
  while ((c = *p++))
      *q++ = c;
  *q++ = ' ';
  a = 1;
    }
    q -= a;
    *q = '\0';
printf("cmd: %s\n", str);
    if (!str[0])
  syslinux_run_default();
    else
  syslinux_run_command(str);
}


int main(int argc, char *argv[]) {
  int i=0;

  console_ansi_raw();

  if (argc == 1) {
    usage();
    return -1;
  }

  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--")) {
      i++;
      break;
    } else {
      unsigned int address = hex_to_uint(argv[i]);
      unsigned char value = hex_to_uchar(argv[i + 1]);
      unsigned char* ptr = (unsigned char*)address;
      printf("Set %02x at %08x instead of %02x\n", value, address, *ptr);
      *ptr = value;
      printf("Value %02x at %08x\n", *ptr, address);
      sleep(5);
      i = i + 1;
    }
  }
  boot_args(&argv[i]);
  return -1;
}
