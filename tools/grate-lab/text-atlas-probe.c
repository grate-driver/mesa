/* Opaque RGBA8 glyph atlas, nearest sampling, explicit GLSL TEX quad.
 * Run only under an external time limit and kernel-fault monitor.
 */
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
   /* Five 5x7 monochrome glyphs, opaque RGBA atlas: G R A T E. */
   static const unsigned char glyphs[5][7]={
      {14,17,16,23,17,17,14},{30,17,17,30,20,18,17},
      {14,17,17,31,17,17,17},{31,4,4,4,4,4,4},
      {31,16,16,30,16,16,31}};
   memset(pixels,0,sizeof(pixels));
   for(unsigned k=0;k<64*64;k++)pixels[k*4+3]=255;
   for(unsigned g=0;g<5;g++)for(unsigned gy=0;gy<7;gy++)for(unsigned gx=0;gx<5;gx++)
      if(glyphs[g][gy] & (1u<<(4-gx)))for(unsigned dy=0;dy<2;dy++)for(unsigned dx=0;dx<2;dx++) {
         unsigned x=2+g*12+gx*2+dx,y=25+(6-gy)*2+dy,k=(y*64+x)*4;
         pixels[k]=pixels[k+1]=pixels[k+2]=255;
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
   const char *vs_source="#version 120\nattribute vec2 position; attribute vec2 texcoord; varying vec2 uv; void main(){gl_Position=vec4(position,0.0,1.0);uv=texcoord;}";
   const char *fs_source="#version 120\nuniform sampler2D image; varying vec2 uv; void main(){gl_FragColor=texture2D(image,uv);}";
   GLuint vs=glCreateShader(GL_VERTEX_SHADER),fs=glCreateShader(GL_FRAGMENT_SHADER);
   glShaderSource(vs,1,&vs_source,NULL);glCompileShader(vs);
   glShaderSource(fs,1,&fs_source,NULL);glCompileShader(fs);
   GLint ok;char shaderlog[4096];glGetShaderiv(vs,GL_COMPILE_STATUS,&ok);if(!ok){glGetShaderInfoLog(vs,sizeof(shaderlog),NULL,shaderlog);fprintf(stderr,"VS %s\n",shaderlog);return 11;}
   glGetShaderiv(fs,GL_COMPILE_STATUS,&ok);if(!ok){glGetShaderInfoLog(fs,sizeof(shaderlog),NULL,shaderlog);fprintf(stderr,"FS %s\n",shaderlog);return 12;}
   GLuint program=glCreateProgram();glAttachShader(program,vs);glAttachShader(program,fs);
   glBindAttribLocation(program,0,"position");glBindAttribLocation(program,1,"texcoord");glLinkProgram(program);
   glGetProgramiv(program,GL_LINK_STATUS,&ok);if(!ok){glGetProgramInfoLog(program,sizeof(shaderlog),NULL,shaderlog);fprintf(stderr,"LINK %s\n",shaderlog);return 13;}
   glUseProgram(program);glUniform1i(glGetUniformLocation(program,"image"),0);
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
      unsigned k=(y*128+x)*4;
      const unsigned char *expected=pixels+((y/2)*64+x/2)*4;
      if(memcmp(readback+k,expected,4)) {
         if(mismatch<8)fprintf(stderr,"PIXEL FAIL %u,%u got=%u,%u,%u,%u expected=%u,%u,%u,%u\n",x,y,readback[k],readback[k+1],readback[k+2],readback[k+3],expected[0],expected[1],expected[2],expected[3]);
         mismatch++;
      }
   }
   fprintf(stderr,"PROBE OPAQUE_GLYPH_ATLAS checked=14400 mismatches=%u %s\n",mismatch,mismatch?"FAIL":"PASS");
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
