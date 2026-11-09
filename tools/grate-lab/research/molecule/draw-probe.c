#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
static GLuint shader(GLenum t,const char*s){GLuint x=glCreateShader(t);glShaderSource(x,1,&s,0);glCompileShader(x);GLint ok;glGetShaderiv(x,GL_COMPILE_STATUS,&ok);if(!ok)exit(5);return x;}
static GLuint program(int tex){
 const char*vs="#version 120\nattribute vec2 position;attribute vec2 coord;varying vec2 uv;void main(){gl_Position=vec4(position,0,1);uv=coord;}";
 const char*fs=tex?"#version 120\nvarying vec2 uv;uniform sampler2D image;void main(){gl_FragColor=texture2D(image,uv);}":"#version 120\nuniform vec4 color;void main(){gl_FragColor=color;}";
 GLuint p=glCreateProgram();glAttachShader(p,shader(GL_VERTEX_SHADER,vs));glAttachShader(p,shader(GL_FRAGMENT_SHADER,fs));glBindAttribLocation(p,0,"position");glBindAttribLocation(p,1,"coord");glLinkProgram(p);GLint ok;glGetProgramiv(p,GL_LINK_STATUS,&ok);if(!ok)exit(6);return p;
}
static void check(const char*s){GLenum e=glGetError();if(e){fprintf(stderr,"GL error %x at %s\n",e,s);exit(7);}}
int main(int argc,char**argv){
 Display*d=XOpenDisplay(0);if(!d)return 2;
 int attrs[]={GLX_RGBA,GLX_DOUBLEBUFFER,GLX_RED_SIZE,8,GLX_GREEN_SIZE,8,GLX_BLUE_SIZE,8,GLX_ALPHA_SIZE,8,None};
 XVisualInfo*v=glXChooseVisual(d,DefaultScreen(d),attrs);if(!v){fputs("No RGBA8 visual\n",stderr);return 3;}
 XSetWindowAttributes a={.colormap=XCreateColormap(d,RootWindow(d,v->screen),v->visual,AllocNone)};
 Window w=XCreateWindow(d,RootWindow(d,v->screen),0,0,256,256,0,v->depth,InputOutput,v->visual,CWColormap,&a);XStoreName(d,w,"GRATE draw boundary validation");XMapWindow(d,w);XSync(d,False);
 GLXContext ctx=glXCreateContext(d,v,0,True);if(!ctx||!glXMakeCurrent(d,w,ctx))return 4;
 const char*r=(const char*)glGetString(GL_RENDERER);printf("DRAW renderer=%s direct=%d\n",r,glXIsDirect(d,ctx));fflush(stdout);
 if(argc!=2||!r||strcmp(r,argv[1]))return 8;
 GLint ab=0;glGetIntegerv(GL_ALPHA_BITS,&ab);if(ab<8){fprintf(stderr,"alpha bits=%d\n",ab);return 9;}

 glViewport(0,0,256,256);glDisable(GL_DITHER);glDisable(GL_DEPTH_TEST);glDisable(GL_BLEND);glPixelStorei(GL_PACK_ALIGNMENT,1);
 GLuint p=program(0);glUseProgram(p);glUniform4f(glGetUniformLocation(p,"color"),80.f/255,200.f/255,120.f/255,1);glEnableVertexAttribArray(0);
 enum{PREFIX=12,VERTS=64*64*6,PAD=8192,INDEX_START=7806};
 float *positions=malloc((PREFIX+VERTS+PAD)*2*sizeof(float));unsigned *indices=malloc((INDEX_START+VERTS+16)*sizeof(unsigned));unsigned short *shorts=malloc((INDEX_START+VERTS+16)*sizeof(unsigned short));unsigned char *image=malloc(256*256*4);if(!positions||!indices||!shorts||!image)return 12;
 for(unsigned i=0;i<PREFIX+VERTS+PAD;i++)positions[i*2]=positions[i*2+1]=2;
 for(unsigned cell=0;cell<4096;cell++){
  float x=(cell%64)*4,y=(cell/64)*4;float coords[12]={x,y,x+3,y,x+3,y+3,x,y,x+3,y+3,x,y+3};
  for(unsigned k=0;k<6;k++){positions[(PREFIX+cell*6+k)*2]=coords[k*2]/128-1;positions[(PREFIX+cell*6+k)*2+1]=coords[k*2+1]/128-1;}
 }
 for(unsigned i=0;i<INDEX_START+VERTS+16;i++){indices[i]=0;shorts[i]=0;}
 for(unsigned i=0;i<VERTS;i++){indices[INDEX_START+i]=PREFIX+i;shorts[INDEX_START+i]=PREFIX+i;}
 GLuint vb,ib;glGenBuffers(1,&vb);glGenBuffers(1,&ib);glBindBuffer(GL_ARRAY_BUFFER,vb);glBufferData(GL_ARRAY_BUFFER,(PREFIX+VERTS+PAD)*2*sizeof(float),positions,GL_STATIC_DRAW);glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,0);
 const unsigned counts[]={0,3,2046,2047,2048,4095,4096,7050,12345};unsigned checked=0,cases=0;
 for(unsigned kind=0;kind<5;kind++){
  if(kind==1||kind==2){glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ib);if(kind==1)glBufferData(GL_ELEMENT_ARRAY_BUFFER,(INDEX_START+VERTS+16)*sizeof(unsigned short),shorts,GL_STATIC_DRAW);else glBufferData(GL_ELEMENT_ARRAY_BUFFER,(INDEX_START+VERTS+16)*sizeof(unsigned),indices,GL_STATIC_DRAW);}
  else glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
  for(unsigned test=0;test<9;test++){
   unsigned count=counts[test];glClearColor(16.f/255,32.f/255,48.f/255,1);glClear(GL_COLOR_BUFFER_BIT);
   if(kind==0)glDrawArrays(GL_TRIANGLES,PREFIX,count);
   else if(kind==1)glDrawElements(GL_TRIANGLES,count,GL_UNSIGNED_SHORT,(void*)(uintptr_t)(INDEX_START*sizeof(unsigned short)));
   else if(kind==2)glDrawElements(GL_TRIANGLES,count,GL_UNSIGNED_INT,(void*)(uintptr_t)(INDEX_START*sizeof(unsigned)));
   else if(kind==3)glDrawElements(GL_TRIANGLES,count,GL_UNSIGNED_SHORT,shorts+INDEX_START);
   else glDrawElements(GL_TRIANGLES,count,GL_UNSIGNED_INT,indices+INDEX_START);
   glFinish();glReadPixels(0,0,256,256,GL_RGBA,GL_UNSIGNED_BYTE,image);check("triangle batches");
   for(unsigned cell=0;cell<4096;cell++)for(unsigned half=0;half<2;half++){
    unsigned x=(cell%64)*4+(half?0:2),y=(cell/64)*4+(half?2:0),offset=(y*256+x)*4;const unsigned char foreground[4]={80,200,120,255},background[4]={16,32,48,255};const unsigned char*expect=cell*2+half<count/3?foreground:background;
    for(unsigned c=0;c<4;c++){if(abs((int)image[offset+c]-expect[c])>3){fprintf(stderr,"DRAW FAIL kind=%u count=%u cell=%u half=%u channel=%u expected=%u got=%u\n",kind,count,cell,half,c,expect[c],image[offset+c]);return 10;}checked++;}
   }
   printf("DRAW PASS kind=%u count=%u\n",kind,count);fflush(stdout);cases++;
  }
 }
 /* Byte indices, also with a nonzero element offset. Reuse the first cell. */
 unsigned char *bytes=calloc(INDEX_START+7050,1);if(!bytes)return 12;for(unsigned i=0;i<7050;i++)bytes[INDEX_START+i]=PREFIX+i%6;
 glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ib);glBufferData(GL_ELEMENT_ARRAY_BUFFER,INDEX_START+7050,bytes,GL_STATIC_DRAW);glClear(GL_COLOR_BUFFER_BIT);glDrawElements(GL_TRIANGLES,7050,GL_UNSIGNED_BYTE,(void*)(uintptr_t)INDEX_START);glFinish();glReadPixels(0,0,256,256,GL_RGBA,GL_UNSIGNED_BYTE,image);check("byte index batches");
 for(unsigned cell=0;cell<4096;cell++)for(unsigned half=0;half<2;half++){
  unsigned x=(cell%64)*4+(half?0:2),y=(cell/64)*4+(half?2:0),offset=(y*256+x)*4;const unsigned char fg[4]={80,200,120,255},bg[4]={16,32,48,255};const unsigned char*expect=cell==0?fg:bg;
  for(unsigned c=0;c<4;c++){if(abs((int)image[offset+c]-expect[c])>3){fprintf(stderr,"DRAW U8 FAIL cell=%u channel=%u expected=%u got=%u\n",cell,c,expect[c],image[offset+c]);return 11;}checked++;}
 }cases++;
 /* An even chunk step must preserve triangle-strip winding under culling. */
 const unsigned strip_count=9000;for(unsigned i=0;i<strip_count;i++){positions[(PREFIX+i)*2]=-.875f+1.75f*(i/2)/(strip_count/2-1);positions[(PREFIX+i)*2+1]=(i&1)?-.75f:.75f;}
 glBindBuffer(GL_ARRAY_BUFFER,vb);glBufferData(GL_ARRAY_BUFFER,(PREFIX+VERTS+PAD)*2*sizeof(float),positions,GL_STATIC_DRAW);glEnable(GL_CULL_FACE);glCullFace(GL_BACK);glFrontFace(GL_CCW);
 for(unsigned kind=0;kind<3;kind++){
  if(kind){glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,ib);if(kind==1)glBufferData(GL_ELEMENT_ARRAY_BUFFER,(INDEX_START+VERTS+16)*sizeof(unsigned short),shorts,GL_STATIC_DRAW);else glBufferData(GL_ELEMENT_ARRAY_BUFFER,(INDEX_START+VERTS+16)*sizeof(unsigned),indices,GL_STATIC_DRAW);}else glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
  glClear(GL_COLOR_BUFFER_BIT);if(kind==0)glDrawArrays(GL_TRIANGLE_STRIP,PREFIX,strip_count);else glDrawElements(GL_TRIANGLE_STRIP,strip_count,kind==1?GL_UNSIGNED_SHORT:GL_UNSIGNED_INT,(void*)(uintptr_t)(INDEX_START*(kind==1?2:4)));glFinish();glReadPixels(0,0,256,256,GL_RGBA,GL_UNSIGNED_BYTE,image);check("strip winding");
  for(unsigned y=0;y<256;y++)for(unsigned x=0;x<256;x++){
   int foreground=x>=18&&x<238&&y>=34&&y<222,background=x<14||x>242||y<30||y>226;if(!foreground&&!background)continue;
   const unsigned char fg[4]={80,200,120,255},bg[4]={16,32,48,255};const unsigned char*expect=foreground?fg:bg;unsigned offset=(y*256+x)*4;
   for(unsigned c=0;c<4;c++){if(abs((int)image[offset+c]-expect[c])>3){fprintf(stderr,"DRAW STRIP FAIL kind=%u x=%u y=%u channel=%u expected=%u got=%u\n",kind,x,y,c,expect[c],image[offset+c]);return 11;}checked++;}
  }printf("DRAW STRIP PASS kind=%u count=%u\n",kind,strip_count);fflush(stdout);cases++;
 }
 printf("DRAW ALL PASS cases=%u components=%u alpha_bits=%d\n",cases,checked,ab);glXMakeCurrent(d,None,0);glXDestroyContext(d,ctx);XDestroyWindow(d,w);XFree(v);XCloseDisplay(d);free(positions);free(indices);free(shorts);free(image);free(bytes);return 0;
}
