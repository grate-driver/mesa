#ifndef GRATE_DIAGNOSTICS_H
#define GRATE_DIAGNOSTICS_H
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
/* Per-process setting. Error/STOP messages are never suppressed. */
static inline bool grate_diagnostics_enabled(void)
{
   const char *value = getenv("GRATE_DIAGNOSTICS");
   return !value || strcmp(value, "0") != 0;
}
#endif
