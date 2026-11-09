/* Diagnostic isolation of engine's default untextured LIT geometry only.
 * Does not implement or repair texture upload or bitmap rendering.
 */
#include <unistd.h>
void glutBitmapCharacter(void *font, int character)
{
 static int once;
 (void)font;(void)character;
 if (!once) {
  static const char s[]="GRATE isolation: bitmap overlay suppressed\n";
  (void)write(2,s,sizeof(s)-1);once=1;
 }
}
int gluBuild2DMipmaps(unsigned int target,int internal_format,int width,int height,
                     unsigned int format,unsigned int type,const void *data)
{
 (void)target;(void)internal_format;(void)width;(void)height;
 (void)format;(void)type;(void)data;
 static const char s[]="GRATE isolation: unused reflection mipmap upload suppressed\n";
 (void)write(2,s,sizeof(s)-1);
 return 0;
}

/* Read the rendered back buffer before swap. XGetImage can return a stale
 * compositor pixmap for a DRI2 window, so it cannot validate rendered pixels. */
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
void glutSwapBuffers(void)
{
 static void (*swap)(void);
 static unsigned frames;
 if(!swap)swap=(void(*)(void))dlsym(RTLD_NEXT,"glutSwapBuffers");
 if(!swap){static const char s[]="GRATE capture: real swap unavailable\n";(void)write(2,s,sizeof(s)-1);_exit(90);}
 if(frames==0){
  const unsigned char*(*getstr)(unsigned)=(const unsigned char*(*)(unsigned))dlsym(RTLD_NEXT,"glGetString");
  if(getstr){const unsigned char*r=getstr(0x1f01);fprintf(stderr,"GRATE renderer: %s\n",r?(const char*)r:"unknown");}
 }
 const char*filename=getenv("GRATE_CAPTURE_PATH");
 if(++frames==3 && filename){
  void(*get)(unsigned,int*)=(void(*)(unsigned,int*))dlsym(RTLD_NEXT,"glGetIntegerv");
  void(*read)(int,int,int,int,unsigned,unsigned,void*)=(void(*)(int,int,int,int,unsigned,unsigned,void*))dlsym(RTLD_NEXT,"glReadPixels");
  unsigned(*error)(void)=(unsigned(*)(void))dlsym(RTLD_NEXT,"glGetError");
  int v[4]={0};
  if(get&&read&&error){
   get(0x0ba2,v);
   if(v[2]>0&&v[3]>0&&v[2]<=2048&&v[3]<=2048){
    unsigned char*p=calloc((size_t)v[2]*v[3],4);
    if(p){
     read(v[0],v[1],v[2],v[3],0x1908,0x1401,p);
     unsigned e=error();
     if(!e){
      FILE*f=fopen(filename,"wbx");
      if(f){
       int ok=fprintf(f,"P6\n%d %d\n255\n",v[2],v[3])>0;
       for(int y=v[3]-1;y>=0;y--)for(int x=0;x<v[2];x++)
        if(fwrite(p+((size_t)y*v[2]+x)*4,1,3,f)!=3)ok=0;
       if(fclose(f))ok=0;
       fprintf(stderr,"GRATE capture: saved=%d width=%d height=%d gl_error=0x%x\n",ok,v[2],v[3],e);
      }else fprintf(stderr,"GRATE capture: cannot create output\n");
     }else fprintf(stderr,"GRATE capture: readback gl_error=0x%x\n",e);
     free(p);
    }
   }
  }
 }
 swap();
}
