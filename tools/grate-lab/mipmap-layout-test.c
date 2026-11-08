#include <assert.h>
#include <string.h>
#include "grate_mipmap.h"
int main(void)
{
   struct grate_mip_layout m, previous;
   assert(grate_mip_layout_init(&m,128,128,4,8,32));
   assert(m.pitch[0]==512 && m.size[0]==65536 && m.offset[1]==65536);
   assert(m.pitch[7]==32 && m.size[7]==32);
   for (unsigned l=0;l<8;l++) {
      unsigned n=128>>l;
      assert(grate_mip_box_valid(&m,l,128,128,4,0,0,n,n,m.total));
      assert(!grate_mip_box_valid(&m,l,128,128,4,0,0,n+1,n,m.total));
      if(l)assert(m.offset[l]>=m.offset[l-1]+m.size[l-1]);
   }
   assert(!grate_mip_box_valid(&m,8,128,128,4,0,0,1,1,m.total));
   assert(!grate_mip_box_valid(&m,0,128,128,4,-1,0,1,1,m.total));
   assert(!grate_mip_box_valid(&m,7,128,128,4,0,0,1,1,m.offset[7]+3));
   previous=m;
   assert(!grate_mip_layout_init(&m,UINT32_MAX,UINT32_MAX,16,1,32));
   assert(!memcmp(&m,&previous,sizeof(m)));
   assert(!grate_mip_layout_init(&m,128,128,4,9,32));
   assert(!grate_mip_layout_init(&m,0,1,4,1,32));
   assert(!grate_mip_layout_init(&m,128,128,4,8,3));
   assert(grate_mip_layout_init(&m,63,35,4,6,32));
   for(unsigned l=0;l<6;l++) {
      unsigned w=63>>l,h=35>>l;
      if(!w)w=1;if(!h)h=1;
      assert(grate_mip_box_valid(&m,l,63,35,4,0,0,w,h,m.total));
   }
   return 0;
}
