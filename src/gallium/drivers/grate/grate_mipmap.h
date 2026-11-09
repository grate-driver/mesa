#ifndef GRATE_MIPMAP_H
#define GRATE_MIPMAP_H
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#define GRATE_MIP_LEVELS 16
struct grate_mip_layout {
   uint32_t pitch[GRATE_MIP_LEVELS];
   uint32_t offset[GRATE_MIP_LEVELS];
   uint32_t size[GRATE_MIP_LEVELS];
   uint32_t total;
   unsigned levels;
};
/* Linear, uncompressed, single-layer 2D storage only. No sampler descriptors. */
static inline bool
grate_mip_layout_init(struct grate_mip_layout *layout, uint32_t width,
                      uint32_t height, unsigned cpp, unsigned levels,
                      unsigned alignment)
{
   struct grate_mip_layout result = {0};
   if (!width || !height || !cpp || cpp > 16 || !levels ||
       levels > GRATE_MIP_LEVELS || !alignment || alignment > 4096 ||
       (alignment & (alignment - 1)))
      return false;
   uint64_t end = 0;
   for (unsigned level = 0; level < levels; level++) {
      if (level && width == 1 && height == 1)
         return false;
      if (level) { width = width > 1 ? width / 2 : 1;
                   height = height > 1 ? height / 2 : 1; }
      uint64_t pitch = ((uint64_t)width * cpp + alignment - 1) &
                       ~(uint64_t)(alignment - 1);
      uint64_t offset = (end + alignment - 1) & ~(uint64_t)(alignment - 1);
      if (pitch > UINT32_MAX || offset > UINT32_MAX)
         return false;
      uint64_t size = pitch * height;
      if (pitch > UINT32_MAX || offset > UINT32_MAX ||
          size > UINT32_MAX || offset + size > UINT32_MAX)
         return false;
      result.pitch[level] = pitch;
      result.offset[level] = offset;
      result.size[level] = size;
      end = offset + size;
   }
   result.total = end;
   result.levels = levels;
   *layout = result;
   return true;
}
static inline bool
grate_mip_box_valid(const struct grate_mip_layout *layout, unsigned level,
                     uint32_t width, uint32_t height, unsigned cpp,
                     int x, int y, int w, int h, uint32_t bo_size)
{
   if (layout->levels > GRATE_MIP_LEVELS || level >= GRATE_MIP_LEVELS ||
       level >= layout->levels || !cpp || cpp > 16 || x < 0 || y < 0 || w <= 0 || h <= 0)
      return false;
   width >>= level; height >>= level;
   if (!width) width = 1;
   if (!height) height = 1;
   uint64_t right = (uint64_t)x + w, bottom = (uint64_t)y + h;
   if (right > width || bottom > height || right * cpp > layout->pitch[level])
      return false;
   uint64_t end = (bottom - 1) * layout->pitch[level] + right * cpp;
   return end <= layout->size[level] &&
          layout->offset[level] + end <= bo_size;
}
#endif
