#ifndef GRATE_COMMON_H
#define GRATE_COMMON_H

#include "grate_screen.h"

#define unimplemented() do { \
   if (grate_debug & GRATE_DEBUG_UNIMPLEMENTED) \
      printf("TODO: %s()\n", __func__); \
} while (0)

#endif
