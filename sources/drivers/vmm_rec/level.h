#include "stdio.h"
#ifdef _DEBUG_SERVER
#include "debug_server/debug_server.h"
#endif

#ifdef _DEBUG_SERVER
#define LEVEL(level, ...)  \
  if (debug_server_level == level) { \
    PRINTK(0, "[level]",   __VA_ARGS__); \
  }
#else
#define LEVEL(level, ...)
#endif
