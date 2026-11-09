/* CPU-only test of actual driver resource create/map/copy with mock GEM BOs. */
#include "grate_resource.c"
struct mock_bo { unsigned char *allocation; uint32_t size; };
int __wrap_drm_tegra_bo_new(struct drm_tegra_bo **,struct drm_tegra *,uint32_t,uint32_t);
int __wrap_drm_tegra_bo_map(struct drm_tegra_bo *,void **);
int __wrap_drm_tegra_bo_unmap(struct drm_tegra_bo *);
int __wrap_drm_tegra_bo_unref(struct drm_tegra_bo *);
int __wrap_drm_tegra_bo_get_size(struct drm_tegra_bo *,uint32_t *);
int __wrap_drm_tegra_bo_get_handle(struct drm_tegra_bo *,uint32_t *);
int __wrap_drm_tegra_bo_new(struct drm_tegra_bo **bo,struct drm_tegra*d,uint32_t flags,uint32_t size){
 struct mock_bo*m=calloc(1,sizeof(*m));assert(m);m->allocation=malloc((size_t)size+32);assert(m->allocation);memset(m->allocation,0xad,(size_t)size+32);m->size=size;*bo=(struct drm_tegra_bo*)m;return 0;
}
int __wrap_drm_tegra_bo_map(struct drm_tegra_bo*bo,void**p){*p=((struct mock_bo*)bo)->allocation+16;return 0;}
int __wrap_drm_tegra_bo_unmap(struct drm_tegra_bo*bo){return 0;}
int __wrap_drm_tegra_bo_get_size(struct drm_tegra_bo*bo,uint32_t*s){*s=((struct mock_bo*)bo)->size;return 0;}
int __wrap_drm_tegra_bo_get_handle(struct drm_tegra_bo*bo,uint32_t*h){*h=1;return 0;}
int __wrap_drm_tegra_bo_unref(struct drm_tegra_bo*bo){
 struct mock_bo*m=(struct mock_bo*)bo;for(unsigned i=0;i<16;i++){assert(m->allocation[i]==0xad);assert(m->allocation[m->size+16+i]==0xad);}free(m->allocation);free(m);return 0;
}
int main(void){
 struct grate_screen screen={0};struct grate_context context={0};
 screen.base.resource_destroy=grate_resource_destroy;context.base.screen=&screen.base;
 slab_create_parent(&screen.transfer_pool,sizeof(struct pipe_transfer),16);
 slab_create_child(&context.transfer_pool,&screen.transfer_pool);grate_context_resource_init(&context.base);
 struct pipe_resource t={.target=PIPE_TEXTURE_2D,.format=PIPE_FORMAT_B8G8R8A8_UNORM,.width0=128,.height0=128,.depth0=1,.array_size=1,.last_level=7,.bind=PIPE_BIND_SAMPLER_VIEW};
 struct pipe_resource*a=grate_screen_resource_create(&screen.base,&t),*b=grate_screen_resource_create(&screen.base,&t);assert(a&&b);
 assert(grate_resource(a)->mip.pitch[7]==16 && grate_resource(a)->mip.offset[7]+16==grate_resource(a)->mip.total);
 for(unsigned l=0;l<8;l++){
  unsigned n=128>>l;struct pipe_box box={.width=n,.height=n,.depth=1};struct pipe_transfer*x=NULL;
  unsigned char*p=grate_resource_transfer_map(&context.base,a,l,PIPE_MAP_WRITE,&box,&x);assert(p&&x);
  for(unsigned y=0;y<n;y++)memset(p+y*x->stride,17+l*23,n*4);
  grate_resource_transfer_unmap(&context.base,x);
 }
 for(unsigned l=0;l<8;l++){
  unsigned n=128>>l;struct pipe_box box={.width=n,.height=n,.depth=1};struct pipe_transfer*x=NULL;
  unsigned char*p=grate_resource_transfer_map(&context.base,a,l,PIPE_MAP_READ,&box,&x);assert(p&&x);
  for(unsigned y=0;y<n;y++)for(unsigned k=0;k<n*4;k++)assert(p[y*x->stride+k]==17+l*23);
  grate_resource_transfer_unmap(&context.base,x);
 }
 struct pipe_box box={.width=32,.height=32,.depth=1};
 grate_resource_copy_region(&context.base,b,2,0,0,0,a,2,&box);
 struct pipe_transfer*x=NULL;unsigned char*p=grate_resource_transfer_map(&context.base,b,2,PIPE_MAP_READ,&box,&x);
 for(unsigned y=0;y<32;y++)for(unsigned k=0;k<128;k++)assert(p[y*x->stride+k]==63);
 grate_resource_transfer_unmap(&context.base,x);
 box.width=box.height=64;p=grate_resource_transfer_map(&context.base,b,1,PIPE_MAP_READ,&box,&x);
 for(unsigned y=0;y<64;y++)for(unsigned k=0;k<256;k++)assert(p[y*x->stride+k]==0xad);
 grate_resource_transfer_unmap(&context.base,x);
 grate_resource_destroy(&screen.base,a);grate_resource_destroy(&screen.base,b);
 slab_destroy_child(&context.transfer_pool);slab_destroy_parent(&screen.transfer_pool);
 puts("mip create/map/copy: 8 distinct levels, neighbor preservation and BO canaries PASS");return 0;
}
