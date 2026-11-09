#define _POSIX_C_SOURCE 200809L
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xrender.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
static const unsigned char glyphs[5][7]={{14,17,16,23,17,17,14},{30,17,17,30,20,18,17},{14,17,17,31,17,17,17},{31,4,4,4,4,4,4},{31,16,16,30,16,16,31}};
static int xerror(Display*d,XErrorEvent*e){char s[200];XGetErrorText(d,e->error_code,s,sizeof(s));fprintf(stderr,"XERROR %s request=%u minor=%u\n",s,e->request_code,e->minor_code);exit(10);}
int main(void){
 Display*d=XOpenDisplay(NULL);if(!d)return 2;XSetErrorHandler(xerror);
 int screen=DefaultScreen(d),event,error;Window root=RootWindow(d,screen);
 if(!XRenderQueryExtension(d,&event,&error))return 3;
 Atom atom=XInternAtom(d,"_NET_WM_CM_S0",False);Window owner=XGetSelectionOwner(d,atom);
 printf("COMPOSITOR selection_owner=0x%lx\n",owner);fflush(stdout);
 XRenderPictFormat*fmt=XRenderFindStandardFormat(d,PictStandardARGB32),*a8=XRenderFindStandardFormat(d,PictStandardA8);
 if(!fmt||!a8)return 4;
 const unsigned n=512;Pixmap src=XCreatePixmap(d,root,n,n,32),dst=XCreatePixmap(d,root,n,n,32);
 Picture sp=XRenderCreatePicture(d,src,fmt,0,NULL),dp=XRenderCreatePicture(d,dst,fmt,0,NULL);
 XRenderColor blue={0,0,65535,65535};XRenderFillRectangle(d,PictOpSrc,dp,&blue,0,0,n,n);
 for(unsigned y=0;y<2;y++)for(unsigned x=0;x<2;x++){
  XRenderColor c={x?0:32768,y?0:32768,x?32768:0,32768};
  XRenderFillRectangle(d,PictOpSrc,sp,&c,x*256,y*256,256,256);
 }
 XRenderComposite(d,PictOpOver,sp,None,dp,0,0,0,0,0,0,n,n);
 GlyphSet set=XRenderCreateGlyphSet(d,a8);Glyph ids[5]={1,2,3,4,5};XGlyphInfo info[5];unsigned char data[5*7*8]={0};
 for(unsigned g=0;g<5;g++){
  info[g]=(XGlyphInfo){.width=5,.height=7,.x=0,.y=0,.xOff=6,.yOff=0};
  for(unsigned y=0;y<7;y++)for(unsigned x=0;x<5;x++)data[(g*7+y)*8+x]=(glyphs[g][y]&(1u<<(4-x)))?255:0;
 }
 XRenderAddGlyphs(d,set,ids,info,5,(char*)data,sizeof(data));
 XRenderColor white={65535,65535,65535,65535};Picture solid=XRenderCreateSolidFill(d,&white);char string[]={1,2,3,4,5};
 XRenderCompositeString8(d,PictOpOver,solid,dp,a8,set,0,0,16,32,string,5);
 XSync(d,False);XImage*im=XGetImage(d,dst,0,0,n,n,AllPlanes,ZPixmap);if(!im)return 5;
 unsigned mismatch=0,glyph_count=0;
 FILE*f=fopen("xrender-readback.ppm","wb");if(!f)return 6;fprintf(f,"P6\n%u %u\n255\n",n,n);
 for(unsigned y=0;y<n;y++)for(unsigned x=0;x<n;x++){
  unsigned long p=XGetPixel(im,x,y);unsigned char got[]={(p>>16)&255,(p>>8)&255,p&255};fwrite(got,1,3,f);
  int expected[3]={x<256?128:0,y<256?128:0,x<256?127:255};
  int on=0;if(y>=32&&y<39&&x>=16&&x<45){unsigned g=(x-16)/6,gx=(x-16)%6;if(g<5&&gx<5)on=!!(glyphs[g][y-32]&(1u<<(4-gx)));}
  if(on){glyph_count++;expected[0]=expected[1]=expected[2]=255;}
  unsigned bad=0;for(unsigned c=0;c<3;c++)if(abs((int)got[c]-expected[c])>1)bad++;
  if(bad){if(mismatch<8)fprintf(stderr,"PIXEL FAIL %u,%u got=%u,%u,%u expected=%d,%d,%d\n",x,y,got[0],got[1],got[2],expected[0],expected[1],expected[2]);mismatch++;}
 }
 fclose(f);printf("XRENDER pixels=%u mismatches=%u glyph_pixels=%u %s\n",n*n,mismatch,glyph_count,mismatch?"FAIL":"PASS");fflush(stdout);
 if(!mismatch){
  Window w=XCreateSimpleWindow(d,root,32,64,320,240,0,0,0);XStoreName(d,w,"Tegra desktop XRender test");
  XRenderPictFormat*wf=XRenderFindVisualFormat(d,DefaultVisual(d,screen));Picture wp=XRenderCreatePicture(d,w,wf,0,NULL);
  XMapWindow(d,w);XSync(d,False);
  for(unsigned frame=0;frame<12;frame++){
   XRenderComposite(d,PictOpSrc,dp,None,wp,frame*8,frame*4,0,0,0,0,320,240);
   XMoveWindow(d,w,32+frame*6,64+frame*4);XSync(d,False);
   struct timespec wait={0,40000000};nanosleep(&wait,NULL);
  }
  XRenderFreePicture(d,wp);XDestroyWindow(d,w);XSync(d,False);puts("WINDOW frames=12 complete");
 }
 XDestroyImage(im);XRenderFreePicture(d,solid);XRenderFreeGlyphSet(d,set);XRenderFreePicture(d,sp);XRenderFreePicture(d,dp);XFreePixmap(d,src);XFreePixmap(d,dst);XCloseDisplay(d);
 return mismatch?11:0;
}
