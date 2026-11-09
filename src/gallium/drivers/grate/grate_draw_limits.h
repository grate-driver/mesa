#ifndef GRATE_DRAW_LIMITS_H
#define GRATE_DRAW_LIMITS_H

#include <limits.h>
#include <string.h>
#include "pipe/p_state.h"
#include "util/u_split_draw.h"

/* Preserve the previously exercised command limit. Use even strip batches. */
#define GRATE_DRAW_LIMIT 2047u
#define GRATE_SPLIT_LIMIT 2046u

static inline bool
grate_next_draw_chunk(enum pipe_prim_type mode, unsigned remaining,
                      unsigned *count, unsigned *step)
{
   if (!remaining) return false;
   *count = *step = remaining;
   if (remaining <= GRATE_DRAW_LIMIT) return true;
   switch (mode) {
   case PIPE_PRIM_POINTS:
   case PIPE_PRIM_LINES:
   case PIPE_PRIM_LINE_STRIP:
   case PIPE_PRIM_TRIANGLES:
   case PIPE_PRIM_TRIANGLE_STRIP:
      break;
   default:
      return false; /* Fans/loops require rebuilding indices, not truncation. */
   }
   struct pipe_draw_info info = { .mode = mode };
   u_split_draw(&info, GRATE_SPLIT_LIMIT, count, step);
   return *count && *count <= GRATE_DRAW_LIMIT && *step && *step <= remaining;
}

static inline bool
grate_index_range_valid(unsigned start, unsigned count, unsigned index_size,
                        unsigned logical_size, unsigned bo_size)
{
   if (index_size != 1 && index_size != 2 && index_size != 4) return false;
   uint64_t end = ((uint64_t)start + count) * index_size;
   return end <= logical_size && end <= bo_size;
}

/* GR3D has verified UINT8/UINT16 modes only. Never submit reserved mode 3. */
static inline bool
grate_index32_to_16(uint8_t *indices, unsigned count)
{
   if (count > GRATE_DRAW_LIMIT) return false;
   for (unsigned i = 0; i < count; i++) {
      uint32_t value;
      memcpy(&value, indices + i * 4, sizeof(value));
      if (value > UINT16_MAX) return false;
      uint16_t narrowed = value;
      memcpy(indices + i * 2, &narrowed, sizeof(narrowed));
   }
   return true;
}

#endif
