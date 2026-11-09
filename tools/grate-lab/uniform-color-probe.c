/* Bounded GL framebuffer correctness probe; run with external fault monitor. */
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int main(int argc, char **argv)
{
   if (argc != 2 || (strcmp(argv[1], "texture"))) {
      fprintf(stderr, "usage: upload-probe texture\n");
      return 2;
   }
   setvbuf(stderr, NULL, _IONBF, 0);
   fprintf(stderr, "PROBE START phase=%s\n", argv[1]);
   Display *display = XOpenDisplay(NULL);
   if (!display) return 3;
   int attrs[] = { GLX_RGBA, GLX_DOUBLEBUFFER, GLX_DEPTH_SIZE, 16, None };
   XVisualInfo *visual = glXChooseVisual(display, DefaultScreen(display), attrs);
   if (!visual) return 4;
   GLXContext context = glXCreateContext(display, visual, NULL, True);
   if (!context) return 5;
   XSetWindowAttributes wa = {
      .colormap = XCreateColormap(display, RootWindow(display, visual->screen),
                                 visual->visual, AllocNone),
      .event_mask = StructureNotifyMask
   };
   Window window = XCreateWindow(display, RootWindow(display, visual->screen),
      0, 0, 128, 128, 0, visual->depth, InputOutput, visual->visual,
      CWColormap | CWEventMask, &wa);
   XStoreName(display, window, "GRATE diagnostic upload probe");
   XMapWindow(display, window);
   XSync(display, False);
   if (!glXMakeCurrent(display, window, context)) return 6;
   const GLubyte *renderer = glGetString(GL_RENDERER);
   if (!renderer) return 6;
   fprintf(stderr, "PROBE CONTEXT renderer=%s direct=%d\n",
           (const char *)renderer, glXIsDirect(display, context));
   int result = 0;
   unsigned char pixels[64*64*4], readback[128*128*4];
   for (unsigned y=0;y<64;y++) for(unsigned x=0;x<64;x++) {
      unsigned k=(y*64+x)*4;
      pixels[k]=x<32?255:0; pixels[k+1]=y<32?255:0;
      pixels[k+2]=x<32?0:255; pixels[k+3]=255;
   }
   GLuint texture; glGenTextures(1,&texture); glBindTexture(GL_TEXTURE_2D,texture);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAX_LEVEL,0);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
   glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,64,64,0,GL_RGBA,GL_UNSIGNED_BYTE,pixels);
   assert(glGetError()==GL_NO_ERROR);
   glViewport(0,0,128,128); glDisable(GL_DEPTH_TEST); glDisable(GL_DITHER);
   glMatrixMode(GL_PROJECTION);glLoadIdentity();glMatrixMode(GL_MODELVIEW);glLoadIdentity();
   const char *vs_source="#version 120\nattribute vec2 position; attribute vec2 texcoord; varying vec4 uv; void main(){gl_Position=vec4(position,0.0,1.0);uv=vec4(texcoord*2.0,0.0,2.0);}";
   const char *fs_source="#version 120\nuniform sampler2D image; uniform vec4 coord; void main(){gl_FragColor=coord;}";
   GLuint vs=glCreateShader(GL_VERTEX_SHADER),fs=glCreateShader(GL_FRAGMENT_SHADER);
   glShaderSource(vs,1,&vs_source,NULL);glCompileShader(vs);
   glShaderSource(fs,1,&fs_source,NULL);glCompileShader(fs);
   GLint ok;char shaderlog[4096];glGetShaderiv(vs,GL_COMPILE_STATUS,&ok);if(!ok){glGetShaderInfoLog(vs,sizeof(shaderlog),NULL,shaderlog);fprintf(stderr,"VS %s\n",shaderlog);return 11;}
   glGetShaderiv(fs,GL_COMPILE_STATUS,&ok);if(!ok){glGetShaderInfoLog(fs,sizeof(shaderlog),NULL,shaderlog);fprintf(stderr,"FS %s\n",shaderlog);return 12;}
   GLuint program=glCreateProgram();glAttachShader(program,vs);glAttachShader(program,fs);
   glBindAttribLocation(program,0,"position");glBindAttribLocation(program,1,"texcoord");glLinkProgram(program);
   glGetProgramiv(program,GL_LINK_STATUS,&ok);if(!ok){glGetProgramInfoLog(program,sizeof(shaderlog),NULL,shaderlog);fprintf(stderr,"LINK %s\n",shaderlog);return 13;}
   glUseProgram(program);glUniform1i(glGetUniformLocation(program,"image"),0);
   glUniform4f(glGetUniformLocation(program,"coord"),0.25,0.75,0.0,1.0);
   const GLfloat vertices[]={-1,-1,0,0, 1,-1,1,0, 1,1,1,1, -1,1,0,1};
   glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(GLfloat),vertices);
   glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(GLfloat),vertices+2);
   glEnableVertexAttribArray(0);glEnableVertexAttribArray(1);
   glClearColor(0.2,0.2,0.2,1);glClear(GL_COLOR_BUFFER_BIT);
   glDrawArrays(GL_TRIANGLE_FAN,0,4);
   glFinish();assert(glGetError()==GL_NO_ERROR);
   glReadBuffer(GL_BACK);glReadPixels(0,0,128,128,GL_RGBA,GL_UNSIGNED_BYTE,readback);
   assert(glGetError()==GL_NO_ERROR);
   FILE *image=fopen("texture-readback.rgba","wb");if(image){fwrite(readback,1,sizeof(readback),image);fclose(image);}
   unsigned mismatch=0;
   for(unsigned y=4;y<124;y++) for(unsigned x=4;x<124;x++) {
      if(x>=60&&x<68)continue;
      if(y>=60&&y<68)continue;
      unsigned k=(y*128+x)*4;
      unsigned char expected[]={64,191,0,255};
      unsigned bad=0;for(unsigned c=0;c<4;c++)if(abs((int)readback[k+c]-expected[c])>1)bad++;
      if(bad) {
         if(mismatch<8)fprintf(stderr,"PIXEL FAIL %u,%u got=%u,%u,%u,%u expected=%u,%u,%u,%u\n",x,y,readback[k],readback[k+1],readback[k+2],readback[k+3],expected[0],expected[1],expected[2],expected[3]);
         mismatch++;
      }
   }
   fprintf(stderr,"PROBE TEXTURE checked=12544 mismatches=%u %s\n",mismatch,mismatch?"FAIL":"PASS");
   result=mismatch?10:0;glDeleteTextures(1,&texture);
   glXMakeCurrent(display, None, NULL);
   glXDestroyContext(display, context);
   XDestroyWindow(display, window);
   XFreeColormap(display, wa.colormap);
   XFree(visual);
   XCloseDisplay(display);
   fprintf(stderr, "PROBE DONE result=%d\n", result);
   return result;
}
