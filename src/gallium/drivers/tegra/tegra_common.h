#ifndef TEGRA_COMMON_H
#define TEGRA_COMMON_H

#include "tegra_screen.h"

#define unimplemented() do { \
   if (tegra_debug & TEGRA_DEBUG_UNIMPLEMENTED) \
      printf("TODO: %s()\n", __func__); \
} while (0)

#endif
